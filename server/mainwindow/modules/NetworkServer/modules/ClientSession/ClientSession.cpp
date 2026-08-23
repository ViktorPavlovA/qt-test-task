#include "ClientSession.h"
#include <QDebug>

ClientSession::ClientSession(QTcpSocket *socket, const QString &clientId, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_clientId(clientId)
{
    if (!m_socket) {
        qWarning() << "ClientSession: null socket for" << clientId;
        return;
    }

    m_socket->setParent(this);   // ownership

    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &ClientSession::onErrorOccurred);
}

ClientSession::~ClientSession()
{
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(500);
        }
    }
}

QString ClientSession::peerAddress() const
{
    return m_socket ? m_socket->peerAddress().toString() : QString();
}

quint16 ClientSession::peerPort() const
{
    return m_socket ? m_socket->peerPort() : 0;
}

bool ClientSession::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void ClientSession::sendJson(const QJsonObject &obj)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');

    m_socket->write(data);
    m_socket->flush();
}

void ClientSession::disconnectFromHost()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
    }
}

void ClientSession::onReadyRead()
{
    if (!m_socket) {
        return;
    }

    m_buffer.append(m_socket->readAll());

    // Line-delimited JSON protocol
    while (true) {
        int idx = m_buffer.indexOf('\n');
        if (idx < 0) {
            break;
        }

        QByteArray line = m_buffer.left(idx).trimmed();
        m_buffer.remove(0, idx + 1);

        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
        
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "ClientSession" << m_clientId
                       << ": invalid JSON:" << parseError.errorString()
                       << "raw:" << line.left(100);
            emit errorOccurred(m_clientId,
                               QString("Invalid JSON: %1").arg(parseError.errorString()));
            continue;
        }

        emit dataReceived(m_clientId, doc.object());
    }
}

void ClientSession::onDisconnected()
{
    emit disconnected(m_clientId);
}

void ClientSession::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    if (m_socket) {
        emit errorOccurred(m_clientId, m_socket->errorString());
    }
}
