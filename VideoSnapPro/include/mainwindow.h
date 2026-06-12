#pragma once
#include <QMainWindow>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMap>
#include <QTimer>

#include "downloadqueue.h"
#include "metafetcher.h"
#include "settingsdialog.h"
#include "downloaditem.h"
#include "autoupdater.h"
#include "scraperdialog.h"
#include <QTimeEdit>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *e) override;
    void changeEvent(QEvent *e) override;

private slots:
    void onAddLinks();
    void onStartAll();
    void onPauseResume();
    void onClearDone();
    void onAbortAll();
    void onOpenSettings();
    void onOpenHistory();
    void onOpenConverter();
    void onOpenScraper();
    void onToggleTheme();
    void onTrayActivated(QSystemTrayIcon::ActivationReason);
    void onCheckForUpdates();   // manual: Help menu
    void onUpdateAvailable(const QString &latest, const QString &url, const QString &notes);
    void onScheduleDownload();

    void onMetaReady(int id, const QString &title, const QString &duration,
                     const QString &thumbnailUrl, const QString &fileSize,
                     bool isPlaylist, int plCount);
    void onMetaFailed(int id, const QString &reason);
    void onThumbnailReady(int id, const QPixmap &pix);

    void onItemProgress(int id, int percent, const QString &speed,
                        const QString &eta, const QString &status,
                        DownloadState state);
    void onItemFinished(int id, bool success, const QString &errorMsg,
                        const QString &outputFile);
    void onQueueEmpty();

    void onRemoveSelectedRows();
    void onOpenOutputFile();
    void onCopyUrl();

private:
    void setupUi();
    void setupTray();
    void setupMenuBar();
    void applySettings();
    int  addTableRow(const DownloadItem &item);
    int  findRowById(int id) const;
    void updateStatusBar();
    void updateRow(int row, const DownloadItem &item);
    void saveWindowState();
    void restoreWindowState();
    QString shortUrl(const QString &url) const;

    // Toolbar / input area
    QTextEdit   *m_urlInput     = nullptr;
    QComboBox   *m_formatCombo  = nullptr;
    QPushButton *m_addBtn       = nullptr;
    QPushButton *m_startBtn     = nullptr;
    QPushButton *m_pauseBtn     = nullptr;
    QPushButton *m_clearBtn     = nullptr;
    QPushButton *m_abortBtn     = nullptr;
    QPushButton *m_themeBtn     = nullptr;
    QLabel      *m_statusLabel  = nullptr;
    QLabel      *m_speedLabel   = nullptr;

    QTableWidget    *m_table    = nullptr;
    DownloadQueue   *m_queue    = nullptr;
    MetaFetcher     *m_fetcher  = nullptr;
    QSystemTrayIcon *m_tray     = nullptr;
    QMenu           *m_trayMenu = nullptr;
    AutoUpdater     *m_updater  = nullptr;

    AppSettings m_settings;

    int  m_nextId      = 1;
    int  m_doneCount   = 0;
    int  m_errorCount  = 0;
    bool m_paused      = false;

    // id -> full DownloadItem (for history)
    QMap<int, DownloadItem> m_items;

    QString m_lastDownloadUrl;  // stored from updateAvailable signal

    // Speed aggregation timer
    QTimer    *m_speedTimer    = nullptr;
    QTimeEdit *m_scheduleEdit  = nullptr;
    QPushButton *m_scheduleBtn = nullptr;
    QLabel    *m_scheduleLbl   = nullptr;
    QTimer    *m_scheduleTimer = nullptr;
    QMap<int, QString> m_currentSpeeds;
};
