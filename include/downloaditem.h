#pragma once
#include <QString>
#include <QDateTime>
#include <QPixmap>

enum class DownloadState {
    Queued, Fetching, Downloading, Converting, Done, Error, Aborted
};

enum class Platform {
    YouTube, TikTok, Instagram, Twitter, Facebook, Vimeo, Other
};

struct DownloadItem {
    int           id          = 0;
    QString       url;
    QString       format;        // best, 4K, 1440p, 1080p, 720p, 480p, 360p, mp3, aac, flac, wav, opus
    QString       title;
    QString       savePath;
    QString       ffmpegPath  = "ffmpeg";  // path to ffmpeg binary
    QString       thumbnailUrl;
    QPixmap       thumbnail;
    QString       duration;
    QString       fileSize;
    QString       speed;
    QString       eta;
    QString       statusText   = "Queued";
    QString       errorMsg;
    QString       outputFile;
    DownloadState state        = DownloadState::Queued;
    Platform      platform     = Platform::Other;
    int           progress     = 0;
    bool          isPlaylist   = false;
    int           playlistIdx  = 0;
    int           playlistTotal= 0;
    QDateTime     addedAt;
    QDateTime     finishedAt;
    qint64        speedLimit   = 0; // bytes/sec, 0 = unlimited
};

// Roles for QTableWidget items
enum ExtraRole {
    ItemIdRole   = Qt::UserRole + 1,
    StateRole    = Qt::UserRole + 2,
    PlatformRole = Qt::UserRole + 3
};
