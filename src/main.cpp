#include <QApplication>
#include <QIcon>
#include "mainwindow.h"
#include "thememanager.h"
#include "splashscreen.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("VideoSnapPro");
    app.setApplicationVersion("2.1.4");
    app.setOrganizationName("VideoSnapPro");
    app.setStyle("Fusion");

    // Set app icon (shows in taskbar, title bar, Alt+Tab)
    app.setWindowIcon(QIcon(":/resources/icon.png"));

    ThemeManager::instance().loadSaved();

    // Show splash screen
    SplashScreen *splash = new SplashScreen();
    splash->startLoading();

    // Create main window but keep hidden until splash finishes
    MainWindow *w = new MainWindow();

    QObject::connect(splash, &SplashScreen::loadingFinished, [&]() {
        splash->hide();
        w->show();
        w->raise();
        w->activateWindow();
        splash->deleteLater();
    });

    return app.exec();
}
