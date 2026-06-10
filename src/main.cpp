#include <QApplication>
#include "mainwindow.h"
#include "thememanager.h"
#include "splashscreen.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("VideoSnapPro");
    app.setApplicationVersion("2.1.2");
    app.setOrganizationName("VideoSnapPro");
    app.setStyle("Fusion");

    ThemeManager::instance().loadSaved();

    // Show splash screen
    SplashScreen *splash = new SplashScreen();
    splash->startLoading();

    // Create main window but keep it hidden
    MainWindow *w = new MainWindow();

    // When splash finishes loading → hide splash, show main window
    QObject::connect(splash, &SplashScreen::loadingFinished, [&]() {
        splash->hide();
        w->show();
        w->raise();
        w->activateWindow();
        splash->deleteLater();
    });

    return app.exec();
}
