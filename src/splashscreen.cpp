#include "splashscreen.h"
#include "autoupdater.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QScreen>
#include <QGuiApplication>
#include <QtMath>

const QStringList SplashScreen::s_steps = {
    "Initialising application...",
    "Loading settings...",
    "Checking dependencies...",
    "Preparing download engine...",
    "Almost ready..."
};

SplashScreen::SplashScreen(QWidget *parent)
    : QWidget(parent)
{
    // Frameless, always on top, no taskbar entry
    setWindowFlags(Qt::FramelessWindowHint |
                   Qt::WindowStaysOnTopHint |
                   Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(480, 300);

    // Center on screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect sg = screen->geometry();
        move(sg.center() - rect().center());
    }

    m_timer = new QTimer(this);
    m_timer->setInterval(30); // ~33fps
    connect(m_timer, &QTimer::timeout, this, &SplashScreen::onTick);
}

void SplashScreen::startLoading()
{
    m_timer->start();
    show();
}

void SplashScreen::setProgress(float v)
{
    m_progress = qBound(0.0f, v, 1.0f);
    update();
}

void SplashScreen::onTick()
{
    m_tick++;
    m_glowAnim = (qSin(m_tick * 0.05f) + 1.0f) / 2.0f; // 0..1 pulsing

    // Smoothly advance progress toward target
    float speed = 0.008f;
    if (m_progress < m_targetProgress)
        m_progress = qMin(m_progress + speed, m_targetProgress);

    // Update loading step text
    m_stepIdx = qMin((int)(m_progress * s_steps.size()), s_steps.size() - 1);

    // Schedule progress milestones
    if (m_tick == 20)  m_targetProgress = 0.20f;
    if (m_tick == 50)  m_targetProgress = 0.45f;
    if (m_tick == 80)  m_targetProgress = 0.65f;
    if (m_tick == 110) m_targetProgress = 0.85f;
    if (m_tick == 140) m_targetProgress = 0.97f;
    if (m_tick == 165) {
        m_targetProgress = 1.0f;
        m_progress       = 1.0f;
    }
    if (m_tick == 175) {
        m_timer->stop();
        emit loadingFinished();
        return;
    }

    update();
}

// ─── Paint ────────────────────────────────────────────────────────────────────

void SplashScreen::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    drawBackground(p);
    drawLogo(p);
    drawProgressBar(p);
    drawText(p);
}

void SplashScreen::drawBackground(QPainter &p)
{
    QRectF r = rect();

    // Rounded card with drop shadow simulation
    // Shadow layers
    for (int i = 12; i > 0; --i) {
        float a = (12 - i) * 3.0f;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, (int)a));
        p.drawRoundedRect(r.adjusted(i, i, -i + 6, -i + 6), 18, 18);
    }

    // Main card background — dark gradient
    QLinearGradient bg(0, 0, 0, r.height());
    bg.setColorAt(0.0, QColor(22, 22, 28));
    bg.setColorAt(1.0, QColor(12, 12, 16));
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(r.adjusted(6, 6, -6, -6), 16, 16);

    // Subtle top border highlight
    p.setPen(QPen(QColor(255, 255, 255, 18), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r.adjusted(6, 6, -6, -6), 16, 16);

    // Accent glow top-left
    QRadialGradient glow(r.width() * 0.25, r.height() * 0.1, r.width() * 0.55);
    float ga = 0.08f + m_glowAnim * 0.06f;
    glow.setColorAt(0.0, QColor(212, 168, 75, (int)(ga * 255)));
    glow.setColorAt(1.0, Qt::transparent);
    p.setBrush(glow);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(r.adjusted(6, 6, -6, -6), 16, 16);
}

void SplashScreen::drawLogo(QPainter &p)
{
    QRectF r = rect().adjusted(6, 6, -6, -6);
    float cx = r.left() + r.width() / 2.0f;

    // Icon circle
    float iconSize = 56;
    float iconX    = cx - iconSize / 2;
    float iconY    = r.top() + 28;

    QRectF iconRect(iconX, iconY, iconSize, iconSize);

    // Icon background circle
    QRadialGradient iconBg(iconRect.center(), iconSize / 2);
    iconBg.setColorAt(0.0, QColor(212, 168, 75));
    iconBg.setColorAt(1.0, QColor(180, 130, 40));
    p.setBrush(iconBg);
    p.setPen(Qt::NoPen);
    p.drawEllipse(iconRect);

    // Draw a download arrow icon using paths (no emoji)
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(20, 20, 20));

    // Arrow shaft (vertical rectangle)
    float arrowCx = iconRect.center().x();
    float arrowCy = iconRect.center().y();
    QRectF shaft(arrowCx - 4, arrowCy - 14, 8, 18);
    p.drawRoundedRect(shaft, 2, 2);

    // Arrow head (triangle pointing down)
    QPainterPath arrowHead;
    arrowHead.moveTo(arrowCx - 11, arrowCy + 4);
    arrowHead.lineTo(arrowCx + 11, arrowCy + 4);
    arrowHead.lineTo(arrowCx,      arrowCy + 17);
    arrowHead.closeSubpath();
    p.drawPath(arrowHead);

    // Bottom bar (download tray)
    QRectF tray(arrowCx - 13, arrowCy + 18, 26, 4);
    p.drawRoundedRect(tray, 2, 2);

    // App name
    p.setPen(QColor(220, 220, 220));
    QFont nameFont("Segoe UI", 18, QFont::Bold);
    p.setFont(nameFont);
    QRectF nameRect(r.left(), iconY + iconSize + 12, r.width(), 30);
    p.drawText(nameRect, Qt::AlignHCenter | Qt::AlignVCenter, "VideoSnap Pro");

    // Version
    p.setPen(QColor(150, 150, 150));
    QFont verFont("Segoe UI", 9);
    p.setFont(verFont);
    QRectF verRect(r.left(), iconY + iconSize + 44, r.width(), 18);
    p.drawText(verRect, Qt::AlignHCenter | Qt::AlignVCenter,
               QString("v%1").arg(APP_VERSION));

    // Tagline
    p.setPen(QColor(100, 100, 100));
    QFont tagFont("Segoe UI", 8);
    p.setFont(tagFont);
    QRectF tagRect(r.left(), iconY + iconSize + 62, r.width(), 16);
    p.drawText(tagRect, Qt::AlignHCenter | Qt::AlignVCenter,
               "Batch Downloader  •  Powered by yt-dlp");
}

void SplashScreen::drawProgressBar(QPainter &p)
{
    QRectF r = rect().adjusted(6, 6, -6, -6);

    float barW  = r.width() - 60;
    float barH  = 5;
    float barX  = r.left() + 30;
    float barY  = r.bottom() - 42;

    // Track
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(40, 40, 50));
    p.drawRoundedRect(QRectF(barX, barY, barW, barH), 3, 3);

    // Fill
    float fillW = barW * m_progress;
    if (fillW > 1) {
        // Gradient fill
        QLinearGradient fill(barX, 0, barX + barW, 0);
        fill.setColorAt(0.0, QColor(180, 130, 40));
        fill.setColorAt(1.0, QColor(212, 168, 75));
        p.setBrush(fill);
        p.drawRoundedRect(QRectF(barX, barY, fillW, barH), 3, 3);

        // Glow at fill edge
        if (fillW > 8) {
            QRadialGradient glow(barX + fillW, barY + barH / 2, 12);
            float ga = 0.5f + m_glowAnim * 0.5f;
            glow.setColorAt(0.0, QColor(255, 210, 100, (int)(ga * 180)));
            glow.setColorAt(1.0, Qt::transparent);
            p.setBrush(glow);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(barX + fillW, barY + barH / 2), 12, 12);
        }
    }

    // Percentage
    p.setPen(QColor(150, 150, 150));
    QFont pctFont("Segoe UI", 8);
    p.setFont(pctFont);
    p.drawText(QRectF(barX + barW + 8, barY - 2, 36, 12),
               Qt::AlignLeft | Qt::AlignVCenter,
               QString("%1%").arg((int)(m_progress * 100)));
}

void SplashScreen::drawText(QPainter &p)
{
    QRectF r = rect().adjusted(6, 6, -6, -6);

    // Step text
    QString step = (m_stepIdx < s_steps.size())
                   ? s_steps[m_stepIdx] : "Ready";

    p.setPen(QColor(120, 120, 130));
    QFont stepFont("Segoe UI", 8);
    p.setFont(stepFont);
    p.drawText(QRectF(r.left() + 30, r.bottom() - 28, r.width() - 60, 16),
               Qt::AlignLeft | Qt::AlignVCenter, step);

    // Right side: dots animation
    int dots = (m_tick / 15) % 4;
    QString dotsStr = QString(".").repeated(dots);
    p.drawText(QRectF(r.left() + 30, r.bottom() - 28, r.width() - 60, 16),
               Qt::AlignRight | Qt::AlignVCenter, dotsStr);
}
