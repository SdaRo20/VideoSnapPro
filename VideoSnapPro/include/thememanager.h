#pragma once
#include <QObject>
#include <QApplication>
#include <QPalette>
#include <QSettings>

class ThemeManager : public QObject {
    Q_OBJECT
public:
    enum Theme { Dark, Light };

    static ThemeManager &instance() {
        static ThemeManager inst;
        return inst;
    }

    Theme currentTheme() const { return m_theme; }

    void setTheme(Theme t) {
        m_theme = t;
        apply();
        QSettings().setValue("theme", (int)t);
        emit themeChanged(t);
    }

    void loadSaved() {
        int t = QSettings().value("theme", (int)Dark).toInt();
        m_theme = (Theme)t;
        apply();
    }

    // Colors that differ between themes
    QColor bg()       const { return m_theme==Dark ? QColor(15,15,15)   : QColor(245,245,245); }
    QColor surface()  const { return m_theme==Dark ? QColor(24,24,24)   : QColor(255,255,255); }
    QColor surface2() const { return m_theme==Dark ? QColor(30,30,30)   : QColor(235,235,235); }
    QColor border()   const { return m_theme==Dark ? QColor(42,42,42)   : QColor(210,210,210); }
    QColor text()     const { return m_theme==Dark ? QColor(220,220,220): QColor(30,30,30);    }
    QColor textMuted()const { return m_theme==Dark ? QColor(120,120,120): QColor(130,130,130); }
    QColor accent()   const { return QColor(212,168,75); }
    QColor accentText()const{ return QColor(0,0,0); }
    QColor success()  const { return QColor(80,200,80);  }
    QColor error()    const { return QColor(220,60,60);  }
    QColor warning()  const { return QColor(212,168,75); }

    QString styleSheet() const;

signals:
    void themeChanged(Theme t);

private:
    ThemeManager() {}
    Theme m_theme = Dark;

    void apply() {
        QPalette p;
        p.setColor(QPalette::Window,        bg());
        p.setColor(QPalette::WindowText,    text());
        p.setColor(QPalette::Base,          surface());
        p.setColor(QPalette::AlternateBase, surface2());
        p.setColor(QPalette::Text,          text());
        p.setColor(QPalette::Button,        surface2());
        p.setColor(QPalette::ButtonText,    text());
        p.setColor(QPalette::Highlight,     accent());
        p.setColor(QPalette::HighlightedText, accentText());
        p.setColor(QPalette::ToolTipBase,   surface2());
        p.setColor(QPalette::ToolTipText,   text());
        qApp->setPalette(p);
        qApp->setStyleSheet(styleSheet());
    }
};
