#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QVersionNumber>

// ── Change these to match YOUR GitHub repo ───────────────────────────────────
#define GITHUB_OWNER  "SdaRo20"
#define GITHUB_REPO   "VideoSnapPro"
#define APP_VERSION   "2.2.0"

class AutoUpdater : public QObject
{
    Q_OBJECT
public:
    explicit AutoUpdater(QObject *parent = nullptr);

    void checkForUpdates(bool silent = true);
    static QString currentVersion() { return APP_VERSION; }

signals:
    void updateAvailable(const QString &latestVersion,
                         const QString &releaseUrl,
                         const QString &downloadUrl,   // direct .exe URL
                         const QString &releaseNotes);
    void alreadyUpToDate();
    void checkFailed(const QString &reason);

    // Download progress signals
    void downloadProgress(qint64 received, qint64 total);
    void downloadFinished(const QString &filePath);
    void downloadFailed(const QString &reason);

public slots:
    void downloadUpdate(const QString &downloadUrl);

private slots:
    void onReplyFinished(QNetworkReply *reply);
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();

private:
    QNetworkAccessManager *m_nam;
    QNetworkReply         *m_downloadReply = nullptr;
    bool                   m_silent = true;
    QString                m_downloadPath;

    static bool    isNewer(const QString &latestTag, const QString &current);
    static QString stripV(const QString &tag);
};
