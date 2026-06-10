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

UpdateDialog::UpdateDialog(const QString &currentVersion,
                           const QString &latestVersion,
                           const QString &releaseUrl,
                           const QString &releaseNotes,
                           QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Update Available — VideoSnap Pro");
    setMinimumWidth(480);
    setMaximumWidth(560);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(14);
    root->setContentsMargins(20, 20, 20, 16);

    // ── Header ───────────────────────────────────────────────────────────────
    auto *headerRow = new QHBoxLayout;

    // App icon / badge area
    auto *badge = new QLabel("⬆");
    badge->setStyleSheet(
        "font-size: 32px;"
        "background: #d4a84b;"
        "border-radius: 10px;"
        "padding: 8px 12px;"
        "color: #000;");
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

    // ── Separator ────────────────────────────────────────────────────────────
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    root->addWidget(sep);

    // ── Release notes ─────────────────────────────────────────────────────────
    if (!releaseNotes.trimmed().isEmpty()) {
        auto *notesLabel = new QLabel("What's new:");
        notesLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #aaa;");
        root->addWidget(notesLabel);

        auto *notes = new QTextEdit;
        notes->setPlainText(releaseNotes);
        notes->setReadOnly(true);
        notes->setFixedHeight(120);
        notes->setStyleSheet(
            "font-size: 12px;"
            "background: palette(base);"
            "border: 1px solid palette(mid);"
            "border-radius: 6px;"
            "padding: 6px;");
        root->addWidget(notes);
    }

    // ── Version badges ───────────────────────────────────────────────────────
    auto *versionRow = new QHBoxLayout;
    versionRow->setSpacing(8);

    auto mkBadge = [](const QString &label, const QString &ver, const QString &bg) {
        auto *w  = new QWidget;
        auto *l  = new QVBoxLayout(w);
        l->setSpacing(2);
        l->setContentsMargins(12,8,12,8);
        w->setStyleSheet(QString("background: %1; border-radius: 8px;").arg(bg));
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size: 10px; color: #888; font-weight: bold;");
        lbl->setAlignment(Qt::AlignCenter);
        auto *vlbl = new QLabel(ver);
        vlbl->setStyleSheet("font-size: 14px; font-weight: bold; color: #fff;");
        vlbl->setAlignment(Qt::AlignCenter);
        l->addWidget(lbl);
        l->addWidget(vlbl);
        return w;
    };

    versionRow->addWidget(mkBadge("CURRENT",  currentVersion, "#333"),  1);
    auto *arrow = new QLabel("→");
    arrow->setAlignment(Qt::AlignCenter);
    arrow->setStyleSheet("font-size: 18px; color: #d4a84b;");
    versionRow->addWidget(arrow);
    versionRow->addWidget(mkBadge("NEW",  latestVersion,  "#1a3a1a"), 1);
    root->addLayout(versionRow);

    // ── Buttons ──────────────────────────────────────────────────────────────
    auto *sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    sep2->setFrameShadow(QFrame::Sunken);
    root->addWidget(sep2);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);

    auto *laterBtn = new QPushButton("Remind Me Later");
    laterBtn->setFixedHeight(34);
    connect(laterBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(laterBtn, 1);

    auto *skipBtn = new QPushButton("Skip This Version");
    skipBtn->setFixedHeight(34);
    skipBtn->setStyleSheet("color: #888;");
    connect(skipBtn, &QPushButton::clicked, this, [this, latestVersion]() {
        // Save skipped version so we don't show it again
        QSettings settings;
        settings.setValue("skippedVersion", latestVersion);
        reject();
    });
    btnRow->addWidget(skipBtn);

    auto *downloadBtn = new QPushButton("⬇  Download Now");
    downloadBtn->setObjectName("btnStart");
    downloadBtn->setFixedHeight(34);
    downloadBtn->setFixedWidth(150);
    connect(downloadBtn, &QPushButton::clicked, this, [this, releaseUrl]() {
        QDesktopServices::openUrl(QUrl(releaseUrl));
        accept();
    });
    btnRow->addWidget(downloadBtn);

    root->addLayout(btnRow);

    // ── Footer note ──────────────────────────────────────────────────────────
    auto *hint = new QLabel(
        "Clicking \"Download Now\" opens the GitHub release page in your browser.\n"
        "Download the new VideoSnapPro.exe and replace the old one.");
    hint->setWordWrap(true);
    hint->setStyleSheet("font-size: 10px; color: #666;");
    hint->setAlignment(Qt::AlignCenter);
    root->addWidget(hint);
}