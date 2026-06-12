#pragma once
#include <QObject>
#include <QThread>
#include <QProcess>
#include "downloaditem.h"

class DownloadWorker : public QObject
{
    Q_OBJECT
public:
    explicit DownloadWorker(DownloadItem item, QObject *parent = nullptr);

public slots:
    void run();
    void abort();

signals:
    void progressChanged(int id, int percent, const QString &speed,
                         const QString &eta, const QString &status,
                         DownloadState state);
    void finished(int id, bool success, const QString &errorMsg,
                  const QString &outputFile);

private:
    DownloadItem m_item;
    QProcess    *m_proc    = nullptr;
    bool         m_aborted = false;

    QStringList buildArgs() const;
    void        parseLine(const QString &line);
    void        cleanupJunkFiles(const QString &dir);
    static QString fmtForId(const QString &fmt);
};
