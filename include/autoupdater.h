#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QVersionNumber>

// ─────────────────────────────────────────────────────────────────────────────
// AutoUpdater
//
// How it works:
//   1. On startup, sends a GET to the GitHub Releases API:
//      https://api.github.com/repos/<owner>/<repo>/releases/latest
//   2. Parses the JSON response for "tag_name" (e.g. "v2.1.0")
//   3. Compares with APP_VERSION defined in this file
//   4. If newer → emits updateAvailable(latestVersion, releaseUrl, releaseNotes)
//   5. MainWindow connects this signal and shows an update dialog
//
// To publish a new release:
//   1. Push your new code to GitHub
//   2. Go to GitHub → Releases → "Draft a new release"
//   3. Set tag to "v2.1.0" (must match version format below)
//   4. Attach VideoSnapPro.exe as a release asset
//   5. Publish — users see the update dialog on next launch
// ─────────────────────────────────────────────────────────────────────────────

// ── Change these two lines to match YOUR GitHub repo ─────────────────────────
#define GITHUB_OWNER  "YourGitHubUsername"
#define GITHUB_REPO   "VideoSnapPro"

// ── Current app version — bump this every release ────────────────────────────
#define APP_VERSION   "2.0.0"

class AutoUpdater : public QObject
{
    Q_OBJECT
public:
    explicit AutoUpdater(QObject *parent = nullptr);

    // Call once on startup — non-blocking, runs in background
    void checkForUpdates(bool silent = true);

    static QString currentVersion() { return APP_VERSION; }

signals:
    // Emitted only when a newer version exists
    void updateAvailable(const QString &latestVersion,
                         const QString &releaseUrl,
                         const QString &releaseNotes);

    // Emitted when already on latest (only if silent=false)
    void alreadyUpToDate();

    // Emitted on network error (only if silent=false)
    void checkFailed(const QString &reason);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_nam;
    bool                   m_silent = true;

    // Returns true if latestTag is newer than APP_VERSION
    static bool isNewer(const QString &latestTag, const QString &current);
    static QString stripV(const QString &tag); // strips leading "v"
};
