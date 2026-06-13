#pragma once
#include <QObject>
#include <QThread>
#include <QMap>
#include <QQueue>
#include "downloadworker.h"
#include "downloaditem.h"

class DownloadQueue : public QObject
{
    Q_OBJECT
public:
    explicit DownloadQueue(int maxConcurrent = 3, QObject *parent = nullptr);
    ~DownloadQueue();

    void setMaxConcurrent(int n);
    int  maxConcurrent() const { return m_maxConcurrent; }
    void enqueue(const DownloadItem &item);
    void abortItem(int id);
    void abortAll();
    void pauseQueue();
    void resumeQueue();
    void pauseItem(int id);     // pause a single active/queued item
    void resumeItem(int id);    // resume a single paused item
    void retryItem(const DownloadItem &item); // re-enqueue a failed item
    bool isItemPaused(int id) const { return m_pausedItems.contains(id); }
    int  activeCount()  const { return m_active.size(); }
    int  queuedCount()  const { return m_pending.size(); }
    bool isPaused()     const { return m_paused; }
    qint64 totalSpeedBytesPerSec() const { return m_totalSpeedBps; }

signals:
    void itemProgress(int id, int percent, const QString &speed,
                      const QString &eta, const QString &status,
                      DownloadState state);
    void itemFinished(int id, bool success, const QString &errorMsg,
                      const QString &outputFile);
    void queueEmpty();

private slots:
    void onWorkerProgress(int id, int percent, const QString &speed,
                          const QString &eta, const QString &status,
                          DownloadState state);
    void onWorkerFinished(int id, bool success, const QString &errorMsg,
                          const QString &outputFile);

private:
    void tryStartNext();
    qint64 parseSpeedToBps(const QString &speed);
    void recomputeTotalSpeed();

    struct Slot {
        QThread        *thread = nullptr;
        DownloadWorker *worker = nullptr;
        DownloadItem    item;
    };

    int                  m_maxConcurrent;
    bool                 m_paused = false;
    QQueue<DownloadItem> m_pending;
    QMap<int, Slot>      m_active;
    QMap<int, DownloadItem> m_pausedItems; // id -> item snapshot, withheld from queue
    QMap<int, qint64>    m_currentSpeeds;  // id -> bytes/sec
    qint64               m_totalSpeedBps = 0;
};
