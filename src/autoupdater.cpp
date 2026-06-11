#include "autoupdater.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVersionNumber>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QCoreApplication>

AutoUpdater::AutoUpdater(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this,  &AutoUpdater::onReplyFinished);
}

void AutoUpdater::checkForUpdates(bool silent)
{
    m_silent = silent;
    QString url = QString("https://api.github.com/repos/%1/%2/releases/latest")
                      .arg(GITHUB_OWNER).arg(GITHUB_REPO);
    QUrl requestUrl(url);
    QNetworkRequest req(requestUrl);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QString("VideoSnapPro/%1").arg(APP_VERSION));
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    m_nam->get(req);
}

void AutoUpdater::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    // If this is a download reply, handled separately
    if (reply == m_downloadReply) return;

    if (reply->error() != QNetworkReply::NoError) {
        if (!m_silent) emit checkFailed(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        if (!m_silent) emit checkFailed("Invalid response from GitHub API.");
        return;
    }

    QJsonObject obj = doc.object();
    QString latestTag = obj["tag_name"].toString().trimmed();
    if (latestTag.isEmpty()) {
        if (!m_silent) emit checkFailed("Could not read version from GitHub.");
        return;
    }

    QString releaseUrl   = obj["html_url"].toString();
    QString releaseNotes = obj["body"].toString();
    if (releaseNotes.length() > 800)
        releaseNotes = releaseNotes.left(800) + "\n...";

    // Find the .exe download URL from release assets
    QString downloadUrl;
    QJsonArray assets = obj["assets"].toArray();
    for (auto a : assets) {
        QJsonObject asset = a.toObject();
        QString name = asset["name"].toString().toLower();
        if (name.endsWith(".exe")) {
            downloadUrl = asset["browser_download_url"].toString();
            break;
        }
    }

    if (isNewer(latestTag, APP_VERSION)) {
        emit updateAvailable(stripV(latestTag), releaseUrl, downloadUrl, releaseNotes);
    } else {
        if (!m_silent) emit alreadyUpToDate();
    }
}

void AutoUpdater::downloadUpdate(const QString &downloadUrl)
{
    if (downloadUrl.isEmpty()) {
        emit downloadFailed("No .exe file attached to this release on GitHub.");
        return;
    }

    // Save to temp folder
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_downloadPath  = tempDir + "/VideoSnapPro_update.exe";

    QUrl url(downloadUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QString("VideoSnapPro/%1").arg(APP_VERSION));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_downloadReply = m_nam->get(req);

    connect(m_downloadReply, &QNetworkReply::downloadProgress,
            this, &AutoUpdater::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished,
            this, &AutoUpdater::onDownloadFinished);
}

void AutoUpdater::onDownloadProgress(qint64 received, qint64 total)
{
    emit downloadProgress(received, total);
}

void AutoUpdater::onDownloadFinished()
{
    if (!m_downloadReply) return;

    if (m_downloadReply->error() != QNetworkReply::NoError) {
        emit downloadFailed(m_downloadReply->errorString());
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
        return;
    }

    // Write downloaded bytes to file
    QFile file(m_downloadPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit downloadFailed("Could not write update file to: " + m_downloadPath);
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
        return;
    }
    file.write(m_downloadReply->readAll());
    file.close();

    m_downloadReply->deleteLater();
    m_downloadReply = nullptr;

    emit downloadFinished(m_downloadPath);
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

QString AutoUpdater::stripV(const QString &tag)
{
    QString s = tag.trimmed();
    if (s.startsWith('v') || s.startsWith('V'))
        s = s.mid(1);
    return s;
}

bool AutoUpdater::isNewer(const QString &latestTag, const QString &current)
{
    QVersionNumber latest  = QVersionNumber::fromString(stripV(latestTag),  nullptr);
    QVersionNumber running = QVersionNumber::fromString(stripV(current), nullptr);
    if (latest.isNull() || running.isNull())
        return stripV(latestTag) > stripV(current);
    return QVersionNumber::compare(latest, running) > 0;
}
