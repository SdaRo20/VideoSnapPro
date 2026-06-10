#pragma once
#include <QDialog>
#include <QString>

// Clean dialog shown to user when a newer version is detected on GitHub.
// Shows: current version, new version, release notes, and two buttons:
//   "Download Now" → opens the GitHub release page in the browser
//   "Remind Me Later" → dismisses, won't check again this session

class UpdateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UpdateDialog(const QString &currentVersion,
                          const QString &latestVersion,
                          const QString &releaseUrl,
                          const QString &releaseNotes,
                          QWidget *parent = nullptr);
};
