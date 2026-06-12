#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>

SettingsDialog::SettingsDialog(const AppSettings &current, QWidget *parent)
    : QDialog(parent), m_result(current)
{
    setWindowTitle("Settings");
    setMinimumWidth(500);

    // Fix checkboxes — the global Fusion stylesheet breaks native checkbox
    // interaction on Windows. This override restores clickable indicators.
    setStyleSheet(R"(
        QDialog {
            background: palette(window);
        }
        QCheckBox {
            spacing: 10px;
            color: palette(text);
            font-size: 13px;
            padding: 4px 0px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #888888;
            border-radius: 4px;
            background: palette(base);
        }
        QCheckBox::indicator:unchecked:hover {
            border-color: #d4a84b;
            background: palette(base);
        }
        QCheckBox::indicator:unchecked:pressed {
            border-color: #d4a84b;
            background: #2a2a2a;
        }
        QCheckBox::indicator:checked {
            border-color: #d4a84b;
            background: #d4a84b;
            image: none;
        }
        QCheckBox::indicator:checked:hover {
            border-color: #e0b95c;
            background: #e0b95c;
        }
        QTabWidget::pane {
            border: 1px solid palette(mid);
            border-radius: 4px;
        }
        QTabBar::tab {
            padding: 6px 18px;
            font-size: 12px;
        }
        QTabBar::tab:selected {
            font-weight: bold;
        }
    )");

    setupUi(current);
}

void SettingsDialog::setupUi(const AppSettings &s)
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(14, 14, 14, 14);

    auto *tabs = new QTabWidget;

    // ── Tab 1: Downloads ────────────────────────────────────────────────────
    auto *dlTab  = new QWidget;
    auto *dlForm = new QFormLayout(dlTab);
    dlForm->setSpacing(12);
    dlForm->setContentsMargins(14, 14, 14, 14);
    dlForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Save folder
    m_savePath = new QLineEdit(s.savePath);
    auto *browseRow = new QHBoxLayout;
    browseRow->setSpacing(6);
    browseRow->addWidget(m_savePath);
    auto *browseBtn = new QPushButton("Browse...");
    browseBtn->setFixedWidth(80);
    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseSave);
    browseRow->addWidget(browseBtn);
    dlForm->addRow("Save folder:", browseRow);

    // Default quality
    m_defFormat = new QComboBox;
    m_defFormat->addItems({"best","4K / 2160p","1440p","1080p","720p","480p","360p","mp3","aac","flac","wav","opus"});
    m_defFormat->setCurrentText(s.defaultFormat);
    m_defFormat->setFixedWidth(160);
    dlForm->addRow("Default quality:", m_defFormat);

    // Parallel downloads
    m_concurrent = new QSpinBox;
    m_concurrent->setRange(1, 10);
    m_concurrent->setValue(s.concurrent);
    m_concurrent->setSuffix("   (max 10)");
    m_concurrent->setFixedWidth(160);
    dlForm->addRow("Parallel downloads:", m_concurrent);

    // Speed limit
    m_speedLimit = new QDoubleSpinBox;
    m_speedLimit->setRange(0.0, 1000.0);
    m_speedLimit->setValue(s.speedLimitMB);
    m_speedLimit->setSuffix(" MB/s  (0 = unlimited)");
    m_speedLimit->setDecimals(1);
    m_speedLimit->setFixedWidth(220);
    dlForm->addRow("Speed limit:", m_speedLimit);

    // Separator
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    dlForm->addRow(sep);

    // Checkboxes
    m_embedThumb = new QCheckBox("Embed thumbnail into video / audio file");
    m_embedThumb->setChecked(s.embedThumb);
    dlForm->addRow("", m_embedThumb);

    m_addMeta = new QCheckBox("Write metadata tags to file");
    m_addMeta->setChecked(s.addMetadata);
    dlForm->addRow("", m_addMeta);

    m_autoFetch = new QCheckBox("Auto-fetch title + thumbnail when URL is added");
    m_autoFetch->setChecked(s.autoFetch);
    dlForm->addRow("", m_autoFetch);

    tabs->addTab(dlTab, "Downloads");

    // ── Tab 2: Application ──────────────────────────────────────────────────
    auto *appTab  = new QWidget;
    auto *appForm = new QFormLayout(appTab);
    appForm->setSpacing(12);
    appForm->setContentsMargins(14, 14, 14, 14);
    appForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_notifyDone = new QCheckBox("Show system notification when all downloads finish");
    m_notifyDone->setChecked(s.notifyDone);
    appForm->addRow("", m_notifyDone);

    m_minTray = new QCheckBox("Minimize to system tray instead of closing");
    m_minTray->setChecked(s.minimizeToTray);
    appForm->addRow("", m_minTray);

    tabs->addTab(appTab, "Application");

    // ── Tab 3: Tools ────────────────────────────────────────────────────────
    auto *toolTab  = new QWidget;
    auto *toolForm = new QFormLayout(toolTab);
    toolForm->setSpacing(12);
    toolForm->setContentsMargins(14, 14, 14, 14);
    toolForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // yt-dlp path
    m_ytdlpPath = new QLineEdit(s.ytdlpPath);
    auto *ytRow = new QHBoxLayout;
    ytRow->setSpacing(6);
    ytRow->addWidget(m_ytdlpPath);
    auto *ytBtn = new QPushButton("Browse...");
    ytBtn->setFixedWidth(80);
    connect(ytBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseYtdlp);
    ytRow->addWidget(ytBtn);
    toolForm->addRow("yt-dlp path:", ytRow);

    // ffmpeg path
    m_ffmpegPath = new QLineEdit(s.ffmpegPath);
    auto *ffRow = new QHBoxLayout;
    ffRow->setSpacing(6);
    ffRow->addWidget(m_ffmpegPath);
    auto *ffBtn = new QPushButton("Browse...");
    ffBtn->setFixedWidth(80);
    connect(ffBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseFfmpeg);
    ffRow->addWidget(ffBtn);
    toolForm->addRow("ffmpeg path:", ffRow);

    auto *hint = new QLabel(
        "Leave as 'yt-dlp' and 'ffmpeg' if they are already on your system PATH.\n"
        "Use Browse to point to a specific .exe file.");
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #888; font-size: 11px; padding-top: 4px;");
    toolForm->addRow("", hint);

    tabs->addTab(toolTab, "Tools");

    root->addWidget(tabs);

    // OK / Cancel buttons
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btns->button(QDialogButtonBox::Ok)->setFixedWidth(90);
    btns->button(QDialogButtonBox::Cancel)->setFixedWidth(90);
    connect(btns, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
}

void SettingsDialog::onBrowseSave()
{
    QString d = QFileDialog::getExistingDirectory(
        this, "Select save folder", m_savePath->text());
    if (!d.isEmpty()) m_savePath->setText(d);
}

void SettingsDialog::onBrowseYtdlp()
{
    QString f = QFileDialog::getOpenFileName(
        this, "Select yt-dlp executable", "",
        "Executables (*.exe);;All Files (*)");
    if (!f.isEmpty()) m_ytdlpPath->setText(f);
}

void SettingsDialog::onBrowseFfmpeg()
{
    QString f = QFileDialog::getOpenFileName(
        this, "Select ffmpeg executable", "",
        "Executables (*.exe);;All Files (*)");
    if (!f.isEmpty()) m_ffmpegPath->setText(f);
}

void SettingsDialog::onAccept()
{
    m_result.savePath       = m_savePath->text().trimmed();
    m_result.concurrent     = m_concurrent->value();
    m_result.defaultFormat  = m_defFormat->currentText();
    m_result.speedLimitMB   = m_speedLimit->value();
    m_result.embedThumb     = m_embedThumb->isChecked();
    m_result.addMetadata    = m_addMeta->isChecked();
    m_result.autoFetch      = m_autoFetch->isChecked();
    m_result.notifyDone     = m_notifyDone->isChecked();
    m_result.minimizeToTray = m_minTray->isChecked();
    m_result.ytdlpPath      = m_ytdlpPath->text().trimmed();
    m_result.ffmpegPath     = m_ffmpegPath->text().trimmed();
    m_result.save();
    accept();
}
