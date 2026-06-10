#include "autoupdater.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVersionNumber>
#include <QUrl>

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

    // GitHub Releases API — checks YOUR repo for latest release
    QString url = QString("https://api.github.com/repos/%1/%2/releases/latest")
                      .arg(GITHUB_OWNER)   // "SdaRo20"
                      .arg(GITHUB_REPO);   // "VideoSnapPro"

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

    if (reply->error() != QNetworkReply::NoError) {
        if (!m_silent)
            emit checkFailed(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        if (!m_silent)
            emit checkFailed("Invalid response from GitHub API.");
        return;
    }

    QJsonObject obj = doc.object();

    // "tag_name": "v2.1.0"
    QString latestTag = obj["tag_name"].toString().trimmed();
    if (latestTag.isEmpty()) {
        if (!m_silent)
            emit checkFailed("Could not read version from GitHub.");
        return;
    }

    QString releaseUrl   = obj["html_url"].toString();
    QString releaseNotes = obj["body"].toString();
    if (releaseNotes.length() > 800)
        releaseNotes = releaseNotes.left(800) + "\n...";

    if (isNewer(latestTag, APP_VERSION)) {
        emit updateAvailable(stripV(latestTag), releaseUrl, releaseNotes);
    } else {
        if (!m_silent)
            emit alreadyUpToDate();
    }
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