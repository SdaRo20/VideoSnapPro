#include "updatedialog.h"
#include "autoupdater.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QSettings>
#include <QProcess>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QCoreApplication>

UpdateDialog::UpdateDialog(const QString &currentVersion,
                           const QString &latestVersion,
                           const QString &releaseUrl,
                           const QString &downloadUrl,
                           const QString &releaseNotes,
                           AutoUpdater   *updater,
                           QWidget *parent)
    : QDialog(parent)
    , m_downloadUrl(downloadUrl)
    , m_releaseUrl(releaseUrl)
    , m_updater(updater)
    , m_latestVersion(latestVersion)
{
    setWindowTitle("Update Available — VideoSnap Pro");
    setMinimumWidth(480);
    setMaximumWidth(560);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(m_updater, &AutoUpdater::downloadProgress,
            this, &UpdateDialog::onDownloadProgress);
    connect(m_updater, &AutoUpdater::downloadFinished,
            this, &UpdateDialog::onDownloadFinished);
    connect(m_updater, &AutoUpdater::downloadFailed,
            this, &UpdateDialog::onDownloadFailed);

    setupUi(currentVersion, latestVersion, releaseNotes);
}

void UpdateDialog::setupUi(const QString &currentVersion,
                            const QString &latestVersion,
                            const QString &releaseNotes)
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(14);
    root->setContentsMargins(20, 20, 20, 16);

    // ── Header ────────────────────────────────────────────────────────────────
    auto *headerRow = new QHBoxLayout;
    auto *badge = new QLabel("⬆");
    badge->setStyleSheet(
        "font-size: 28px; background: #d4a84b; border-radius: 10px;"
        "padding: 8px 12px; color: #000;");
    badge->setFixedSize(56, 56);
    badge->setAlignment(Qt::AlignCenter);
    headerRow->addWidget(badge);
    headerRow->addSpacing(12);

    auto *titleCol = new QVBoxLayout;
    auto *title = new QLabel("A new version is available!");
    title->setStyleSheet("font-size: 15px; font-weight: bold;");
    auto *subtitle = new QLabel(
        QString("VideoSnap Pro <b>%1</b> is ready — you have <b>%2</b>")
            .arg(latestVersion, currentVersion));
    subtitle->setStyleSheet("font-size: 12px; color: #888;");
    subtitle->setWordWrap(true);
    titleCol->addWidget(title);
    titleCol->addWidget(subtitle);
    headerRow->addLayout(titleCol, 1);
    root->addLayout(headerRow);

    // ── Separator ─────────────────────────────────────────────────────────────
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    root->addWidget(sep);

    // ── Release notes ─────────────────────────────────────────────────────────
    if (!releaseNotes.trimmed().isEmpty()) {
        auto *notesLabel = new QLabel("What's new:");
        notesLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #aaa;");
        root->addWidget(notesLabel);

        auto *notes = new QTextEdit;
        notes->setPlainText(releaseNotes);
        notes->setReadOnly(true);
        notes->setFixedHeight(110);
        notes->setStyleSheet(
            "font-size: 12px; background: palette(base);"
            "border: 1px solid palette(mid); border-radius: 6px; padding: 6px;");
        root->addWidget(notes);
    }

    // ── Version badges ────────────────────────────────────────────────────────
    auto *vRow = new QHBoxLayout;
    auto mkBadge = [](const QString &lbl, const QString &ver, const QString &bg) {
        auto *w = new QWidget;
        auto *l = new QVBoxLayout(w);
        l->setSpacing(2); l->setContentsMargins(12,8,12,8);
        w->setStyleSheet(QString("background:%1;border-radius:8px;").arg(bg));
        auto *a = new QLabel(lbl);
        a->setStyleSheet("font-size:10px;color:#888;font-weight:bold;");
        a->setAlignment(Qt::AlignCenter);
        auto *b = new QLabel(ver);
        b->setStyleSheet("font-size:14px;font-weight:bold;color:#fff;");
        b->setAlignment(Qt::AlignCenter);
        l->addWidget(a); l->addWidget(b);
        return w;
    };
    vRow->addWidget(mkBadge("INSTALLED", currentVersion, "#333"), 1);
    auto *arrow = new QLabel("→");
    arrow->setAlignment(Qt::AlignCenter);
    arrow->setStyleSheet("font-size:18px;color:#d4a84b;");
    vRow->addWidget(arrow);
    vRow->addWidget(mkBadge("NEW", latestVersion, "#1a3a1a"), 1);
    root->addLayout(vRow);

    // ── Progress bar (hidden until download starts) ───────────────────────────
    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(true);
    m_progress->setFixedHeight(18);
    m_progress->hide();
    root->addWidget(m_progress);

    m_statusLbl = new QLabel("");
    m_statusLbl->setStyleSheet("font-size: 11px; color: #888;");
    m_statusLbl->setAlignment(Qt::AlignCenter);
    m_statusLbl->hide();
    root->addWidget(m_statusLbl);

    // ── Buttons ───────────────────────────────────────────────────────────────
    auto *sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    root->addWidget(sep2);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);

    m_laterBtn = new QPushButton("Remind Me Later");
    m_laterBtn->setFixedHeight(34);
    connect(m_laterBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(m_laterBtn, 1);

    m_skipBtn = new QPushButton("Skip This Version");
    m_skipBtn->setFixedHeight(34);
    m_skipBtn->setStyleSheet("color: #888;");
    connect(m_skipBtn, &QPushButton::clicked, this, [this]() {
        QSettings().setValue("skippedVersion", m_latestVersion);
        reject();
    });
    btnRow->addWidget(m_skipBtn);

    // If no .exe attached → show "Open GitHub" instead of auto-update
    if (m_downloadUrl.isEmpty()) {
        m_updateBtn = new QPushButton("Open GitHub Release");
        m_updateBtn->setObjectName("btnStart");
        m_updateBtn->setFixedHeight(34);
        connect(m_updateBtn, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl(m_releaseUrl));
            accept();
        });
    } else {
        m_updateBtn = new QPushButton("⬇  Update Now");
        m_updateBtn->setObjectName("btnStart");
        m_updateBtn->setFixedHeight(34);
        connect(m_updateBtn, &QPushButton::clicked, this, &UpdateDialog::onUpdateNow);
    }
    m_updateBtn->setFixedWidth(160);
    btnRow->addWidget(m_updateBtn);
    root->addLayout(btnRow);

    // ── Hint ──────────────────────────────────────────────────────────────────
    QString hint = m_downloadUrl.isEmpty()
        ? "No .exe attached to this release. Attach VideoSnapPro.exe to your GitHub release to enable auto-update."
        : "The update will download and install automatically. The app will restart.";
    auto *hintLbl = new QLabel(hint);
    hintLbl->setWordWrap(true);
    hintLbl->setStyleSheet("font-size: 10px; color: #666;");
    hintLbl->setAlignment(Qt::AlignCenter);
    root->addWidget(hintLbl);
}

void UpdateDialog::onUpdateNow()
{
    m_updateBtn->setEnabled(false);
    m_updateBtn->setText("Downloading...");
    m_laterBtn->setEnabled(false);
    m_skipBtn->setEnabled(false);
    m_progress->show();
    m_statusLbl->show();
    m_statusLbl->setText("Connecting to GitHub...");

    m_updater->downloadUpdate(m_downloadUrl);
}

void UpdateDialog::onDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0) {
        int pct = (int)(received * 100 / total);
        m_progress->setValue(pct);

        // Human-readable sizes
        auto fmt = [](qint64 b) -> QString {
            if (b > 1024*1024)
                return QString::number(b/1024/1024.0, 'f', 1) + " MB";
            return QString::number(b/1024) + " KB";
        };
        m_statusLbl->setText(
            QString("Downloading... %1 / %2 (%3%)")
                .arg(fmt(received)).arg(fmt(total)).arg(pct));
    } else {
        m_statusLbl->setText("Downloading...");
    }
}

void UpdateDialog::onDownloadFinished(const QString &filePath)
{
    m_progress->setValue(100);
    m_statusLbl->setText("Download complete! Launching installer...");

    // Launch the new .exe, then quit current app
    // The new exe replaces this one (user's exe path)
    QString currentExe = QCoreApplication::applicationFilePath();

    // Create a tiny batch script that:
    // 1. Waits for current app to close
    // 2. Copies new exe over old one
    // 3. Launches new exe
    QString tempDir  = QFileInfo(filePath).absolutePath();
    QString batchPath = tempDir + "/videosnapro_updater.bat";

    QFile batch(batchPath);
    if (batch.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&batch);
        s << "@echo off\n";
        s << "timeout /t 2 /nobreak > nul\n";
        s << "copy /y \"" << QDir::toNativeSeparators(filePath) << "\" "
          << "\"" << QDir::toNativeSeparators(currentExe) << "\"\n";
        s << "start \"\" \"" << QDir::toNativeSeparators(currentExe) << "\"\n";
        s << "del \"" << QDir::toNativeSeparators(batchPath) << "\"\n";
        batch.close();

        QProcess::startDetached("cmd.exe",
            {"/c", QDir::toNativeSeparators(batchPath)});
    } else {
        // Fallback: just launch the new exe directly
        QProcess::startDetached(filePath, {});
    }

    // Close current app
    QApplication::quit();
}

void UpdateDialog::onDownloadFailed(const QString &reason)
{
    m_updateBtn->setEnabled(true);
    m_updateBtn->setText("⬇  Update Now");
    m_laterBtn->setEnabled(true);
    m_skipBtn->setEnabled(true);
    m_progress->hide();
    m_statusLbl->setText("Download failed: " + reason);
    m_statusLbl->setStyleSheet("font-size: 11px; color: #e04444;");

    QMessageBox::warning(this, "Download Failed",
        "Could not download the update:\n\n" + reason +
        "\n\nClick 'Open GitHub Release' to download manually.");

    m_updateBtn->setText("Open GitHub Release");
    disconnect(m_updateBtn, &QPushButton::clicked, this, &UpdateDialog::onUpdateNow);
    connect(m_updateBtn, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl(m_releaseUrl));
        accept();
    });
}
