#include <QApplication>
#include "ServerWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("NPOKalibriTest Server");
    app.setApplicationVersion("0.0.1");
    app.setOrganizationName("NPOKalibriTest");

    ServerWindow window;
    window.showMaximized();

    return app.exec();
}
