#pragma once
#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QVector>
#include "scraperengine.h"

class ScraperDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ScraperDialog(const QString &defaultSavePath,
                           const QString &defaultFormat,
                           QWidget *parent = nullptr);

    struct DownloadRequest {
        QStringList urls;
        QString     format;
        QString     savePath;
        int         threads = 3;
    };

    DownloadRequest downloadRequest() const { return m_request; }
    bool hasDownloadRequest() const { return !m_request.urls.isEmpty(); }

private slots:
    void onScrape();
    void onAbort();
    void onVideoFound(const ScrapedVideo &v);
    void onProgress(int found, int total, const QString &status);
    void onFinished(int total, bool aborted);
    void onError(const QString &msg);
    void onSelectAll();
    void onDeselectAll();
    void onSendToQueue();
    void onUrlChanged();
    void onBrowseSave();
    void onFilterChanged();

private:
    void setupUi();
    void setControlsEnabled(bool on);
    void updateSelectionLabel();

    // Left panel
    QTextEdit   *m_urlInput      = nullptr;
    QComboBox   *m_platformCombo = nullptr;
    QComboBox   *m_filterCombo   = nullptr;
    QComboBox   *m_typeCombo     = nullptr;
    QSpinBox    *m_limitSpin     = nullptr;
    QSpinBox    *m_threadSpin    = nullptr;
    QLineEdit   *m_savePath      = nullptr;
    QPushButton *m_browseBtn     = nullptr;
    QComboBox   *m_formatCombo   = nullptr;
    QPushButton *m_scrapeBtn     = nullptr;
    QPushButton *m_abortBtn      = nullptr;

    // Right panel
    QTableWidget *m_table        = nullptr;
    QLabel       *m_statusLbl    = nullptr;
    QLabel       *m_selectionLbl = nullptr;
    QLabel       *m_hintLbl      = nullptr;
    QProgressBar *m_progress     = nullptr;
    QPushButton  *m_selectAllBtn = nullptr;
    QPushButton  *m_deselectBtn  = nullptr;
    QPushButton  *m_sendBtn      = nullptr;

    // Engine & data
    ScraperEngine        *m_engine = nullptr;
    QVector<ScrapedVideo> m_videos;
    DownloadRequest       m_request;
    QString               m_defaultSavePath;
    QString               m_defaultFormat;

    // Multi-URL scrape queue — onScrape() fills this, then we process one
    // source URL at a time, accumulating results into m_table/m_videos.
    QStringList m_scrapeQueue;
    int         m_scrapeQueueTotal = 0;
    int         m_scrapeQueueDone  = 0;
    void startNextScrapeJob();

    enum Col {
        COL_CHECK = 0,
        COL_NUM   = 1,
        COL_TITLE = 2,
        COL_DUR   = 3,
        COL_VIEWS = 4,
        COL_URL   = 5,
        COL_COUNT = 6
    };
};
