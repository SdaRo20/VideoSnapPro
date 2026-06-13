#include "downloadworker.h"
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QFileInfo>

DownloadWorker::DownloadWorker(DownloadItem item, QObject *parent)
    : QObject(parent), m_item(std::move(item))
{}

void DownloadWorker::abort()
{
    m_aborted = true;
    if (m_proc && m_proc->state() != QProcess::NotRunning)
        m_proc->kill();
}

// ─── Smart format selector ────────────────────────────────────────────────────
// Priority chain: tries best quality first, falls back gracefully.
// Video streams are always paired with best audio then merged by ffmpeg.
QString DownloadWorker::fmtForId(const QString &fmt)
{
    // ── Audio-only ────────────────────────────────────────────────────────────
    if (fmt == "mp3" || fmt == "aac" || fmt == "flac" || fmt == "wav" || fmt == "opus")
        return "bestaudio/best";

    // ── Specific resolution caps ───────────────────────────────────────────────
    // Try AV1 first (best quality/size), then VP9, then H.265, then H.264,
    // then any codec at that resolution, then fall back to best muxed stream.
    auto resCap = [](int h) -> QString {
        return QString(
            // AV1 + best audio (smallest file, best quality)
            "bestvideo[vcodec^=av01][height<=%1]+bestaudio/"
            // VP9 + best audio
            "bestvideo[vcodec^=vp9][height<=%1]+bestaudio/"
            // H.265/HEVC + best audio
            "bestvideo[vcodec^=hev][height<=%1]+bestaudio/"
            "bestvideo[vcodec^=hvc][height<=%1]+bestaudio/"
            // H.264 + best audio (most compatible)
            "bestvideo[vcodec^=avc][height<=%1]+bestaudio/"
            // Any video at resolution + best audio
            "bestvideo[height<=%1]+bestaudio/"
            // Muxed fallback (video+audio in one stream, may be lower quality)
            "best[height<=%1]/"
            // Last resort: just best available
            "best"
        ).arg(h);
    };

    if (fmt == "2160p" || fmt == "4K")  return resCap(2160);
    if (fmt == "1440p")                 return resCap(1440);
    if (fmt == "1080p")                 return resCap(1080);
    if (fmt == "720p")                  return resCap(720);
    if (fmt == "480p")                  return resCap(480);
    if (fmt == "360p")                  return resCap(360);

    // ── "best" — absolute maximum quality, no cap ─────────────────────────────
    // This is what a professional downloader does: grab the highest resolution
    // + best codec video paired with best audio, merge with ffmpeg.
    return
        // 8K AV1
        "bestvideo[vcodec^=av01][height>=4320]+bestaudio/"
        // 4K AV1
        "bestvideo[vcodec^=av01][height>=2160]+bestaudio/"
        // 4K VP9
        "bestvideo[vcodec^=vp9][height>=2160]+bestaudio/"
        // 4K H.264
        "bestvideo[vcodec^=avc][height>=2160]+bestaudio/"
        // Best AV1 any resolution
        "bestvideo[vcodec^=av01]+bestaudio/"
        // Best VP9 any resolution
        "bestvideo[vcodec^=vp9]+bestaudio/"
        // Best H.265
        "bestvideo[vcodec^=hev]+bestaudio/"
        "bestvideo[vcodec^=hvc]+bestaudio/"
        // Best H.264
        "bestvideo[vcodec^=avc]+bestaudio/"
        // Best video any codec + best audio
        "bestvideo+bestaudio/"
        // Muxed fallback
        "best";
}

// ─── Build yt-dlp argument list ───────────────────────────────────────────────
QStringList DownloadWorker::buildArgs() const
{
    QStringList a;
    const QString &f = m_item.format;
    bool isAudio = (f == "mp3" || f == "aac" || f == "flac" ||
                    f == "wav" || f == "opus");

    // ── Format ───────────────────────────────────────────────────────────────
    a << "-f" << fmtForId(f);

    // ── Output container / conversion ────────────────────────────────────────
    if (isAudio) {
        a << "-x";
        a << "--audio-format"  << f;
        a << "--audio-quality" << "0";          // best VBR
    } else {
        // Let ffmpeg pick the best container for the codec combination:
        // AV1/VP9 → mkv,  H.264/H.265 → mp4,  anything else → mkv
        // yt-dlp handles this automatically with "mkv" as preferred merge format
        a << "--merge-output-format" << "mkv/mp4/webm";

        // Explicitly pass ffmpeg location so merge never fails
        if (!m_item.ffmpegPath.isEmpty() && m_item.ffmpegPath != "ffmpeg")
            a << "--ffmpeg-location" << m_item.ffmpegPath;
    }

    // ── Subtitle download ────────────────────────────────────────────────────
    // Only request subtitles on platforms that actually provide them (YouTube).
    // On TikTok/Instagram/etc, --write-subs causes yt-dlp to error out
    // ("Subtitle ... not available") and the whole download is reported as
    // failed even though the video itself downloaded fine.
    if (m_item.platform == Platform::YouTube) {
        a << "--write-subs";
        a << "--write-auto-subs";
        a << "--sub-langs" << "en,en-US,en-GB";
        a << "--convert-subs" << "srt";
    }

    // ── Thumbnail ────────────────────────────────────────────────────────────
    a << "--write-thumbnail";

    // ── Metadata ─────────────────────────────────────────────────────────────
    a << "--add-metadata";
    // NOTE: --write-info-json removed — we don't want .json clutter files

    // ── Performance ──────────────────────────────────────────────────────────
    a << "--concurrent-fragments" << "8";       // 8 parallel fragment downloads
    a << "--retries"              << "10";
    a << "--fragment-retries"     << "10";
    a << "--file-access-retries"  << "5";
    a << "--http-chunk-size"      << "10M";     // 10 MB chunks for speed
    a << "--buffer-size"          << "16K";

    // ── Speed limit ──────────────────────────────────────────────────────────
    if (m_item.speedLimit > 0)
        a << "--limit-rate" << QString::number(m_item.speedLimit);

    // ── Playlist ─────────────────────────────────────────────────────────────
    if (!m_item.isPlaylist)
        a << "--no-playlist";
    else
        a << "--yes-playlist";

    // ── Progress ─────────────────────────────────────────────────────────────
    a << "--newline";                           // one progress line per stdout line
    a << "--progress-template"
      << "download:[download] %(progress._percent_str)s of %(progress._total_bytes_str)s "
         "at %(progress._speed_str)s ETA %(progress._eta_str)s";

    // ── Output path ──────────────────────────────────────────────────────────
    QString outDir = m_item.savePath;
    if (!outDir.endsWith(QDir::separator()))
        outDir += QDir::separator();

    // Separate sub-templates for video, audio, subtitles, thumbnails
    // so files are neatly named and yt-dlp doesn't mix them up
    a << "-o" << (outDir +
        (m_item.isPlaylist
            ? "%(playlist_title)s" + QString(QDir::separator()) +
              "%(playlist_index)s - %(title)s.%(ext)s"
            : "%(title)s.%(ext)s"));

    // Subtitle output template
    a << "-o" << ("subtitle:" + outDir +
        (m_item.isPlaylist
            ? "%(playlist_title)s" + QString(QDir::separator()) +
              "%(playlist_index)s - %(title)s.%(ext)s"
            : "%(title)s.%(ext)s"));

    // Thumbnail output template
    a << "-o" << ("thumbnail:" + outDir +
        (m_item.isPlaylist
            ? "%(playlist_title)s" + QString(QDir::separator()) +
              "%(playlist_index)s - %(title)s.%(ext)s"
            : "%(title)s.%(ext)s"));

    a << m_item.url;
    return a;
}

// ─── Worker thread entry ──────────────────────────────────────────────────────
void DownloadWorker::run()
{
    if (m_aborted) {
        emit finished(m_item.id, false, "Aborted", "");
        return;
    }

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);

    emit progressChanged(m_item.id, 0, "", "", "Starting...", DownloadState::Downloading);

    m_proc->start("yt-dlp", buildArgs());

    if (!m_proc->waitForStarted(10000)) {
        emit finished(m_item.id, false,
            "yt-dlp not found — install it and add to PATH.", "");
        return;
    }

    QString outputFile;
    QString lastDestination;

    while (m_proc->state() != QProcess::NotRunning) {
        m_proc->waitForReadyRead(200);
        while (m_proc->canReadLine()) {
            QString line = QString::fromUtf8(m_proc->readLine()).trimmed();
            if (line.isEmpty()) continue;

            parseLine(line);

            // Capture merged output filename
            // [Merger] Merging formats into "filename.mkv"
            if (line.contains("[Merger] Merging formats into")) {
                int q1 = line.indexOf('"');
                int q2 = line.lastIndexOf('"');
                if (q1 >= 0 && q2 > q1)
                    outputFile = line.mid(q1 + 1, q2 - q1 - 1);
            }

            // [download] Destination: filename
            if (line.startsWith("[download] Destination:")) {
                lastDestination = line.mid(23).trimmed();
                if (m_item.format == "mp3" || m_item.format == "aac" ||
                    m_item.format == "flac" || m_item.format == "wav" ||
                    m_item.format == "opus")
                    outputFile = lastDestination;
            }

            // [ExtractAudio] Destination: filename
            if (line.startsWith("[ExtractAudio] Destination:"))
                outputFile = line.mid(27).trimmed();
        }
        if (m_aborted) break;
    }

    // Drain
    while (m_proc->canReadLine()) {
        QString line = QString::fromUtf8(m_proc->readLine()).trimmed();
        if (!line.isEmpty()) parseLine(line);
    }

    int  code = m_proc->exitCode();
    bool ok   = !m_aborted && (code == 0);

    if (ok && outputFile.isEmpty())
        outputFile = lastDestination;

    if (ok) {
        emit progressChanged(m_item.id, 100, "", "", "Done", DownloadState::Done);

        // Clean up leftover .json, .info, .ytdl, .part files in save folder
        cleanupJunkFiles(m_item.savePath);
    }

    QString failMsg;
    if (m_aborted) {
        failMsg = "Aborted by user";
    } else if (!ok) {
        failMsg = !m_lastError.isEmpty()
                      ? m_lastError.left(200)
                      : QString("yt-dlp failed (exit %1)").arg(code);
    }

    emit finished(m_item.id, ok, failMsg, outputFile);
}

// ─── Delete junk files left by yt-dlp ────────────────────────────────────────
void DownloadWorker::cleanupJunkFiles(const QString &dir)
{
    QDir d(dir);
    if (!d.exists()) return;

    // Remove .info.json, .json, .ytdl, .part files recursively
    static const QStringList junkExts = {
        "*.info.json", "*.json", "*.ytdl", "*.part",
        "*.temp", "*.description", "*.annotations.xml"
    };

    // Also search subdirectories (playlist folders)
    QDirIterator it(dir, junkExts,
                    QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFile::remove(it.filePath());
    }
}

// ─── Parse yt-dlp stdout lines ───────────────────────────────────────────────
void DownloadWorker::parseLine(const QString &line)
{
    // Post-processing
    if (line.contains("[Merger]") || line.contains("[ffmpeg]")) {
        emit progressChanged(m_item.id, 99, "", "",
                             "Merging video + audio...", DownloadState::Converting);
        return;
    }
    if (line.contains("[ExtractAudio]") || line.contains("[AudioConverter]")) {
        emit progressChanged(m_item.id, 99, "", "",
                             "Converting audio...", DownloadState::Converting);
        return;
    }
    if (line.contains("[EmbedThumbnail]")) {
        emit progressChanged(m_item.id, 99, "", "",
                             "Embedding thumbnail...", DownloadState::Converting);
        return;
    }
    if (line.contains("[Metadata]") || line.contains("[MetadataFromField]")) {
        emit progressChanged(m_item.id, 99, "", "",
                             "Writing metadata...", DownloadState::Converting);
        return;
    }
    if (line.contains("[SubtitlesConvertor]") || line.contains("[WriteSubtitles]")) {
        emit progressChanged(m_item.id, 99, "", "",
                             "Saving subtitles...", DownloadState::Converting);
        return;
    }

    // Download progress line
    // [download]  45.3% of  ~180.00MiB at    5.23MiB/s ETA 00:30
    static QRegularExpression reProgress(
        R"(\[download\]\s+([\d\.]+)%\s+of\s+~?([\S]+)\s+at\s+([\S]+)\s+ETA\s+([\S]+))");
    auto m = reProgress.match(line);
    if (m.hasMatch()) {
        int pct = qBound(0, (int)m.captured(1).toDouble(), 99);
        emit progressChanged(m_item.id, pct,
                             m.captured(3), m.captured(4),
                             QString("Downloading %1%").arg(pct),
                             DownloadState::Downloading);
        return;
    }

    // 100%
    if (line.contains("[download] 100%")) {
        emit progressChanged(m_item.id, 99, "", "",
                             "Finalising...", DownloadState::Converting);
        return;
    }

    // Playlist item progress
    static QRegularExpression rePl(
        R"(\[download\] Downloading item (\d+) of (\d+))");
    auto mp = rePl.match(line);
    if (mp.hasMatch()) {
        int cur   = mp.captured(1).toInt();
        int total = mp.captured(2).toInt();
        emit progressChanged(m_item.id,
                             total > 0 ? cur * 100 / total : 0,
                             "", "",
                             QString("Playlist: %1 / %2").arg(cur).arg(total),
                             DownloadState::Downloading);
        return;
    }

    // Error
    if (line.startsWith("ERROR:")) {
        m_lastError = line.mid(6).trimmed();
        emit progressChanged(m_item.id, -1, "", "",
                             line.left(120), DownloadState::Error);
    }
}
