#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QHostAddress>

/**
 * @brief Обёртка над одним клиентским QTcpSocket.
 */
class ClientSession : public QObject
{
    Q_OBJECT
public:
    explicit ClientSession(QTcpSocket *socket, const QString &clientId, QObject *parent = nullptr);
    ~ClientSession() override;

    QString clientId() const { return m_clientId; }
    QString peerAddress() const;
    quint16 peerPort() const;
    bool isConnected() const;

    void sendJson(const QJsonObject &obj);
    void disconnectFromHost();

signals:
    void dataReceived(const QString &clientId, const QJsonObject &data);
    void disconnected(const QString &clientId);
    void errorOccurred(const QString &clientId, const QString &errorString);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QTcpSocket *m_socket = nullptr;
    QString m_clientId;
    QByteArray m_buffer;
};

#endif 
