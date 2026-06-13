#include "scraperengine.h"
#include "platformdetector.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

ScraperEngine::ScraperEngine(QObject *parent) : QObject(parent) {}
ScraperEngine::~ScraperEngine() { abort(); }

bool ScraperEngine::isRunning() const
{
    return m_proc && m_proc->state() != QProcess::NotRunning;
}

void ScraperEngine::abort()
{
    m_aborted = true;
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        m_proc->kill();
        m_proc->waitForFinished(3000);
    }
}

QString ScraperEngine::detectPlatform(const QString &url)
{
    return PlatformDetector::name(PlatformDetector::detect(url));
}

bool ScraperEngine::isProfileUrl(const QString &url)
{
    return PlatformDetector::isProfileUrl(url);
}

QString ScraperEngine::normalizeUrl(const QString &url, const QString &filterType)
{
    QString u = url.trimmed();
    while (u.endsWith('/')) u.chop(1);
    if (detectPlatform(u) == "YouTube") {
        if      (filterType == "shorts")  { if (!u.contains("/shorts"))  u += "/shorts";  }
        else if (filterType == "videos")  { if (!u.contains("/videos"))  u += "/videos";  }
        else if (filterType == "streams") { if (!u.contains("/streams")) u += "/streams"; }
    }
    return u;
}

void ScraperEngine::startScrape(const ScrapeJob &job)
{
    if (isRunning()) return;
    m_aborted   = false;
    m_found     = 0;
    m_maxVideos = job.maxVideos;
    m_platform  = job.platform;
    m_buffer.clear();
    m_stderrBuffer.clear();

    QString resolvedUrl = normalizeUrl(job.sourceUrl, job.filterType);

    QStringList args;
    // --flat-playlist = don't fetch full info per video, just list entries
    // --dump-json     = print one JSON line per entry to stdout
    // This is the most compatible approach across all yt-dlp versions
    args << "--flat-playlist";
    args << "--dump-json";
    args << "--no-warnings";
    args << "--no-check-certificates";
    if (job.maxVideos > 0)
        args << "--playlist-end" << QString::number(job.maxVideos);
    args << resolvedUrl;

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_proc, &QProcess::readyReadStandardOutput,
            this,   &ScraperEngine::onReadyRead);
    connect(m_proc, &QProcess::readyReadStandardError,
            this,   &ScraperEngine::onStderrReady);
    connect(m_proc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this,   &ScraperEngine::onProcessFinished);
    connect(m_proc, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError err) {
                if (err == QProcess::FailedToStart) {
                    emit errorOccurred(
                        "yt-dlp not found.\n\n"
                        "Install it from: https://github.com/yt-dlp/yt-dlp/releases\n"
                        "Then add it to your system PATH.");
                    if (m_proc) {
                        m_proc->deleteLater();
                        m_proc = nullptr;
                    }
                    emit finished(0, false);
                }
            });

    emit progress(0, 0, QString("Connecting to %1...").arg(job.platform));
    m_proc->start("yt-dlp", args);
    emit progress(0, 0, "Scraping — please wait...");
}

void ScraperEngine::onReadyRead()
{
    if (!m_proc) return;

    // Read all available data and split into lines
    m_buffer += m_proc->readAllStandardOutput();

    // Process complete lines
    int newlineIdx;
    while ((newlineIdx = m_buffer.indexOf('\n')) != -1) {
        QByteArray line = m_buffer.left(newlineIdx).trimmed();
        m_buffer = m_buffer.mid(newlineIdx + 1);

        if (line.isEmpty()) continue;

        // Each line is a complete JSON object for one video
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;

        QJsonObject entry = doc.object();

        ScrapedVideo v;
        v.id       = entry["id"].toString();
        v.title    = entry["title"].toString();
        if (v.title.isEmpty()) v.title = entry["fulltitle"].toString();
        if (v.title.isEmpty()) v.title = "Video " + QString::number(m_found + 1);

        // Get video URL
        v.url = entry["webpage_url"].toString();
        if (v.url.isEmpty()) v.url = entry["url"].toString();
        if (v.url.isEmpty() && !v.id.isEmpty()) {
            if (m_platform == "YouTube")
                v.url = "https://www.youtube.com/watch?v=" + v.id;
        }
        if (v.url.isEmpty()) continue;

        v.thumbnail = entry["thumbnail"].toString();
        v.uploader  = entry["uploader"].toString();
        if (v.uploader.isEmpty()) v.uploader = entry["channel"].toString();
        v.viewCount = (qint64)entry["view_count"].toDouble(0);
        v.platform  = m_platform;

        // Format duration
        int durSecs = entry["duration"].toInt(0);
        if (durSecs > 0) {
            int h = durSecs / 3600, m = (durSecs % 3600) / 60, s = durSecs % 60;
            if (h > 0)
                v.duration = QString("%1:%2:%3").arg(h)
                                 .arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'));
            else
                v.duration = QString("%1:%2").arg(m).arg(s,2,10,QChar('0'));
        }

        m_found++;
        emit videoFound(v);
        emit progress(m_found, 0, QString("Found %1 video(s)...").arg(m_found));

        if (m_maxVideos > 0 && m_found >= m_maxVideos) {
            abort();
            return;
        }
    }
}

void ScraperEngine::onStderrReady()
{
    if (!m_proc) return;
    m_stderrBuffer += m_proc->readAllStandardError();
}

void ScraperEngine::onProcessFinished(int code, QProcess::ExitStatus)
{
    // Drain any remaining buffered data
    if (m_proc) {
        m_buffer += m_proc->readAllStandardOutput();
        m_stderrBuffer += m_proc->readAllStandardError();
        m_proc->deleteLater();
        m_proc = nullptr;
    }

    // Process any remaining complete lines in buffer
    int newlineIdx;
    while ((newlineIdx = m_buffer.indexOf('\n')) != -1) {
        QByteArray line = m_buffer.left(newlineIdx).trimmed();
        m_buffer = m_buffer.mid(newlineIdx + 1);
        if (line.isEmpty()) continue;
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        QJsonObject entry = doc.object();
        ScrapedVideo v;
        v.id    = entry["id"].toString();
        v.title = entry["title"].toString();
        if (v.title.isEmpty()) v.title = "Video " + QString::number(m_found + 1);
        v.url   = entry["webpage_url"].toString();
        if (v.url.isEmpty()) v.url = entry["url"].toString();
        if (v.url.isEmpty() && !v.id.isEmpty() && m_platform == "YouTube")
            v.url = "https://www.youtube.com/watch?v=" + v.id;
        if (v.url.isEmpty()) continue;
        v.platform = m_platform;
        int d = entry["duration"].toInt(0);
        if (d > 0) {
            int m2 = (d%3600)/60, s2 = d%60;
            v.duration = QString("%1:%2").arg(m2).arg(s2,2,10,QChar('0'));
        }
        m_found++;
        emit videoFound(v);
    }

    // If yt-dlp found nothing and exited with an error, surface the actual
    // reason instead of a silent "no videos found".
    if (m_found == 0 && !m_aborted && code != 0) {
        QString errText = QString::fromUtf8(m_stderrBuffer).trimmed();
        // yt-dlp error lines look like "ERROR: <message>"
        QRegularExpression errRe(R"(ERROR:\s*(.+))");
        auto m = errRe.match(errText);
        QString reason = m.hasMatch() ? m.captured(1).trimmed()
                          : (errText.isEmpty()
                                ? QString("yt-dlp exited with code %1").arg(code)
                                : errText.left(200));
        emit errorOccurred(reason);
    }

    emit finished(m_found, m_aborted);
}