#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QMap>
#include <QJsonObject>
#include "ClientSession.h"

/**
 * @brief Сетевой сервер, работающий в отдельном потоке.
 *
 * Владеет QTcpServer и всеми ClientSession.
 * Все методы, вызываемые из GUI, должны быть слотами (QueuedConnection).
 */
class NetworkServer : public QObject
{
    Q_OBJECT
public:
    explicit NetworkServer(QObject *parent = nullptr);
    ~NetworkServer() override;

public slots:
    void start(quint16 port = 12345);
    void stop();

    void sendStartToAll();
    void sendStopToAll();
    void sendConfigToAll(const QJsonObject &config);
    void sendToClient(const QString &clientId, const QJsonObject &obj);

signals:
    void started(quint16 port);
    void stopped();
    void clientConnected(const QString &clientId, const QString &ip, quint16 port);
    void clientDisconnected(const QString &clientId);
    void dataReceived(const QString &clientId, const QJsonObject &data);
    void errorOccurred(const QString &message);
    void logMessage(const QString &message);

private slots:
    void onNewConnection();
    void onClientData(const QString &clientId, const QJsonObject &data);
    void onClientDisconnected(const QString &clientId);
    void onClientError(const QString &clientId, const QString &errorString);

private:
    QString generateClientId();

    QTcpServer *m_tcpServer = nullptr;
    QMap<QString, ClientSession*> m_clients;   // clientId -> session
    int m_nextId = 1;
    bool m_running = false;
};

#endif // NETWORKSERVER_H
