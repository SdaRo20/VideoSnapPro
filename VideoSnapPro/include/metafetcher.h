#pragma once
#include <QObject>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "downloaditem.h"

// Asynchronously fetches metadata for a URL using yt-dlp --dump-json
class MetaFetcher : public QObject
{
    Q_OBJECT
public:
    explicit MetaFetcher(QObject *parent = nullptr);
    void fetch(int itemId, const QString &url, bool isPlaylist = false);
    void cancel(int itemId);

signals:
    void metaReady(int itemId, const QString &title, const QString &duration,
                   const QString &thumbnailUrl, const QString &fileSize,
                   bool isPlaylist, int playlistCount);
    void metaFailed(int itemId, const QString &reason);
    void thumbnailReady(int itemId, const QPixmap &pix);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onThumbnailDownloaded(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_nam;
    QMap<QProcess*, int>   m_procToId;
    QMap<int, QProcess*>   m_idToProc;
    QMap<QNetworkReply*, int> m_replyToId;

    void fetchThumbnail(int itemId, const QString &url);
    static QString formatDuration(int secs);
};
