#include "downloadqueue.h"

DownloadQueue::DownloadQueue(int maxConcurrent, QObject *parent)
    : QObject(parent), m_maxConcurrent(maxConcurrent)
{}

DownloadQueue::~DownloadQueue() { abortAll(); }

void DownloadQueue::setMaxConcurrent(int n)
{
    m_maxConcurrent = qBound(1, n, 10);
    tryStartNext();
}

void DownloadQueue::enqueue(const DownloadItem &item)
{
    m_pending.enqueue(item);
    if (!m_paused) tryStartNext();
}

void DownloadQueue::abortItem(int id)
{
    if (m_active.contains(id)) {
        m_active[id].worker->abort();
    } else {
        QQueue<DownloadItem> remaining;
        while (!m_pending.isEmpty()) {
            auto it = m_pending.dequeue();
            if (it.id != id) remaining.enqueue(it);
        }
        m_pending = remaining;
    }
}

void DownloadQueue::abortAll()
{
    m_pending.clear();
    for (auto &slot : m_active)
        slot.worker->abort();
}

void DownloadQueue::pauseQueue()
{
    m_paused = true;
}

void DownloadQueue::resumeQueue()
{
    m_paused = false;
    tryStartNext();
}

void DownloadQueue::tryStartNext()
{
    while (!m_paused &&
           m_active.size() < m_maxConcurrent &&
           !m_pending.isEmpty())
    {
        DownloadItem item = m_pending.dequeue();
        auto *thread = new QThread(this);
        auto *worker = new DownloadWorker(item);
        worker->moveToThread(thread);

        connect(thread, &QThread::started,   worker, &DownloadWorker::run);
        connect(worker, &DownloadWorker::progressChanged,
                this,   &DownloadQueue::onWorkerProgress);
        connect(worker, &DownloadWorker::finished,
                this,   &DownloadQueue::onWorkerFinished);
        connect(worker, &DownloadWorker::finished, thread, &QThread::quit);
        connect(thread, &QThread::finished,  worker, &QObject::deleteLater);
        connect(thread, &QThread::finished,  thread, &QObject::deleteLater);

        m_active.insert(item.id, {thread, worker});
        thread->start();
    }
}

void DownloadQueue::onWorkerProgress(int id, int percent, const QString &speed,
    const QString &eta, const QString &status, DownloadState state)
{
    emit itemProgress(id, percent, speed, eta, status, state);
}

void DownloadQueue::onWorkerFinished(int id, bool success,
    const QString &errorMsg, const QString &outputFile)
{
    m_active.remove(id);
    emit itemFinished(id, success, errorMsg, outputFile);
    tryStartNext();
    if (m_active.isEmpty() && m_pending.isEmpty())
        emit queueEmpty();
}
