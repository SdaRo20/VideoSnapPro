#include "metafetcher.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QPixmap>

MetaFetcher::MetaFetcher(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &MetaFetcher::onThumbnailDownloaded);
}

void MetaFetcher::fetch(int itemId, const QString &url, bool isPlaylist)
{
    auto *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    m_procToId[proc] = itemId;
    m_idToProc[itemId] = proc;

    connect(proc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MetaFetcher::onProcessFinished);

    QStringList args;
    args << "--dump-json" << "--no-playlist" << "--flat-playlist";
    if (isPlaylist)
        args << "--yes-playlist";
    args << url;

    proc->start("yt-dlp", args);
}

void MetaFetcher::cancel(int itemId)
{
    if (m_idToProc.contains(itemId)) {
        m_idToProc[itemId]->kill();
        m_idToProc.remove(itemId);
    }
}

void MetaFetcher::onProcessFinished(int exitCode, QProcess::ExitStatus)
{
    auto *proc = qobject_cast<QProcess*>(sender());
    if (!proc) return;
    int itemId = m_procToId.value(proc, -1);
    m_procToId.remove(proc);
    m_idToProc.remove(itemId);

    if (exitCode != 0 || itemId < 0) {
        emit metaFailed(itemId, "yt-dlp metadata fetch failed");
        proc->deleteLater();
        return;
    }

    QByteArray output = proc->readAll();
    proc->deleteLater();

    // yt-dlp may output multiple JSON lines for playlists
    QList<QByteArray> lines = output.split('\n');
    QList<QJsonObject> entries;
    for (auto &line : lines) {
        line = line.trimmed();
        if (line.isEmpty()) continue;
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isNull() && doc.isObject())
            entries.append(doc.object());
    }

    if (entries.isEmpty()) {
        emit metaFailed(itemId, "No metadata returned");
        return;
    }

    QJsonObject obj = entries.first();
    QString title       = obj["title"].toString("Unknown");
    int     durSecs     = obj["duration"].toInt(0);
    QString duration    = formatDuration(durSecs);
    QString thumbUrl    = obj["thumbnail"].toString();
    double  fileSizeD   = obj["filesize_approx"].toDouble(0);
    QString fileSize;
    if (fileSizeD > 0) {
        if (fileSizeD > 1e9)
            fileSize = QString::number(fileSizeD/1e9, 'f', 1) + " GB";
        else
            fileSize = QString::number(fileSizeD/1e6, 'f', 1) + " MB";
    }

    bool isPlaylist = obj.contains("_type") && obj["_type"].toString() == "playlist";
    int  plCount    = isPlaylist ? entries.size() : 0;

    emit metaReady(itemId, title, duration, thumbUrl, fileSize, isPlaylist, plCount);

    if (!thumbUrl.isEmpty())
        fetchThumbnail(itemId, thumbUrl);
}

void MetaFetcher::fetchThumbnail(int itemId, const QString &url)
{
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = m_nam->get(req);
    m_replyToId[reply] = itemId;
}

void MetaFetcher::onThumbnailDownloaded(QNetworkReply *reply)
{
    int itemId = m_replyToId.value(reply, -1);
    m_replyToId.remove(reply);
    reply->deleteLater();

    if (itemId < 0 || reply->error() != QNetworkReply::NoError) return;

    QPixmap pix;
    pix.loadFromData(reply->readAll());
    if (!pix.isNull())
        emit thumbnailReady(itemId, pix.scaled(80, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QString MetaFetcher::formatDuration(int secs)
{
    if (secs <= 0) return "";
    int h = secs / 3600, m = (secs % 3600) / 60, s = secs % 60;
    if (h > 0)
        return QString("%1:%2:%3").arg(h).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'));
    return QString("%1:%2").arg(m).arg(s,2,10,QChar('0'));
}
