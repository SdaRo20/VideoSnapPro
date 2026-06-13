#pragma once
#include <QString>
#include <QRegularExpression>
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

    // ── Profile / channel URL detection across all supported platforms ───────
    static bool isProfileUrl(const QString &url) {
        QString u = url.trimmed();

        // YouTube channel/user/handle pages (not a single video/short/live)
        if (u.contains("youtube.com/@"))       return true;
        if (u.contains("youtube.com/channel/") && !u.contains("/watch")) return true;
        if (u.contains("youtube.com/user/") && !u.contains("/watch"))    return true;
        if (u.contains("youtube.com/c/") && !u.contains("/watch"))       return true;

        // TikTok profile: tiktok.com/@username (no trailing /video/...)
        static QRegularExpression ttRe(R"(tiktok\.com/@[^/?\s]+/?$)");
        if (ttRe.match(u).hasMatch()) return true;

        // Instagram profile root: instagram.com/username (excluding posts/reels/etc.)
        static QRegularExpression igRe(R"(^https?://(?:www\.)?instagram\.com/([^/?]+)/?$)");
        auto igM = igRe.match(u);
        if (igM.hasMatch()) {
            QString seg = igM.captured(1);
            static const QStringList reserved = {
                "p", "reel", "reels", "stories", "explore", "tv"
            };
            if (!reserved.contains(seg)) return true;
        }

        // Twitter/X profile: x.com/username or twitter.com/username
        // (excluding /status/, /i/, /search, /home, /messages, etc.)
        static QRegularExpression twRe(
            R"(^https?://(?:www\.)?(?:twitter\.com|x\.com)/([^/?]+)/?$)");
        auto twM = twRe.match(u);
        if (twM.hasMatch()) {
            QString seg = twM.captured(1).toLower();
            static const QStringList reserved = {
                "i", "home", "search", "explore", "messages", "notifications",
                "settings", "compose", "status"
            };
            if (!reserved.contains(seg)) return true;
        }

        // Facebook profile/page: facebook.com/username or /pages/...
        // (excluding direct video links like /watch/ or /videos/<id> or /reel/<id>)
        if (u.contains("facebook.com")) {
            if (u.contains("/videos/") || u.contains("/watch/") ||
                u.contains("/reel/") || u.contains("watch?v="))
                return false;
            static QRegularExpression fbRe(
                R"(^https?://(?:www\.)?facebook\.com/([^/?]+)/?$)");
            auto fbM = fbRe.match(u);
            if (fbM.hasMatch()) {
                QString seg = fbM.captured(1).toLower();
                static const QStringList reserved = {
                    "watch", "groups", "marketplace", "events", "search"
                };
                if (!reserved.contains(seg)) return true;
            }
        }

        // Vimeo profile/channel: vimeo.com/username (a video URL is
        // vimeo.com/<numeric id>, so a non-numeric path segment is a profile)
        static QRegularExpression vimRe(R"(^https?://(?:www\.)?vimeo\.com/([^/?]+)/?$)");
        auto vimM = vimRe.match(u);
        if (vimM.hasMatch()) {
            QString seg = vimM.captured(1);
            static QRegularExpression numeric(R"(^\d+$)");
            if (!numeric.match(seg).hasMatch() && seg.toLower() != "channels" &&
                seg.toLower() != "groups")
                return true;
        }

        return false;
    }
};
