#pragma once
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QDateTime>

struct ScrapedVideo {
    QString id;
    QString url;
    QString title;
    QString duration;
    QString thumbnail;
    QString uploader;
    QString platform;
    qint64  viewCount = 0;
};

struct ScrapeJob {
    QString sourceUrl;
    QString platform;
    int     maxVideos  = 100;
    QString filterType; // "all", "videos", "shorts", "reels", "streams"
};

class ScraperEngine : public QObject
{
    Q_OBJECT
public:
    explicit ScraperEngine(QObject *parent = nullptr);
    ~ScraperEngine();

    void startScrape(const ScrapeJob &job);
    void abort();
    bool isRunning() const;

    static QString detectPlatform(const QString &url);
    static bool    isProfileUrl(const QString &url);
    static QString normalizeUrl(const QString &url, const QString &filterType);

signals:
    void videoFound(const ScrapedVideo &video);
    void progress(int found, int total, const QString &status);
    void finished(int totalFound, bool aborted);
    void errorOccurred(const QString &msg);

private slots:
    void onReadyRead();
    void onStderrReady();
    void onProcessFinished(int code, QProcess::ExitStatus status);

private:
    QProcess *m_proc    = nullptr;
    bool      m_aborted = false;
    int       m_found   = 0;
    int       m_maxVideos = 0;
    QString   m_platform;
    QByteArray m_buffer; // line buffer
};
