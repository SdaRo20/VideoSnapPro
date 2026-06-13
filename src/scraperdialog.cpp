#include "scraperdialog.h"
#include "scraperengine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QStandardPaths>
#include <QDir>
#include <QSplitter>
#include <QFrame>
#include <QScrollArea>
#include <QApplication>
#include <QClipboard>

ScraperDialog::ScraperDialog(const QString &defaultSavePath,
                             const QString &defaultFormat,
                             QWidget *parent)
    : QDialog(parent)
    , m_defaultSavePath(defaultSavePath)
    , m_defaultFormat(defaultFormat)
{
    setWindowTitle("Scraper");
    setMinimumSize(860, 580);
    resize(960, 660);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_engine = new ScraperEngine(this);
    connect(m_engine, &ScraperEngine::videoFound,    this, &ScraperDialog::onVideoFound);
    connect(m_engine, &ScraperEngine::progress,      this, &ScraperDialog::onProgress);
    connect(m_engine, &ScraperEngine::finished,      this, &ScraperDialog::onFinished);
    connect(m_engine, &ScraperEngine::errorOccurred, this, &ScraperDialog::onError);

    setupUi();
}

void ScraperDialog::setupUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── LEFT PANEL ────────────────────────────────────────────────────────────
    auto *leftPanel = new QWidget;
    leftPanel->setFixedWidth(300);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setStyleSheet(
        "#leftPanel { background: palette(base); border-right: 1px solid palette(mid); }");

    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(14, 14, 14, 14);
    leftLayout->setSpacing(12);

    // Title
    auto *titleLbl = new QLabel("Scraper");
    titleLbl->setStyleSheet("font-size: 16px; font-weight: bold; color: #d4a84b;");
    leftLayout->addWidget(titleLbl);

    auto *subLbl = new QLabel("Collect videos from any profile or channel");
    subLbl->setStyleSheet("font-size: 11px; color: #888;");
    subLbl->setWordWrap(true);
    leftLayout->addWidget(subLbl);

    // Separator
    auto addSep = [&]() {
        auto *sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color: palette(mid);");
        leftLayout->addWidget(sep);
    };
    addSep();

    // ── Scrape Options ────────────────────────────────────────────────────────
    auto *scrapeOptLbl = new QLabel("Scrape Option");
    scrapeOptLbl->setStyleSheet("font-size: 11px; font-weight: bold; color: #aaa; text-transform: uppercase; letter-spacing: 1px;");
    leftLayout->addWidget(scrapeOptLbl);

    // Social media selector
    auto *smRow = new QHBoxLayout;
    auto *smLbl = new QLabel("Social Media");
    smLbl->setFixedWidth(90);
    m_platformCombo = new QComboBox;
    m_platformCombo->addItems({"Auto-detect", "YouTube", "TikTok", "Instagram", "Twitter/X", "Facebook", "Vimeo"});
    smRow->addWidget(smLbl);
    smRow->addWidget(m_platformCombo, 1);
    leftLayout->addLayout(smRow);

    // Content filter
    auto *ftRow = new QHBoxLayout;
    auto *ftLbl = new QLabel("Content Type");
    ftLbl->setFixedWidth(90);
    m_filterCombo = new QComboBox;
    m_filterCombo->addItems({"All Videos", "Videos only", "Shorts only", "Live streams"});
    connect(m_filterCombo, &QComboBox::currentTextChanged,
            this, &ScraperDialog::onFilterChanged);
    ftRow->addWidget(ftLbl);
    ftRow->addWidget(m_filterCombo, 1);
    leftLayout->addLayout(ftRow);

    addSep();

    // ── Download Options ──────────────────────────────────────────────────────
    auto *dlOptLbl = new QLabel("Download Option");
    dlOptLbl->setStyleSheet("font-size: 11px; font-weight: bold; color: #aaa; text-transform: uppercase; letter-spacing: 1px;");
    leftLayout->addWidget(dlOptLbl);

    // Parallel threads
    auto *thrRow = new QHBoxLayout;
    auto *thrLbl = new QLabel("Parallel");
    thrLbl->setFixedWidth(90);
    m_threadSpin = new QSpinBox;
    m_threadSpin->setRange(1, 10);
    m_threadSpin->setValue(3);
    m_threadSpin->setSuffix(" threads");
    thrRow->addWidget(thrLbl);
    thrRow->addWidget(m_threadSpin, 1);
    leftLayout->addLayout(thrRow);

    // Quality
    auto *qualRow = new QHBoxLayout;
    auto *qualLbl = new QLabel("Quality");
    qualLbl->setFixedWidth(90);
    m_formatCombo = new QComboBox;
    m_formatCombo->addItems({"best","1080p","720p","480p","360p","mp3","aac","flac"});
    m_formatCombo->setCurrentText(m_defaultFormat);
    qualRow->addWidget(qualLbl);
    qualRow->addWidget(m_formatCombo, 1);
    leftLayout->addLayout(qualRow);

    // Save path
    auto *saveRow = new QHBoxLayout;
    auto *saveLbl = new QLabel("Save to");
    saveLbl->setFixedWidth(90);
    m_savePath = new QLineEdit(m_defaultSavePath);
    m_savePath->setPlaceholderText("Save folder...");
    m_browseBtn = new QPushButton("...");
    m_browseBtn->setFixedWidth(30);
    connect(m_browseBtn, &QPushButton::clicked, this, &ScraperDialog::onBrowseSave);
    saveRow->addWidget(saveLbl);
    saveRow->addWidget(m_savePath, 1);
    saveRow->addWidget(m_browseBtn);
    leftLayout->addLayout(saveRow);

    addSep();

    // ── Input ─────────────────────────────────────────────────────────────────
    auto *inputLbl = new QLabel("Type");
    inputLbl->setStyleSheet("font-size: 11px; font-weight: bold; color: #aaa; text-transform: uppercase; letter-spacing: 1px;");
    leftLayout->addWidget(inputLbl);

    // Type selector: Username / URL
    auto *typeRow = new QHBoxLayout;
    auto *typeLbl = new QLabel("Input Type");
    typeLbl->setFixedWidth(90);
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({"URL", "Username"});
    typeRow->addWidget(typeLbl);
    typeRow->addWidget(m_typeCombo, 1);
    leftLayout->addLayout(typeRow);

    // Max videos
    auto *maxRow = new QHBoxLayout;
    auto *maxLbl = new QLabel("Max videos");
    maxLbl->setFixedWidth(90);
    m_limitSpin = new QSpinBox;
    m_limitSpin->setRange(1, 9999);
    m_limitSpin->setValue(100);
    maxRow->addWidget(maxLbl);
    maxRow->addWidget(m_limitSpin, 1);
    leftLayout->addLayout(maxRow);

    // URL / Username input area
    m_urlInput = new QTextEdit;
    m_urlInput->setPlaceholderText(
        "https://www.youtube.com/@channel\n"
        "https://www.tiktok.com/@user\n"
        "https://www.instagram.com/user\n\n"
        "Paste multiple URLs (one per line) to scrape\n"
        "them all, or usernames when 'Username' type\n"
        "is selected");
    m_urlInput->setFixedHeight(130);
    m_urlInput->setFont(QFont("Consolas", 10));
    connect(m_urlInput, &QTextEdit::textChanged,
            this, &ScraperDialog::onUrlChanged);
    leftLayout->addWidget(m_urlInput);

    leftLayout->addStretch();

    // ── Scrape / Stop buttons ─────────────────────────────────────────────────
    m_scrapeBtn = new QPushButton("Scrape");
    m_scrapeBtn->setObjectName("btnStart");
    m_scrapeBtn->setFixedHeight(38);
    connect(m_scrapeBtn, &QPushButton::clicked, this, &ScraperDialog::onScrape);
    leftLayout->addWidget(m_scrapeBtn);

    m_abortBtn = new QPushButton("Stop");
    m_abortBtn->setObjectName("btnAbort");
    m_abortBtn->setFixedHeight(34);
    m_abortBtn->setEnabled(false);
    connect(m_abortBtn, &QPushButton::clicked, this, &ScraperDialog::onAbort);
    leftLayout->addWidget(m_abortBtn);

    root->addWidget(leftPanel);

    // ── RIGHT PANEL ───────────────────────────────────────────────────────────
    auto *rightPanel = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(12, 12, 12, 12);
    rightLayout->setSpacing(8);

    // Right panel header
    auto *rightHeader = new QHBoxLayout;
    auto *resultLbl = new QLabel("Results");
    resultLbl->setStyleSheet("font-size: 14px; font-weight: bold;");
    rightHeader->addWidget(resultLbl);
    rightHeader->addStretch();

    m_statusLbl = new QLabel("Enter a URL or username, then click Scrape.");
    m_statusLbl->setStyleSheet("font-size: 11px; color: #888;");
    rightHeader->addWidget(m_statusLbl);
    rightLayout->addLayout(rightHeader);

    // Progress bar
    m_progress = new QProgressBar;
    m_progress->setRange(0, 0);
    m_progress->setFixedHeight(6);
    m_progress->setTextVisible(false);
    m_progress->setVisible(false);
    m_progress->setStyleSheet(
        "QProgressBar { background: palette(mid); border-radius: 3px; border: none; }"
        "QProgressBar::chunk { background: #d4a84b; border-radius: 3px; }");
    rightLayout->addWidget(m_progress);

    // Selection row
    auto *selRow = new QHBoxLayout;
    m_selectAllBtn = new QPushButton("Select All");
    m_selectAllBtn->setFixedWidth(85);
    m_deselectBtn  = new QPushButton("Deselect All");
    m_deselectBtn->setFixedWidth(85);
    m_selectionLbl = new QLabel("0 selected");
    m_selectionLbl->setStyleSheet("color: #d4a84b; font-size: 12px; font-weight: bold;");
    connect(m_selectAllBtn, &QPushButton::clicked, this, &ScraperDialog::onSelectAll);
    connect(m_deselectBtn,  &QPushButton::clicked, this, &ScraperDialog::onDeselectAll);
    selRow->addWidget(m_selectAllBtn);
    selRow->addWidget(m_deselectBtn);
    selRow->addStretch();
    selRow->addWidget(m_selectionLbl);
    rightLayout->addLayout(selRow);

    // Results table
    m_table = new QTableWidget(0, COL_COUNT);
    m_table->setHorizontalHeaderLabels({"", "#", "Title", "Duration", "Views", "URL"});
    auto *hdr = m_table->horizontalHeader();
    hdr->setSectionResizeMode(COL_CHECK, QHeaderView::Fixed);
    m_table->setColumnWidth(COL_CHECK, 28);
    hdr->setSectionResizeMode(COL_NUM,   QHeaderView::Fixed);
    m_table->setColumnWidth(COL_NUM, 40);
    hdr->setSectionResizeMode(COL_TITLE, QHeaderView::Stretch);
    hdr->setSectionResizeMode(COL_DUR,   QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(COL_VIEWS, QHeaderView::ResizeToContents);
    m_table->setColumnHidden(COL_URL, true); // URL hidden, stored as tooltip
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(26);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setStyleSheet(
        "QTableWidget { border: 1px solid palette(mid); border-radius: 6px; }"
        "QHeaderView::section { font-size: 11px; font-weight: bold; }");

    // Click row to toggle checkbox
    connect(m_table, &QTableWidget::cellClicked, this,
        [this](int row, int) {
            auto *cb = qobject_cast<QCheckBox*>(
                m_table->cellWidget(row, COL_CHECK));
            if (cb) {
                cb->setChecked(!cb->isChecked());
                updateSelectionLabel();
            }
        });

    rightLayout->addWidget(m_table, 1);

    // Bottom action row
    auto *bottomRow = new QHBoxLayout;
    auto *cancelBtn = new QPushButton("Cancel");
    cancelBtn->setFixedWidth(80);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_hintLbl = new QLabel("Tick videos you want, then click Download.");
    m_hintLbl->setStyleSheet("font-size: 11px; color: #888;");

    m_sendBtn = new QPushButton("Download Selected");
    m_sendBtn->setObjectName("btnStart");
    m_sendBtn->setFixedHeight(36);
    m_sendBtn->setFixedWidth(160);
    m_sendBtn->setEnabled(false);
    connect(m_sendBtn, &QPushButton::clicked, this, &ScraperDialog::onSendToQueue);

    bottomRow->addWidget(cancelBtn);
    bottomRow->addWidget(m_hintLbl, 1);
    bottomRow->addWidget(m_sendBtn);
    rightLayout->addLayout(bottomRow);

    root->addWidget(rightPanel, 1);
}

// ─── Slots ────────────────────────────────────────────────────────────────────

void ScraperDialog::onUrlChanged()
{
    QString url = m_urlInput->toPlainText().trimmed().split('\n').first().trimmed();
    if (url.isEmpty()) return;

    // Auto-detect platform
    QString plat = ScraperEngine::detectPlatform(url);
    if (plat != "Other") {
        int idx = m_platformCombo->findText(plat);
        if (idx >= 0) m_platformCombo->setCurrentIndex(idx);
    }
}

void ScraperDialog::onFilterChanged()
{
    QString plat = m_platformCombo->currentText();
    m_filterCombo->blockSignals(true);
    m_filterCombo->clear();
    if (plat == "TikTok") {
        m_filterCombo->addItems({"All Videos", "Videos only"});
    } else if (plat == "Instagram") {
        m_filterCombo->addItems({"All", "Reels only", "Posts only"});
    } else {
        m_filterCombo->addItems({"All Videos", "Videos only", "Shorts only", "Live streams"});
    }
    m_filterCombo->blockSignals(false);
}

void ScraperDialog::onBrowseSave()
{
    QString d = QFileDialog::getExistingDirectory(
        this, "Select save folder", m_savePath->text());
    if (!d.isEmpty()) m_savePath->setText(d);
}

void ScraperDialog::onScrape()
{
    // Collect URLs from input
    QStringList rawLines = m_urlInput->toPlainText()
        .split('\n', Qt::SkipEmptyParts);

    QStringList urls;
    for (auto &line : rawLines) {
        QString s = line.trimmed();
        if (s.isEmpty()) continue;

        // If "Username" mode, build URL from username
        if (m_typeCombo->currentText() == "Username") {
            QString plat = m_platformCombo->currentText();
            if (plat == "YouTube" || plat == "Auto-detect")
                s = "https://www.youtube.com/@" + s.remove('@');
            else if (plat == "TikTok")
                s = "https://www.tiktok.com/@" + s.remove('@');
            else if (plat == "Instagram")
                s = "https://www.instagram.com/" + s.remove('@');
            else if (plat == "Twitter/X")
                s = "https://x.com/" + s.remove('@');
            else if (plat == "Facebook")
                s = "https://www.facebook.com/" + s.remove('@');
            else if (plat == "Vimeo")
                s = "https://vimeo.com/" + s.remove('@');
        }
        urls << s;
    }

    if (urls.isEmpty()) {
        QMessageBox::warning(this, "No Input",
            "Please enter a URL or username.");
        return;
    }

    // Clear results
    m_table->setRowCount(0);
    m_videos.clear();
    m_sendBtn->setEnabled(false);
    m_statusLbl->setStyleSheet("font-size: 11px; color: #888;");
    updateSelectionLabel();

    // Queue all URLs — they're scraped one at a time, results accumulate
    // into the same results table.
    m_scrapeQueue      = urls;
    m_scrapeQueueTotal = urls.size();
    m_scrapeQueueDone  = 0;

    setControlsEnabled(false);
    m_progress->setVisible(true);
    m_progress->setRange(0, 0);

    startNextScrapeJob();
}

void ScraperDialog::startNextScrapeJob()
{
    if (m_scrapeQueue.isEmpty()) {
        setControlsEnabled(true);
        m_progress->setVisible(false);

        if (m_videos.isEmpty()) {
            m_statusLbl->setText("No videos found. Check the URL(s) and try again.");
            m_statusLbl->setStyleSheet("font-size: 11px; color: #e04444;");
            return;
        }

        m_statusLbl->setText(
            QString("Done! Found %1 video(s) from %2 source(s). Select and click Download.")
                .arg(m_videos.size()).arg(m_scrapeQueueTotal));
        m_statusLbl->setStyleSheet("font-size: 11px; color: #d4a84b;");
        m_sendBtn->setEnabled(true);
        updateSelectionLabel();
        return;
    }

    QString sourceUrl = m_scrapeQueue.takeFirst();

    ScrapeJob job;
    job.sourceUrl  = sourceUrl;
    job.maxVideos  = m_limitSpin->value();
    job.platform   = m_platformCombo->currentText() == "Auto-detect"
                     ? ScraperEngine::detectPlatform(sourceUrl)
                     : m_platformCombo->currentText();

    QString filter = m_filterCombo->currentText().toLower();
    if      (filter.contains("short"))  job.filterType = "shorts";
    else if (filter.contains("video"))  job.filterType = "videos";
    else if (filter.contains("live"))   job.filterType = "streams";
    else if (filter.contains("reel"))   job.filterType = "reels";
    else                                job.filterType = "all";

    if (m_scrapeQueueTotal > 1) {
        m_statusLbl->setText(
            QString("Scraping source %1 of %2...")
                .arg(m_scrapeQueueDone + 1).arg(m_scrapeQueueTotal));
    } else {
        m_statusLbl->setText("Scraping...");
    }

    m_engine->startScrape(job);
}

void ScraperDialog::onAbort()
{
    m_scrapeQueue.clear();
    m_engine->abort();
    m_abortBtn->setEnabled(false);
    m_statusLbl->setText("Stopping...");
}

void ScraperDialog::onVideoFound(const ScrapedVideo &v)
{
    m_videos.append(v);

    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setRowHeight(row, 26);

    // Checkbox
    auto *cb = new QCheckBox;
    cb->setChecked(true);
    cb->setStyleSheet("margin-left: 5px;");
    connect(cb, &QCheckBox::toggled, this, [this](bool) {
        updateSelectionLabel();
    });
    m_table->setCellWidget(row, COL_CHECK, cb);

    // Number
    auto *numItem = new QTableWidgetItem(QString::number(row + 1));
    numItem->setTextAlignment(Qt::AlignCenter);
    numItem->setForeground(QColor("#888"));
    m_table->setItem(row, COL_NUM, numItem);

    // Title
    auto *titleItem = new QTableWidgetItem(v.title);
    titleItem->setToolTip(v.url);
    m_table->setItem(row, COL_TITLE, titleItem);

    // Duration
    auto *durItem = new QTableWidgetItem(v.duration.isEmpty() ? "-" : v.duration);
    durItem->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, COL_DUR, durItem);

    // Views
    QString viewStr = "-";
    if (v.viewCount > 0) {
        if (v.viewCount >= 1000000)
            viewStr = QString("%1M").arg(v.viewCount/1000000.0, 0, 'f', 1);
        else if (v.viewCount >= 1000)
            viewStr = QString("%1K").arg(v.viewCount/1000.0, 0, 'f', 0);
        else
            viewStr = QString::number(v.viewCount);
    }
    auto *viewItem = new QTableWidgetItem(viewStr);
    viewItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_table->setItem(row, COL_VIEWS, viewItem);

    // URL (hidden)
    m_table->setItem(row, COL_URL, new QTableWidgetItem(v.url));

    updateSelectionLabel();

    // Auto-scroll to last item
    m_table->scrollToBottom();
}

void ScraperDialog::onProgress(int found, int total, const QString &status)
{
    m_statusLbl->setText(status);
    if (total > 0) {
        m_progress->setRange(0, total);
        m_progress->setValue(found);
    }
}

void ScraperDialog::onFinished(int total, bool aborted)
{
    Q_UNUSED(total)

    if (aborted) {
        m_scrapeQueue.clear();
        setControlsEnabled(true);
        m_progress->setVisible(false);
        m_statusLbl->setText(
            QString("Stopped — %1 video(s) found so far.").arg(m_videos.size()));
        m_statusLbl->setStyleSheet("font-size: 11px; color: #d4a84b;");
        if (!m_videos.isEmpty()) {
            m_sendBtn->setEnabled(true);
            updateSelectionLabel();
        }
        return;
    }

    ++m_scrapeQueueDone;
    startNextScrapeJob();
}

void ScraperDialog::onError(const QString &msg)
{
    // If we're in a multi-URL batch, log the error but keep going with the
    // remaining sources instead of stopping the whole batch.
    if (!m_scrapeQueue.isEmpty() || m_scrapeQueueTotal > 1) {
        m_statusLbl->setText("Error on one source — continuing: " + msg.left(60));
        m_statusLbl->setStyleSheet("font-size: 11px; color: #e04444;");
        ++m_scrapeQueueDone;
        startNextScrapeJob();
        return;
    }

    setControlsEnabled(true);
    m_progress->setVisible(false);
    m_statusLbl->setText("Error: " + msg.left(80));
    m_statusLbl->setStyleSheet("font-size: 11px; color: #e04444;");
    QMessageBox::warning(this, "Scrape Error", msg);
}

void ScraperDialog::onSelectAll()
{
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto *cb = qobject_cast<QCheckBox*>(m_table->cellWidget(r, COL_CHECK));
        if (cb) cb->setChecked(true);
    }
    updateSelectionLabel();
}

void ScraperDialog::onDeselectAll()
{
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto *cb = qobject_cast<QCheckBox*>(m_table->cellWidget(r, COL_CHECK));
        if (cb) cb->setChecked(false);
    }
    updateSelectionLabel();
}

void ScraperDialog::onSendToQueue()
{
    QStringList selected;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto *cb = qobject_cast<QCheckBox*>(m_table->cellWidget(r, COL_CHECK));
        if (cb && cb->isChecked()) {
            auto *urlItem = m_table->item(r, COL_URL);
            if (urlItem && !urlItem->text().isEmpty())
                selected << urlItem->text();
            else if (r < m_videos.size())
                selected << m_videos[r].url;
        }
    }

    if (selected.isEmpty()) {
        QMessageBox::information(this, "Nothing Selected",
            "Please tick at least one video.");
        return;
    }

    m_request.urls     = selected;
    m_request.format   = m_formatCombo->currentText();
    m_request.savePath = m_savePath->text();
    m_request.threads  = m_threadSpin->value();

    accept();
}

void ScraperDialog::updateSelectionLabel()
{
    int count = 0;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto *cb = qobject_cast<QCheckBox*>(m_table->cellWidget(r, COL_CHECK));
        if (cb && cb->isChecked()) ++count;
    }
    m_selectionLbl->setText(QString("%1 / %2 selected")
        .arg(count).arg(m_table->rowCount()));
    m_sendBtn->setEnabled(count > 0);
}

void ScraperDialog::setControlsEnabled(bool on)
{
    m_scrapeBtn->setEnabled(on);
    m_abortBtn->setEnabled(!on);
    m_urlInput->setEnabled(on);
    m_platformCombo->setEnabled(on);
    m_filterCombo->setEnabled(on);
    m_limitSpin->setEnabled(on);
    m_typeCombo->setEnabled(on);
}
