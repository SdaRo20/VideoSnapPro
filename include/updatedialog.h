#pragma once
#include <QDialog>
#include <QString>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include "autoupdater.h"

// Full auto-update dialog:
// Shows release notes + version info, then on "Update Now":
//   1. Downloads .exe to temp folder with progress bar
//   2. Launches the new .exe
//   3. Closes the current app automatically
class UpdateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UpdateDialog(const QString &currentVersion,
                          const QString &latestVersion,
                          const QString &releaseUrl,
                          const QString &downloadUrl,
                          const QString &releaseNotes,
                          AutoUpdater   *updater,
                          QWidget *parent = nullptr);

private slots:
    void onUpdateNow();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(const QString &filePath);
    void onDownloadFailed(const QString &reason);

private:
    void setupUi(const QString &currentVersion, const QString &latestVersion,
                 const QString &releaseNotes);

    QString      m_downloadUrl;
    QString      m_releaseUrl;
    AutoUpdater *m_updater;

    QPushButton *m_updateBtn  = nullptr;
    QPushButton *m_laterBtn   = nullptr;
    QPushButton *m_skipBtn    = nullptr;
    QProgressBar*m_progress   = nullptr;
    QLabel      *m_statusLbl  = nullptr;
    QString      m_latestVersion;
};
