#include <QApplication>
#include "mainwindow.h"
#include "thememanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("VideoSnapPro");
    app.setApplicationVersion("2.0.0");
    app.setOrganizationName("VideoSnapPro");
    app.setStyle("Fusion");

    ThemeManager::instance().loadSaved();

    MainWindow w;
    w.show();
    return app.exec();
}
