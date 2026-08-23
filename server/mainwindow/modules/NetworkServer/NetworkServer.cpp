#include "NetworkServer.h"
#include <QDebug>

NetworkServer::NetworkServer(QObject *parent)
    : QObject(parent)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &NetworkServer::onNewConnection);
}

NetworkServer::~NetworkServer()
{
    stop();
}

void NetworkServer::start(quint16 port)
{
    if (m_running) {
        return;
    }

    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        emit errorOccurred(QString("Cannot listen on port %1: %2")
                           .arg(port)
                           .arg(m_tcpServer->errorString()));
        return;
    }

    m_running = true;
    emit started(port);
    emit logMessage(QString("Server started on port %1").arg(port));
}

void NetworkServer::stop()
{
    if (!m_running) {
        return;
    }

    // Корректно закрываем всех клиентов
    const auto keys = m_clients.keys();
    for (const QString &id : keys) {
        if (m_clients.contains(id)) {
            m_clients[id]->disconnectFromHost();
            m_clients[id]->deleteLater();
        }
    }
    m_clients.clear();

    m_tcpServer->close();
    m_running = false;
    emit stopped();
    emit logMessage("Server stopped");
}

void NetworkServer::sendStartToAll()
{
    QJsonObject cmd{{"type", "Start"}};
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it) {
        it.value()->sendJson(cmd);
    }
    emit logMessage(QString("Start command sent to %1 clients").arg(m_clients.size()));
}

void NetworkServer::sendStopToAll()
{
    QJsonObject cmd{{"type", "Stop"}};
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it) {
        it.value()->sendJson(cmd);
    }
    emit logMessage(QString("Stop command sent to %1 clients").arg(m_clients.size()));
}

void NetworkServer::sendConfigToAll(const QJsonObject &config)
{
    QJsonObject cmd = config;
    cmd["type"] = "Config";
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it) {
        it.value()->sendJson(cmd);
    }
    emit logMessage(QString("Config sent to %1 clients").arg(m_clients.size()));
}

void NetworkServer::sendToClient(const QString &clientId, const QJsonObject &obj)
{
    if (m_clients.contains(clientId)) {
        m_clients[clientId]->sendJson(obj);
    }
}

void NetworkServer::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket *socket = m_tcpServer->nextPendingConnection();
        if (!socket) {
            continue;
        }

        QString clientId = generateClientId();
        ClientSession *session = new ClientSession(socket, clientId, this);

        m_clients.insert(clientId, session);

        connect(session, &ClientSession::dataReceived,
                this, &NetworkServer::onClientData);
        connect(session, &ClientSession::disconnected,
                this, &NetworkServer::onClientDisconnected);
        connect(session, &ClientSession::errorOccurred,
                this, &NetworkServer::onClientError);

        // Отправляем подтверждение подключения
        QJsonObject ack{
            {"type", "ConnectionAck"},
            {"client_id", clientId},
            {"status", "ok"}
        };
        session->sendJson(ack);

        emit clientConnected(clientId, session->peerAddress(), session->peerPort());
        emit logMessage(QString("Client connected: %1 (%2:%3)")
                        .arg(clientId)
                        .arg(session->peerAddress())
                        .arg(session->peerPort()));
    }
}

void NetworkServer::onClientData(const QString &clientId, const QJsonObject &data)
{
    emit dataReceived(clientId, data);
}

void NetworkServer::onClientDisconnected(const QString &clientId)
{
    if (m_clients.contains(clientId)) {
        m_clients[clientId]->deleteLater();
        m_clients.remove(clientId);
    }
    emit clientDisconnected(clientId);
    emit logMessage(QString("Client disconnected: %1").arg(clientId));
}

void NetworkServer::onClientError(const QString &clientId, const QString &errorString)
{
    emit logMessage(QString("Error from %1: %2").arg(clientId, errorString));
}

QString NetworkServer::generateClientId()
{
    return QString("client-%1").arg(m_nextId++);
}
