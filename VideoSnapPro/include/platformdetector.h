#pragma once
#include <QString>
#include "downloaditem.h"

class PlatformDetector {
public:
    static Platform detect(const QString &url) {
        if (url.contains("youtube.com") || url.contains("youtu.be") ||
            url.contains("youtube.com/shorts/"))
            return Platform::YouTube;
        if (url.contains("tiktok.com"))
            return Platform::TikTok;
        if (url.contains("instagram.com"))
            return Platform::Instagram;
        if (url.contains("twitter.com") || url.contains("x.com"))
            return Platform::Twitter;
        if (url.contains("facebook.com") || url.contains("fb.watch"))
            return Platform::Facebook;
        if (url.contains("vimeo.com"))
            return Platform::Vimeo;
        return Platform::Other;
    }

    static QString icon(Platform p) {
        switch (p) {
            case Platform::YouTube:   return "▶";
            case Platform::TikTok:    return "♪";
            case Platform::Instagram: return "📷";
            case Platform::Twitter:   return "𝕏";
            case Platform::Facebook:  return "f";
            case Platform::Vimeo:     return "V";
            default:                  return "⬇";
        }
    }

    static QString name(Platform p) {
        switch (p) {
            case Platform::YouTube:   return "YouTube";
            case Platform::TikTok:    return "TikTok";
            case Platform::Instagram: return "Instagram";
            case Platform::Twitter:   return "Twitter/X";
            case Platform::Facebook:  return "Facebook";
            case Platform::Vimeo:     return "Vimeo";
            default:                  return "Other";
        }
    }

    static QString color(Platform p) {
        switch (p) {
            case Platform::YouTube:   return "#ff4444";
            case Platform::TikTok:    return "#69c9d0";
            case Platform::Instagram: return "#e1306c";
            case Platform::Twitter:   return "#1d9bf0";
            case Platform::Facebook:  return "#1877f2";
            case Platform::Vimeo:     return "#1ab7ea";
            default:                  return "#888888";
        }
    }

    static bool isPlaylistUrl(const QString &url) {
        // Shorts are NOT playlists — single video
        if (url.contains("/shorts/")) return false;
        return url.contains("list=") || url.contains("/playlist/") ||
               url.contains("/sets/");
    }
};
