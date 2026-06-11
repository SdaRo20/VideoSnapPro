#pragma once
#include <QWidget>
#include <QTimer>
#include <QProgressBar>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class SplashScreen : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float progress READ progress WRITE setProgress)

public:
    explicit SplashScreen(QWidget *parent = nullptr);

    float progress() const { return m_progress; }
    void  setProgress(float v);

    // Call this to start the loading animation
    void startLoading();

signals:
    void loadingFinished();

private slots:
    void onTick();

private:
    void paintEvent(QPaintEvent *) override;
    void drawBackground(QPainter &p);
    void drawLogo(QPainter &p);
    void drawProgressBar(QPainter &p);
    void drawText(QPainter &p);

    QTimer *m_timer    = nullptr;
    float   m_progress = 0.0f;   // 0.0 to 1.0
    float   m_targetProgress = 0.0f;
    int     m_tick     = 0;
    float   m_glowAnim = 0.0f;

    // Loading steps shown to user
    static const QStringList s_steps;
    int m_stepIdx = 0;
};
