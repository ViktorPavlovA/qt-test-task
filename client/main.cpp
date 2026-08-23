#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include "DeviceClient.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("NPOKalibri_Client");
    app.setApplicationVersion("0.0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Эмулятор сетевого устройства (клиент телеком-системы)");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption hostOption(QStringList() << "H" << "host",
                                  "Адрес сервера", "host", "127.0.0.1");
    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  "Порт сервера", "port", "12345");
    parser.addOption(hostOption);
    parser.addOption(portOption);
    parser.process(app);

    QString host = parser.value(hostOption);
    quint16 port = static_cast<quint16>(parser.value(portOption).toUInt());

    qInfo() << "=== Telecom Device Client ===";
    qInfo() << "Connecting to" << host << ":" << port;

    DeviceClient client(host, port);
    client.start();

    return app.exec();
}
