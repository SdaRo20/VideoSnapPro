#include "downloadqueue.h"
#include <QRegularExpression>

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
    m_pausedItems.remove(id);

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
    m_pausedItems.clear();
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

// ─── Per-item pause ──────────────────────────────────────────────────────────
// We can't truly suspend a running yt-dlp process mid-download cleanly, so
// "pausing" an item means:
//  - If it's still pending (not started yet): pull it out of the queue and
//    hold it in m_pausedItems so tryStartNext() skips it.
//  - If it's already active/downloading: abort the worker (yt-dlp leaves a
//    .part file behind) and stash the item so resumeItem() can re-enqueue it;
//    yt-dlp will pick up the .part file and continue from where it left off.
void DownloadQueue::pauseItem(int id)
{
    if (m_pausedItems.contains(id)) return;

    if (m_active.contains(id)) {
        DownloadItem snapshot = m_active[id].item;
        m_pausedItems.insert(id, snapshot);
        m_active[id].worker->abort();
        // onWorkerFinished will fire (aborted); we intercept it there and
        // avoid emitting a failure for paused items.
    } else {
        QQueue<DownloadItem> remaining;
        while (!m_pending.isEmpty()) {
            auto it = m_pending.dequeue();
            if (it.id == id) m_pausedItems.insert(id, it);
            else remaining.enqueue(it);
        }
        m_pending = remaining;
        if (m_pausedItems.contains(id))
            emit itemProgress(id, m_pausedItems[id].progress, "", "",
                               "Paused", DownloadState::Aborted);
    }
}

void DownloadQueue::resumeItem(int id)
{
    if (!m_pausedItems.contains(id)) return;
    DownloadItem item = m_pausedItems.take(id);
    m_pending.enqueue(item);
    if (!m_paused) tryStartNext();
}

// ─── Retry a failed item ─────────────────────────────────────────────────────
void DownloadQueue::retryItem(const DownloadItem &item)
{
    m_pausedItems.remove(item.id);
    DownloadItem copy = item;
    copy.state    = DownloadState::Queued;
    copy.progress = 0;
    copy.errorMsg.clear();
    m_pending.enqueue(copy);
    if (!m_paused) tryStartNext();
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

        m_active.insert(item.id, {thread, worker, item});
        thread->start();
    }
}

void DownloadQueue::onWorkerProgress(int id, int percent, const QString &speed,
    const QString &eta, const QString &status, DownloadState state)
{
    if (m_active.contains(id)) {
        m_active[id].item.progress = qMax(0, percent);
        m_active[id].item.state    = state;
    }

    if (percent >= 0 && !speed.isEmpty())
        m_currentSpeeds[id] = parseSpeedToBps(speed);
    else
        m_currentSpeeds.remove(id);

    recomputeTotalSpeed();
    emit itemProgress(id, percent, speed, eta, status, state);
}

void DownloadQueue::onWorkerFinished(int id, bool success,
    const QString &errorMsg, const QString &outputFile)
{
    m_active.remove(id);
    m_currentSpeeds.remove(id);
    recomputeTotalSpeed();

    // If this item was aborted because the user paused it, don't report
    // failure — it's sitting in m_pausedItems waiting to be resumed.
    if (m_pausedItems.contains(id)) {
        tryStartNext();
        return;
    }

    emit itemFinished(id, success, errorMsg, outputFile);
    tryStartNext();
    if (m_active.isEmpty() && m_pending.isEmpty() && m_pausedItems.isEmpty())
        emit queueEmpty();
}

// ─── Speed helpers ────────────────────────────────────────────────────────────
qint64 DownloadQueue::parseSpeedToBps(const QString &speed)
{
    // Speed strings look like "5.23MiB/s", "812.4KiB/s", "1.2GiB/s"
    static QRegularExpression re(R"(([\d\.]+)\s*([KMG]?i?B)/s)",
                                  QRegularExpression::CaseInsensitiveOption);
    auto m = re.match(speed);
    if (!m.hasMatch()) return 0;

    double val = m.captured(1).toDouble();
    QString unit = m.captured(2).toUpper();

    double mult = 1.0;
    if (unit.startsWith("K")) mult = 1024.0;
    else if (unit.startsWith("M")) mult = 1024.0 * 1024.0;
    else if (unit.startsWith("G")) mult = 1024.0 * 1024.0 * 1024.0;

    return (qint64)(val * mult);
}

void DownloadQueue::recomputeTotalSpeed()
{
    qint64 total = 0;
    for (auto v : m_currentSpeeds) total += v;
    m_totalSpeedBps = total;
}
