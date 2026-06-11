#include "mainwindow.h"
#include "progressdelegate.h"
#include "thememanager.h"
#include "historydialog.h"
#include "converterdialog.h"
#include "platformdetector.h"
#include "updatedialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QStandardPaths>
#include <QSettings>
#include <QDir>
#include <QSizePolicy>
#include <QStatusBar>
#include <QMenuBar>
#include <QCloseEvent>
#include <QTimer>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollBar>
#include <QIcon>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("VideoSnap Pro");
    setMinimumSize(1000, 620);
    setWindowIcon(QIcon(":/resources/icon.png"));

    m_settings = AppSettings::load();

    ThemeManager::instance().loadSaved();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeManager::Theme) { /* stylesheet applied globally */ });

    m_queue   = new DownloadQueue(m_settings.concurrent, this);
    m_fetcher = new MetaFetcher(this);
    m_updater = new AutoUpdater(this);

    connect(m_queue,   &DownloadQueue::itemProgress,  this, &MainWindow::onItemProgress);
    connect(m_queue,   &DownloadQueue::itemFinished,  this, &MainWindow::onItemFinished);
    connect(m_queue,   &DownloadQueue::queueEmpty,    this, &MainWindow::onQueueEmpty);
    connect(m_fetcher, &MetaFetcher::metaReady,       this, &MainWindow::onMetaReady);
    connect(m_fetcher, &MetaFetcher::metaFailed,      this, &MainWindow::onMetaFailed);
    connect(m_fetcher, &MetaFetcher::thumbnailReady,  this, &MainWindow::onThumbnailReady);
    connect(m_updater, &AutoUpdater::updateAvailable, this,
        [this](const QString &latest, const QString &url,
               const QString &downloadUrl, const QString &notes) {
            m_lastDownloadUrl = downloadUrl;
            onUpdateAvailable(latest, url, notes);
        });

    setupMenuBar();
    setupUi();
    setupTray();
    restoreWindowState();

    // Aggregate speed display
    m_speedTimer = new QTimer(this);
    m_speedTimer->setInterval(1000);
    connect(m_speedTimer, &QTimer::timeout, this, [this]() {
        if (m_currentSpeeds.isEmpty()) {
            m_speedLabel->setText("");
            return;
        }
        // Sum speeds (rough)
        m_speedLabel->setText(QString("Active: %1 dl").arg(m_queue->activeCount()));
    });
    m_speedTimer->start();
}

MainWindow::~MainWindow()
{
    saveWindowState();
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu("File");
    auto *settAct = new QAction("Settings", this);
    settAct->setShortcut(QKeySequence("Ctrl+,"));
    connect(settAct, &QAction::triggered, this, &MainWindow::onOpenSettings);
    fileMenu->addAction(settAct);
    fileMenu->addSeparator();
    auto *quitAct = new QAction("Quit", this);
    quitAct->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAct, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quitAct);

    auto *toolsMenu = menuBar()->addMenu("Tools");
    auto *histAct = new QAction("Download History", this);
    histAct->setShortcut(QKeySequence("Ctrl+H"));
    connect(histAct, &QAction::triggered, this, &MainWindow::onOpenHistory);
    toolsMenu->addAction(histAct);
    auto *convAct = new QAction("Converter", this);
    convAct->setShortcut(QKeySequence("Ctrl+K"));
    connect(convAct, &QAction::triggered, this, &MainWindow::onOpenConverter);
    toolsMenu->addAction(convAct);

    auto *viewMenu = menuBar()->addMenu("View");
    auto *themeAct = new QAction("Toggle Dark/Light Theme", this);
    themeAct->setShortcut(QKeySequence("Ctrl+T"));
    connect(themeAct, &QAction::triggered, this, &MainWindow::onToggleTheme);
    viewMenu->addAction(themeAct);

    auto *helpMenu = menuBar()->addMenu("Help");
    auto *checkAct = new QAction("Check for Updates...", this);
    connect(checkAct, &QAction::triggered, this, &MainWindow::onCheckForUpdates);
    helpMenu->addAction(checkAct);
    helpMenu->addSeparator();
    auto *aboutAct = new QAction(QString("About VideoSnap Pro v%1").arg(APP_VERSION), this);
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "VideoSnap Pro",
            QString("<b>VideoSnap Pro</b> v%1<br><br>"
                    "A fast, professional batch video downloader.<br>"
                    "Powered by yt-dlp and ffmpeg.<br><br>"
                    "<a href='https://github.com/%2/%3'>GitHub: %2/%3</a>")
            .arg(APP_VERSION).arg(GITHUB_OWNER).arg(GITHUB_REPO));
    });
    helpMenu->addAction(aboutAct);

    // Startup update check — silent (no dialog if already up to date)
    // Small delay so window appears first
    QTimer::singleShot(2000, this, [this]() {
        // Only check if not skipped
        QSettings s;
        // Always check on startup (skipped version handled in onUpdateAvailable)
        m_updater->checkForUpdates(true);
    });
}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(12, 10, 12, 8);
    root->setSpacing(8);

    // ── Header bar ──────────────────────────────────────────────────────────
    auto *headerRow = new QHBoxLayout;
    auto *appTitle = new QLabel("VideoSnap Pro");
    appTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #d4a84b;");
    headerRow->addWidget(appTitle);

    auto *appSub = new QLabel("Batch Downloader");
    appSub->setStyleSheet("font-size: 12px; color: #888; margin-left: 8px;");
    headerRow->addWidget(appSub);
    headerRow->addStretch();

    m_themeBtn = new QPushButton("🌙 Dark");
    m_themeBtn->setObjectName("btnTheme");
    m_themeBtn->setFixedWidth(90);
    connect(m_themeBtn, &QPushButton::clicked, this, &MainWindow::onToggleTheme);
    headerRow->addWidget(m_themeBtn);

    auto *settBtn = new QPushButton("⚙ Settings");
    settBtn->setFixedWidth(90);
    connect(settBtn, &QPushButton::clicked, this, &MainWindow::onOpenSettings);
    headerRow->addWidget(settBtn);

    root->addLayout(headerRow);

    // ── Input box ───────────────────────────────────────────────────────────
    auto *inputBox = new QGroupBox("Add URLs — one per line (YouTube, TikTok, Instagram, …)");
    auto *inputLayout = new QVBoxLayout(inputBox);
    inputLayout->setSpacing(8);

    m_urlInput = new QTextEdit;
    m_urlInput->setPlaceholderText(
        "https://www.youtube.com/watch?v=...\n"
        "https://www.tiktok.com/@user/video/...\n"
        "https://www.youtube.com/playlist?list=...   (playlist supported)");
    m_urlInput->setFixedHeight(90);
    m_urlInput->setFont(QFont("Consolas", 10));
    // Right-click context menu with smart paste
    m_urlInput->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_urlInput, &QTextEdit::customContextMenuRequested, this,
        [this](const QPoint &pos) {
            QMenu *menu = new QMenu(this);
            auto *pasteAct = menu->addAction("Paste URLs from Clipboard");
            pasteAct->setShortcut(QKeySequence("Ctrl+V"));
            connect(pasteAct, &QAction::triggered, this, [this]() {
                QString clip = QApplication::clipboard()->text().trimmed();
                if (clip.isEmpty()) return;
                // Extract all URLs from clipboard (one per line or space-separated)
                QStringList urls;
                QStringList parts = clip.split(QRegularExpression(R"([

	 ]+)"),
                                               Qt::SkipEmptyParts);
                for (auto &p : parts) {
                    if (p.startsWith("http://") || p.startsWith("https://"))
                        urls << p;
                }
                if (!urls.isEmpty()) {
                    m_urlInput->setPlainText(urls.join("\n"));
                    statusBar()->showMessage(
                        QString("Pasted %1 URL(s) from clipboard").arg(urls.size()), 3000);
                } else {
                    // Fallback: just paste raw
                    m_urlInput->paste();
                }
            });
            menu->addSeparator();
            auto *clearAct = menu->addAction("Clear");
            connect(clearAct, &QAction::triggered, m_urlInput, &QTextEdit::clear);
            menu->exec(m_urlInput->mapToGlobal(pos));
            menu->deleteLater();
        });
    inputLayout->addWidget(m_urlInput);

    // Format + buttons row
    auto *optRow = new QHBoxLayout;
    optRow->setSpacing(8);

    auto *fmtLabel = new QLabel("Quality:");
    fmtLabel->setFixedWidth(48);
    m_formatCombo = new QComboBox;
    m_formatCombo->addItems({"best","4K / 2160p","1440p","1080p","720p","480p","360p","mp3","aac","flac","wav","opus"});
    m_formatCombo->setCurrentText(m_settings.defaultFormat);
    m_formatCombo->setFixedWidth(130);
    optRow->addWidget(fmtLabel);
    optRow->addWidget(m_formatCombo);
    optRow->addSpacing(8);

    m_addBtn   = new QPushButton("+ Add to Queue");
    m_startBtn = new QPushButton("Start");
    m_pauseBtn = new QPushButton("Pause");
    m_clearBtn = new QPushButton("Clear Done");
    m_abortBtn = new QPushButton("Abort All");

    m_addBtn->setObjectName("btnAdd");
    m_startBtn->setObjectName("btnStart");
    m_abortBtn->setObjectName("btnAbort");

    for (auto *b : {m_addBtn, m_startBtn, m_pauseBtn, m_clearBtn, m_abortBtn})
        b->setFixedHeight(32);

    // Schedule row — time picker for delayed start
    auto *schedLabel = new QLabel("Schedule:");
    schedLabel->setFixedWidth(60);
    m_scheduleEdit = new QTimeEdit;
    m_scheduleEdit->setDisplayFormat("HH:mm");
    m_scheduleEdit->setTime(QTime::currentTime());
    m_scheduleEdit->setFixedWidth(70);
    m_scheduleEdit->setToolTip("Set a time to start downloads automatically");
    m_scheduleBtn = new QPushButton("Set Timer");
    m_scheduleBtn->setFixedHeight(32);
    m_scheduleBtn->setFixedWidth(90);
    m_scheduleBtn->setToolTip("Schedule downloads to start at the selected time");
    m_scheduleLbl = new QLabel("");
    m_scheduleLbl->setStyleSheet("color: #d4a84b; font-size: 11px;");

    optRow->addWidget(m_addBtn);
    optRow->addWidget(m_startBtn);
    optRow->addWidget(m_pauseBtn);
    optRow->addSpacing(16);
    optRow->addWidget(schedLabel);
    optRow->addWidget(m_scheduleEdit);
    optRow->addWidget(m_scheduleBtn);
    optRow->addWidget(m_scheduleLbl);
    optRow->addStretch();
    optRow->addWidget(m_clearBtn);
    optRow->addWidget(m_abortBtn);

    inputLayout->addLayout(optRow);
    root->addWidget(inputBox);

    // ── Download Table ───────────────────────────────────────────────────────
    m_table = new QTableWidget(0, COL_COUNT);
    m_table->setHorizontalHeaderLabels({
        "Thumb","Title","Platform","Format","Status","Progress","Speed","ETA","Size","✕"
    });

    auto *hdr = m_table->horizontalHeader();
    hdr->setSectionResizeMode(COL_THUMB,    QHeaderView::Fixed);       m_table->setColumnWidth(COL_THUMB, 90);
    hdr->setSectionResizeMode(COL_TITLE,    QHeaderView::Stretch);
    hdr->setSectionResizeMode(COL_PLATFORM, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(COL_FORMAT,   QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(COL_STATUS,   QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(COL_PROGRESS, QHeaderView::Fixed);        m_table->setColumnWidth(COL_PROGRESS, 110);
    hdr->setSectionResizeMode(COL_SPEED,    QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(COL_ETA,      QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(COL_SIZE,     QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(COL_ACTION,   QHeaderView::Fixed);        m_table->setColumnWidth(COL_ACTION, 36);

    m_table->verticalHeader()->setDefaultSectionSize(54);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setItemDelegate(new ProgressDelegate(this));
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->setShowGrid(false);

    connect(m_table, &QTableWidget::customContextMenuRequested, this,
        [this](const QPoint &pos) {
            QMenu menu;
            menu.addAction("Remove selected", this, &MainWindow::onRemoveSelectedRows);
            menu.addAction("Open output file", this, &MainWindow::onOpenOutputFile);
            menu.addAction("Copy URL", this, &MainWindow::onCopyUrl);
            menu.exec(m_table->viewport()->mapToGlobal(pos));
        });

    root->addWidget(m_table, 1);

    // ── Status bar ───────────────────────────────────────────────────────────
    m_statusLabel = new QLabel("Ready");
    m_speedLabel  = new QLabel("");
    m_speedLabel->setStyleSheet("color: #d4a84b; font-size: 11px;");
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_speedLabel);

    // ── Connections ──────────────────────────────────────────────────────────
    connect(m_addBtn,     &QPushButton::clicked, this, &MainWindow::onAddLinks);
    connect(m_startBtn,   &QPushButton::clicked, this, &MainWindow::onStartAll);
    connect(m_pauseBtn,   &QPushButton::clicked, this, &MainWindow::onPauseResume);
    connect(m_clearBtn,   &QPushButton::clicked, this, &MainWindow::onClearDone);
    connect(m_abortBtn,   &QPushButton::clicked, this, &MainWindow::onAbortAll);
    connect(m_scheduleBtn,&QPushButton::clicked, this, &MainWindow::onScheduleDownload);

    // Update theme button label
    auto updateThemeBtn = [this]() {
        bool dark = ThemeManager::instance().currentTheme() == ThemeManager::Dark;
        m_themeBtn->setText(dark ? "Light Mode" : "Dark Mode");
    };
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [updateThemeBtn](ThemeManager::Theme) { updateThemeBtn(); });
    updateThemeBtn();
}

void MainWindow::setupTray()
{
    m_tray = new QSystemTrayIcon(this);
    m_tray->setToolTip("VideoSnap Pro");

    // Use a simple colored icon
    QPixmap px(16,16);
    px.fill(QColor(212,168,75));
    m_tray->setIcon(QIcon(px));

    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction("Show", this, &QWidget::show);
    m_trayMenu->addAction("Start Downloads", this, &MainWindow::onStartAll);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction("Quit", this, [this]() {
        m_tray->hide();
        QApplication::quit();
    });
    m_tray->setContextMenu(m_trayMenu);
    m_tray->show();

    connect(m_tray, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayActivated);
}

void MainWindow::applySettings()
{
    m_queue->setMaxConcurrent(m_settings.concurrent);
    m_formatCombo->setCurrentText(m_settings.defaultFormat);
}

// ─── WINDOW STATE ────────────────────────────────────────────────────────────

void MainWindow::saveWindowState()
{
    QSettings s;
    s.setValue("geometry", saveGeometry());
    s.setValue("windowState", saveState());
}

void MainWindow::restoreWindowState()
{
    QSettings s;
    if (s.contains("geometry")) restoreGeometry(s.value("geometry").toByteArray());
    if (s.contains("windowState")) restoreState(s.value("windowState").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (m_settings.minimizeToTray && m_tray->isVisible()) {
        hide();
        m_tray->showMessage("VideoSnap Pro",
            "Running in the system tray. Right-click to quit.",
            QSystemTrayIcon::Information, 2000);
        e->ignore();
    } else {
        saveWindowState();
        e->accept();
    }
}

void MainWindow::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange &&
        isMinimized() && m_settings.minimizeToTray) {
        hide();
        e->ignore();
        return;
    }
    QMainWindow::changeEvent(e);
}

// ─── TABLE HELPERS ────────────────────────────────────────────────────────────

int MainWindow::addTableRow(const DownloadItem &item)
{
    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setRowHeight(row, 54);

    // Thumb placeholder
    auto *thumbItem = new QTableWidgetItem();
    thumbItem->setData(ItemIdRole, item.id);
    m_table->setItem(row, COL_THUMB, thumbItem);

    // Title (short URL until metadata arrives)
    auto *titleItem = new QTableWidgetItem(
        item.title.isEmpty() ? shortUrl(item.url) : item.title);
    titleItem->setToolTip(item.url);
    titleItem->setData(ItemIdRole, item.id);
    m_table->setItem(row, COL_TITLE, titleItem);

    // Platform
    auto *platItem = new QTableWidgetItem(PlatformDetector::name(item.platform));
    platItem->setForeground(QColor(PlatformDetector::color(item.platform)));
    platItem->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, COL_PLATFORM, platItem);

    // Format
    auto *fmtItem = new QTableWidgetItem(item.format.toUpper());
    fmtItem->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, COL_FORMAT, fmtItem);

    // Status
    auto *statItem = new QTableWidgetItem("Queued");
    statItem->setForeground(QColor("#888"));
    statItem->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, COL_STATUS, statItem);

    // Progress
    auto *progItem = new QTableWidgetItem();
    progItem->setData(Qt::DisplayRole, 0);
    m_table->setItem(row, COL_PROGRESS, progItem);

    // Speed / ETA / Size
    auto mk = [](const QString &t = "-") {
        auto *i = new QTableWidgetItem(t);
        i->setTextAlignment(Qt::AlignCenter);
        return i;
    };
    m_table->setItem(row, COL_SPEED, mk());
    m_table->setItem(row, COL_ETA,   mk());
    m_table->setItem(row, COL_SIZE,  mk(item.fileSize.isEmpty() ? "-" : item.fileSize));

    // Remove button
    auto *rmBtn = new QPushButton("✕");
    rmBtn->setFixedSize(30, 30);
    rmBtn->setStyleSheet("QPushButton { background: transparent; border: none; "
                         "color: #e04444; font-size: 14px; }"
                         "QPushButton:hover { color: #ff6666; }");
    int capturedId = item.id;
    connect(rmBtn, &QPushButton::clicked, this, [this, capturedId]() {
        m_queue->abortItem(capturedId);
        int r = findRowById(capturedId);
        if (r >= 0) m_table->removeRow(r);
        m_items.remove(capturedId);
        updateStatusBar();
    });
    m_table->setCellWidget(row, COL_ACTION, rmBtn);

    m_items[item.id] = item;
    return row;
}

int MainWindow::findRowById(int id) const
{
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto *it = m_table->item(r, COL_THUMB);
        if (it && it->data(ItemIdRole).toInt() == id) return r;
    }
    return -1;
}

void MainWindow::updateStatusBar()
{
    int total = m_table->rowCount();
    int active = m_queue->activeCount();
    int queued = m_queue->queuedCount();
    m_statusLabel->setText(
        QString("Total: %1  |  Active: %2  |  Queued: %3  |  Done: %4  |  Errors: %5")
        .arg(total).arg(active).arg(queued).arg(m_doneCount).arg(m_errorCount));
}

QString MainWindow::shortUrl(const QString &url) const
{
    // Show just the domain + tail
    QUrl u(url);
    QString s = u.host() + u.path();
    if (s.length() > 60) s = s.left(57) + "...";
    return s;
}

// ─── ACTIONS ─────────────────────────────────────────────────────────────────

// ─── Detect if URL is a channel/profile (not a single video) ────────────────
static bool isChannelOrProfileUrl(const QString &url)
{
    // YouTube channel/user patterns
    if (url.contains("youtube.com/@"))       return true;
    if (url.contains("youtube.com/channel/") &&
        !url.contains("/watch"))             return true;
    if (url.contains("youtube.com/user/"))   return true;
    if (url.contains("youtube.com/c/"))      return true;
    // TikTok profile
    if (QRegularExpression(R"(tiktok\.com/@[^/]+$)").match(url).hasMatch()) return true;
    // Instagram profile
    if (QRegularExpression(R"(instagram\.com/[^/p/reel])").match(url).hasMatch()
        && !url.contains("/p/") && !url.contains("/reel/")) return true;
    return false;
}

void MainWindow::onAddLinks()
{
    QString raw = m_urlInput->toPlainText().trimmed();
    if (raw.isEmpty()) { statusBar()->showMessage("No URLs entered.", 3000); return; }

    QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
    QString fmt  = m_formatCombo->currentText();

    // Always use persisted save path from settings
    QString path = m_settings.savePath;
    if (path.isEmpty())
        path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir().mkpath(path);

    int added = 0;
    QStringList channelUrls;

    for (const QString &line : lines) {
        QString url = line.trimmed();
        if (url.isEmpty() || url.startsWith('#')) continue;

        // Auto-detect channel/profile URLs — treat as playlist
        bool isProfile = isChannelOrProfileUrl(url);
        bool isPlaylist = isProfile || PlatformDetector::isPlaylistUrl(url);

        // For profile URLs, append /videos to get all videos if YouTube
        QString resolvedUrl = url;
        if (isProfile && url.contains("youtube.com/@") && !url.contains("/videos"))
            resolvedUrl = url.trimmed();
        if (resolvedUrl.endsWith('/')) resolvedUrl.chop(1);
        resolvedUrl += "/videos";

        DownloadItem item;
        item.id         = m_nextId++;
        item.url        = resolvedUrl;
        item.format     = fmt == "4K / 2160p" ? "4K" : fmt;
        item.savePath   = path;
        item.ffmpegPath = m_settings.ffmpegPath;
        item.platform   = PlatformDetector::detect(url);
        item.isPlaylist = isPlaylist;
        item.addedAt    = QDateTime::currentDateTime();
        item.speedLimit = (qint64)(m_settings.speedLimitMB * 1024 * 1024);

        addTableRow(item);

        if (m_settings.autoFetch)
            m_fetcher->fetch(item.id, resolvedUrl, isPlaylist);

        if (isProfile) channelUrls << url;
        ++added;
    }

    m_urlInput->clear();

    QString msg = QString("Added %1 URL(s).").arg(added);
    if (!channelUrls.isEmpty())
        msg += QString(" %1 channel/profile URL(s) detected — will download all videos.")
               .arg(channelUrls.size());

    statusBar()->showMessage(msg, 5000);
    updateStatusBar();
}

void MainWindow::onStartAll()
{
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto *statItem = m_table->item(r, COL_STATUS);
        if (!statItem) continue;
        if (statItem->text() != "Queued" && statItem->text() != "Queued...") continue;

        auto *thumbItem = m_table->item(r, COL_THUMB);
        if (!thumbItem) continue;
        int id = thumbItem->data(ItemIdRole).toInt();

        if (!m_items.contains(id)) continue;
        DownloadItem item = m_items[id];

        statItem->setText("Queued...");
        statItem->setForeground(QColor("#d4a84b"));
        m_queue->enqueue(item);
    }
    if (m_paused) { m_paused = false; m_queue->resumeQueue(); }
    statusBar()->showMessage("Downloads started...");
    updateStatusBar();
}

void MainWindow::onPauseResume()
{
    m_paused = !m_paused;
    if (m_paused) {
        m_queue->pauseQueue();
        m_pauseBtn->setText("▶ Resume");
        statusBar()->showMessage("Queue paused — active downloads continue until done.");
    } else {
        m_queue->resumeQueue();
        m_pauseBtn->setText("⏸ Pause");
        statusBar()->showMessage("Queue resumed.");
    }
}

void MainWindow::onClearDone()
{
    for (int r = m_table->rowCount()-1; r >= 0; --r) {
        auto *s = m_table->item(r, COL_STATUS);
        if (!s) continue;
        if (s->text() == "Done" || s->text().startsWith("Error") ||
            s->text() == "Aborted") {
            auto *t = m_table->item(r, COL_THUMB);
            if (t) m_items.remove(t->data(ItemIdRole).toInt());
            m_table->removeRow(r);
        }
    }
    updateStatusBar();
}

void MainWindow::onAbortAll()
{
    m_queue->abortAll();
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto *s = m_table->item(r, COL_STATUS);
        if (s && s->text() != "Done" && !s->text().startsWith("Error")) {
            s->setText("Aborted");
            s->setForeground(QColor("#e04444"));
            auto *p = m_table->item(r, COL_PROGRESS);
            if (p) p->setData(Qt::DisplayRole, 0);
        }
    }
    statusBar()->showMessage("All aborted.", 4000);
    updateStatusBar();
}

void MainWindow::onOpenSettings()
{
    SettingsDialog dlg(m_settings, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_settings = dlg.result();
        m_settings.save();  // immediately persist to QSettings
        applySettings();
        statusBar()->showMessage("Settings saved.", 2000);
    }
}

void MainWindow::onOpenHistory()
{
    HistoryDialog dlg(this);
    dlg.exec();
}

void MainWindow::onOpenConverter()
{
    ConverterDialog dlg(this);
    dlg.exec();
}

void MainWindow::onToggleTheme()
{
    auto &tm = ThemeManager::instance();
    tm.setTheme(tm.currentTheme() == ThemeManager::Dark
                ? ThemeManager::Light : ThemeManager::Dark);
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        show();
        raise();
        activateWindow();
    }
}

void MainWindow::onRemoveSelectedRows()
{
    auto rows = m_table->selectionModel()->selectedRows();
    // Sort descending
    std::sort(rows.begin(), rows.end(), [](const QModelIndex &a, const QModelIndex &b){
        return a.row() > b.row();
    });
    for (auto &idx : rows) {
        int r = idx.row();
        auto *t = m_table->item(r, COL_THUMB);
        if (t) {
            int id = t->data(ItemIdRole).toInt();
            m_queue->abortItem(id);
            m_items.remove(id);
        }
        m_table->removeRow(r);
    }
    updateStatusBar();
}

void MainWindow::onOpenOutputFile()
{
    int r = m_table->currentRow();
    if (r < 0) return;
    auto *t = m_table->item(r, COL_THUMB);
    if (!t) return;
    int id = t->data(ItemIdRole).toInt();
    if (m_items.contains(id) && !m_items[id].outputFile.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_items[id].outputFile));
}

void MainWindow::onCopyUrl()
{
    int r = m_table->currentRow();
    if (r < 0) return;
    auto *t = m_table->item(r, COL_TITLE);
    if (t) QApplication::clipboard()->setText(t->toolTip());
}

// ─── META CALLBACKS ──────────────────────────────────────────────────────────

void MainWindow::onMetaReady(int id, const QString &title, const QString &duration,
    const QString &thumbnailUrl, const QString &fileSize, bool, int)
{
    int r = findRowById(id);
    if (r < 0) return;

    if (!title.isEmpty()) {
        auto *t = m_table->item(r, COL_TITLE);
        if (t) {
            QString display = title;
            if (!duration.isEmpty()) display += "  [" + duration + "]";
            t->setText(display);
        }
    }
    if (!fileSize.isEmpty()) {
        auto *s = m_table->item(r, COL_SIZE);
        if (s) s->setText(fileSize);
    }
    if (m_items.contains(id)) {
        m_items[id].title    = title;
        m_items[id].duration = duration;
        m_items[id].fileSize = fileSize;
    }
}

void MainWindow::onMetaFailed(int id, const QString &)
{
    // Silently ignore — URL still queued, just won't have title
    Q_UNUSED(id)
}

void MainWindow::onThumbnailReady(int id, const QPixmap &pix)
{
    int r = findRowById(id);
    if (r < 0) return;
    auto *t = m_table->item(r, COL_THUMB);
    if (t) t->setData(Qt::DecorationRole, pix);
    if (m_items.contains(id)) m_items[id].thumbnail = pix;
}

// ─── QUEUE CALLBACKS ─────────────────────────────────────────────────────────

void MainWindow::onItemProgress(int id, int percent, const QString &speed,
    const QString &eta, const QString &status, DownloadState state)
{
    int r = findRowById(id);
    if (r < 0) return;

    auto *statItem = m_table->item(r, COL_STATUS);
    auto *progItem = m_table->item(r, COL_PROGRESS);
    auto *spdItem  = m_table->item(r, COL_SPEED);
    auto *etaItem  = m_table->item(r, COL_ETA);

    if (statItem) {
        statItem->setText(status);
        QColor c = (state == DownloadState::Done)      ? QColor(80,200,80) :
                   (state == DownloadState::Error)     ? QColor(220,60,60) :
                   (state == DownloadState::Converting)? QColor(100,180,255) :
                                                         QColor(212,168,75);
        statItem->setForeground(c);
    }
    if (progItem) progItem->setData(Qt::DisplayRole, qMax(0,percent));
    if (spdItem)  spdItem->setText(speed.isEmpty() ? "-" : speed);
    if (etaItem)  etaItem->setText(eta.isEmpty()   ? "-" : eta);

    if (!speed.isEmpty()) m_currentSpeeds[id] = speed;
    updateStatusBar();
}

void MainWindow::onItemFinished(int id, bool success, const QString &errorMsg,
                                const QString &outputFile)
{
    int r = findRowById(id);
    if (r < 0) return;

    m_currentSpeeds.remove(id);

    if (success) ++m_doneCount; else ++m_errorCount;

    auto *statItem = m_table->item(r, COL_STATUS);
    auto *progItem = m_table->item(r, COL_PROGRESS);
    auto *spdItem  = m_table->item(r, COL_SPEED);
    auto *etaItem  = m_table->item(r, COL_ETA);

    if (statItem) {
        statItem->setText(success ? "Done" : "Error");
        statItem->setForeground(success ? QColor(80,200,80) : QColor(220,60,60));
        if (!success && !errorMsg.isEmpty())
            statItem->setToolTip(errorMsg);
    }
    if (progItem) progItem->setData(Qt::DisplayRole, success ? 100 : 0);
    if (spdItem)  spdItem->setText("-");
    if (etaItem)  etaItem->setText("-");

    // Save to history
    if (m_items.contains(id)) {
        auto &item = m_items[id];
        item.outputFile  = outputFile;
        item.finishedAt  = QDateTime::currentDateTime();
        item.state       = success ? DownloadState::Done : DownloadState::Error;

        HistoryEntry he;
        he.title      = item.title.isEmpty() ? item.url : item.title;
        he.url        = item.url;
        he.format     = item.format;
        he.outputFile = outputFile;
        he.platform   = PlatformDetector::name(item.platform);
        he.finishedAt = item.finishedAt;
        he.success    = success;
        HistoryDialog::addEntry(he);
    }

    updateStatusBar();
}

void MainWindow::onQueueEmpty()
{
    statusBar()->showMessage(
        QString("All done — %1 succeeded, %2 failed.")
        .arg(m_doneCount).arg(m_errorCount), 8000);

    if (m_settings.notifyDone && m_tray) {
        m_tray->showMessage("VideoSnap Pro — Downloads Complete",
            QString("%1 succeeded, %2 failed.")
            .arg(m_doneCount).arg(m_errorCount),
            QSystemTrayIcon::Information, 4000);
    }
}

// ─── SCHEDULE SLOT ───────────────────────────────────────────────────────────

void MainWindow::onScheduleDownload()
{
    QTime target = m_scheduleEdit->time();
    QTime now    = QTime::currentTime();

    // Calculate milliseconds until target time
    int msecs = now.msecsTo(target);
    if (msecs <= 0)
        msecs += 24 * 60 * 60 * 1000; // next day

    int secs = msecs / 1000;
    int h    = secs / 3600;
    int m    = (secs % 3600) / 60;
    int s    = secs % 60;

    QString countdown;
    if (h > 0) countdown = QString("%1h %2m").arg(h).arg(m);
    else if (m > 0) countdown = QString("%1m %2s").arg(m).arg(s);
    else countdown = QString("%1s").arg(s);

    m_scheduleLbl->setText(QString("Starts in %1").arg(countdown));
    m_scheduleBtn->setText("Cancel Timer");
    m_scheduleBtn->setStyleSheet("color: #e04444;");

    // Cancel any existing timer
    if (m_scheduleTimer) {
        m_scheduleTimer->stop();
        m_scheduleTimer->deleteLater();
        m_scheduleTimer = nullptr;
    }

    // Check if button says cancel
    if (m_scheduleBtn->text() == "Cancel Timer" &&
        m_scheduleLbl->text().isEmpty()) {
        m_scheduleLbl->setText("");
        m_scheduleBtn->setText("Set Timer");
        m_scheduleBtn->setStyleSheet("");
        return;
    }

    m_scheduleTimer = new QTimer(this);
    m_scheduleTimer->setSingleShot(true);

    connect(m_scheduleTimer, &QTimer::timeout, this, [this]() {
        m_scheduleLbl->setText("Starting now...");
        m_scheduleBtn->setText("Set Timer");
        m_scheduleBtn->setStyleSheet("");
        onStartAll();
        QTimer::singleShot(3000, this, [this]() { m_scheduleLbl->setText(""); });
    });

    // Update countdown every second
    QTimer *countdownTimer = new QTimer(this);
    connect(countdownTimer, &QTimer::timeout, this, [this, countdownTimer]() {
        if (!m_scheduleTimer || !m_scheduleTimer->isActive()) {
            countdownTimer->stop();
            countdownTimer->deleteLater();
            return;
        }
        int remaining = m_scheduleTimer->remainingTime() / 1000;
        if (remaining <= 0) {
            m_scheduleLbl->setText("Starting...");
            countdownTimer->stop();
            countdownTimer->deleteLater();
            return;
        }
        int h = remaining / 3600, m = (remaining % 3600) / 60, s = remaining % 60;
        QString t;
        if (h > 0) t = QString("%1h %2m").arg(h).arg(m);
        else if (m > 0) t = QString("%1m %2s").arg(m).arg(s);
        else t = QString("%1s").arg(s);
        m_scheduleLbl->setText(QString("Starts in %1").arg(t));
    });
    countdownTimer->start(1000);

    m_scheduleTimer->start(msecs);

    statusBar()->showMessage(
        QString("Downloads scheduled for %1 (in %2)")
        .arg(target.toString("HH:mm")).arg(countdown), 5000);
}

// ─── UPDATE SLOTS ─────────────────────────────────────────────────────────────

void MainWindow::onCheckForUpdates()
{
    // Manual check — show result even if already up to date
    auto *updater = new AutoUpdater(this);

    connect(updater, &AutoUpdater::updateAvailable, this,
        [this](const QString &latest, const QString &url,
               const QString &downloadUrl, const QString &notes) {
            m_lastDownloadUrl = downloadUrl;
            onUpdateAvailable(latest, url, notes);
        });

    connect(updater, &AutoUpdater::alreadyUpToDate, this, [this]() {
        QMessageBox::information(this, "No Updates Available",
            QString("You are already running the latest version (%1).")
                .arg(APP_VERSION));
    });

    connect(updater, &AutoUpdater::checkFailed, this, [this](const QString &err) {
        QMessageBox::warning(this, "Update Check Failed",
            "Could not reach GitHub to check for updates.\n\n" + err);
    });

    statusBar()->showMessage("Checking for updates...", 3000);
    updater->checkForUpdates(false);  // false = not silent
}

void MainWindow::onUpdateAvailable(const QString &latest,
                                    const QString &url,
                                    const QString &notes)
{
    QSettings s;
    if (s.value("skippedVersion").toString() == latest) return;

    // downloadUrl is passed via the signal — get it from AutoUpdater
    // We re-use m_updater which already has the download URL from the signal
    UpdateDialog dlg(APP_VERSION, latest, url,
                     m_lastDownloadUrl,   // stored when signal fires
                     notes, m_updater, this);
    dlg.exec();
}
