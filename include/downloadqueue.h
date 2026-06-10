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
    int  activeCount()  const { return m_active.size(); }
    int  queuedCount()  const { return m_pending.size(); }
    bool isPaused()     const { return m_paused; }

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

    struct Slot {
        QThread        *thread = nullptr;
        DownloadWorker *worker = nullptr;
    };

    int                  m_maxConcurrent;
    bool                 m_paused = false;
    QQueue<DownloadItem> m_pending;
    QMap<int, Slot>      m_active;
};
