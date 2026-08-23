#ifndef DEVICECLIENT_H
#define DEVICECLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRandomGenerator>

/**
 * @brief Консольный клиент — эмулятор сетевого устройства.
 *
 * Состояния:
 *  Disconnected → Connecting → WaitingAck → Idle → Sending
 *
 * При потере соединения автоматически пытается переподключиться каждые 5 секунд.
 */
class DeviceClient : public QObject
{
    Q_OBJECT
public:
    explicit DeviceClient(const QString &host = "127.0.0.1",
                          quint16 port = 12345,
                          QObject *parent = nullptr);
    ~DeviceClient() override;

    void start();

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onReconnectTimeout();
    void onSendTimeout();

private:
    enum class State {
        Disconnected,
        Connecting,
        WaitingAck,
        Idle,          // подключён, ждём Start
        Sending        // активно шлём данные
    };

    void setState(State newState);
    void tryConnect();
    void processJson(const QJsonObject &obj);
    void sendJson(const QJsonObject &obj);
    void generateAndSendPacket();

    QJsonObject createNetworkMetrics() const;
    QJsonObject createDeviceStatus() const;
    QJsonObject createLog() const;
    QString randomLogMessage(bool longMessage) const;

    QTcpSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QTimer *m_sendTimer = nullptr;

    QString m_host;
    quint16 m_port;
    QString m_clientId;          // получен из ConnectionAck
    State m_state = State::Disconnected;
    QByteArray m_buffer;

    // Конфигурация критических порогов (от сервера)
    double m_criticalLatency = 50.0;
    double m_criticalPacketLoss = 0.05;
    int m_criticalCpu = 80;

    // Для генерации uptime
    qint64 m_startTimeMs = 0;
};

#endif // DEVICECLIENT_H
