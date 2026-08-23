#include "DeviceClient.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QThread>

DeviceClient::DeviceClient(const QString &host, quint16 port, QObject *parent)
    : QObject(parent)
    , m_host(host)
    , m_port(port)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &DeviceClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &DeviceClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &DeviceClient::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &DeviceClient::onErrorOccurred);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(5000);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DeviceClient::onReconnectTimeout);

    m_sendTimer = new QTimer(this);
    m_sendTimer->setSingleShot(true);  
    connect(m_sendTimer, &QTimer::timeout, this, &DeviceClient::onSendTimeout);

    m_startTimeMs = QDateTime::currentMSecsSinceEpoch();
}

DeviceClient::~DeviceClient()
{
    m_sendTimer->stop();
    m_reconnectTimer->stop();
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        m_socket->waitForDisconnected(1000);
    }
}

void DeviceClient::start()
{
    qInfo() << "DeviceClient starting. Target:" << m_host << ":" << m_port;
    tryConnect();
}

void DeviceClient::setState(State newState)
{
    if (m_state == newState) {
        return;
    }
    m_state = newState;

    switch (m_state) {
    case State::Disconnected:
        qInfo() << "[STATE] Disconnected";
        break;
    case State::Connecting:
        qInfo() << "[STATE] Connecting...";
        break;
    case State::WaitingAck:
        qInfo() << "[STATE] Waiting for ConnectionAck";
        break;
    case State::Idle:
        qInfo() << "[STATE] Idle (waiting for Start command)";
        break;
    case State::Sending:
        qInfo() << "[STATE] Sending data";
        break;
    }
}

void DeviceClient::tryConnect()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        return;
    }
    setState(State::Connecting);
    m_socket->connectToHost(m_host, m_port);
}

void DeviceClient::onConnected()
{
    qInfo() << "Connected to server";
    setState(State::WaitingAck);
    m_buffer.clear();
}

void DeviceClient::onDisconnected()
{
    qWarning() << "Disconnected from server";
    m_sendTimer->stop();
    setState(State::Disconnected);
    m_clientId.clear();

    // Планируем переподключение
    if (!m_reconnectTimer->isActive()) {
        qInfo() << "Will retry connection in 5 seconds...";
        m_reconnectTimer->start();
    }
}

void DeviceClient::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    qWarning() << "Socket error:" << m_socket->errorString();

    if (m_state == State::Connecting || m_state == State::Disconnected) {
        // Обработка соедиенения 
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start();
        }
    }
}

void DeviceClient::onReconnectTimeout()
{
    if (m_state == State::Disconnected || m_state == State::Connecting) {
        tryConnect();
    }
}

void DeviceClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

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

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "Invalid JSON from server:" << err.errorString();
            continue;
        }

        processJson(doc.object());
    }
}

void DeviceClient::processJson(const QJsonObject &obj)
{
    QString type = obj.value("type").toString();

    if (type == "ConnectionAck") {
        m_clientId = obj.value("client_id").toString();
        QString status = obj.value("status").toString();
        qInfo() << "Received ConnectionAck. client_id =" << m_clientId
                << "status =" << status;
        setState(State::Idle);
    }
    else if (type == "Start") {
        qInfo() << "Received Start command → begin sending data";
        setState(State::Sending);
        // Первый пакет сразу, дальше по таймеру
        onSendTimeout();
    }
    else if (type == "Stop") {
        qInfo() << "Received Stop command → pause sending";
        m_sendTimer->stop();
        setState(State::Idle);
    }
    else if (type == "Config") {
        if (obj.contains("critical_latency"))
            m_criticalLatency = obj["critical_latency"].toDouble();
        if (obj.contains("critical_packet_loss"))
            m_criticalPacketLoss = obj["critical_packet_loss"].toDouble();
        if (obj.contains("critical_cpu"))
            m_criticalCpu = obj["critical_cpu"].toInt();

        qInfo() << "Config updated:"
                << "latency_crit =" << m_criticalLatency
                << "packet_loss_crit =" << m_criticalPacketLoss
                << "cpu_crit =" << m_criticalCpu;
    }
    else {
        qInfo() << "Unknown command from server:" << type;
    }
}

void DeviceClient::sendJson(const QJsonObject &obj)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');
    m_socket->write(data);
    m_socket->flush();
}

void DeviceClient::onSendTimeout()
{
    if (m_state != State::Sending) {
        return;
    }

    generateAndSendPacket();

    // задержка 10–100 мс
    int delayMs = QRandomGenerator::global()->bounded(10, 101);
    m_sendTimer->start(delayMs);
}

void DeviceClient::generateAndSendPacket()
{
    int r = QRandomGenerator::global()->bounded(100);
    QJsonObject packet;

    if (r < 40) {
        packet = createNetworkMetrics();
    } else if (r < 80) {
        packet = createDeviceStatus();
    } else {
        packet = createLog();
    }

    sendJson(packet);

    
    QString type = packet["type"].toString();
    if (type == "Log") {
        qInfo().noquote() << "TX Log [" << packet["severity"].toString() << "]:"
                          << packet["message"].toString().left(80);
    } else {
        qInfo().noquote() << "TX" << type;
    }
}

QJsonObject DeviceClient::createNetworkMetrics() const
{
    // Случайные реалистичные значения
    double bandwidth = QRandomGenerator::global()->bounded(50, 200);   // Mbps
    double latency = QRandomGenerator::global()->bounded(5, 80);       // ms
    double packetLoss = QRandomGenerator::global()->generateDouble() * (0.08);

    QJsonObject obj{
        {"type", "NetworkMetrics"},
        {"bandwidth", qRound(bandwidth * 10) / 10.0},
        {"latency", qRound(latency * 10) / 10.0},
        {"packet_loss", qRound(packetLoss * 10000) / 10000.0}
    };

    // Если превысили порог — дополнительно отправим warning Log (но в этом пакете не делаем)
    // Предупреждение генерируется в createLog при необходимости
    return obj;
}

QJsonObject DeviceClient::createDeviceStatus() const
{
    qint64 uptimeSec = (QDateTime::currentMSecsSinceEpoch() - m_startTimeMs) / 1000;
    int cpu = QRandomGenerator::global()->bounded(5, 95);
    int mem = QRandomGenerator::global()->bounded(20, 90);

    return QJsonObject{
        {"type", "DeviceStatus"},
        {"uptime", static_cast<int>(uptimeSec)},
        {"cpu_usage", cpu},
        {"memory_usage", mem}
    };
}

QJsonObject DeviceClient::createLog() const
{
    // Иногда генерируем warning при «превышении» порогов
    bool forceWarning = (QRandomGenerator::global()->bounded(100) < 15);

    QString severity = "INFO";
    QString message;

    if (forceWarning) {
        severity = "WARNING";
        // Имитация превышения порогов
        int kind = QRandomGenerator::global()->bounded(3);
        if (kind == 0) {
            message = QString("Latency exceeded critical threshold (%.1f ms > %.1f ms)")
                          .arg(m_criticalLatency + 10.0 + QRandomGenerator::global()-> generateDouble() * (40.0 - 10.0))
                          .arg(m_criticalLatency);
        } else if (kind == 1) {
            message = QString("Packet loss above limit (%.3f > %.3f)")
                          .arg(m_criticalPacketLoss + 0.02)
                          .arg(m_criticalPacketLoss);
        } else {
            message = QString("CPU usage critical: %1% (threshold %2%)")
                          .arg(m_criticalCpu + QRandomGenerator::global()->bounded(5, 20))
                          .arg(m_criticalCpu);
        }
    } else {
        // Обычные логи 
        bool longMsg = (QRandomGenerator::global()->bounded(100) < 25);
        message = randomLogMessage(longMsg);
        severity = (QRandomGenerator::global()->bounded(100) < 10) ? "DEBUG" : "INFO";
    }

    return QJsonObject{
        {"type", "Log"},
        {"message", message},
        {"severity", severity}
    };
}

QString DeviceClient::randomLogMessage(bool longMessage) const
{
    static const QStringList shortMsgs = {
        "Interface eth0 link up",
        "DHCP lease renewed",
        "ARP table updated",
        "NTP sync completed",
        "Watchdog OK",
        "Temperature normal",
        "Fan speed adjusted",
        "Config reload successful"
    };

    static const QStringList mediumMsgs = {
        "Interface eth0 restarted after carrier loss detection",
        "Packet queue overflow on interface eth1, dropped 12 packets",
        "Neighbor discovery: new device 192.168.1.45 detected",
        "QoS policy applied to traffic class video",
        "SNMP trap sent: linkDown for ifIndex 3"
    };

    if (!longMessage) {
        if (QRandomGenerator::global()->bounded(100) < 70) {
            return shortMsgs[QRandomGenerator::global()->bounded(shortMsgs.size())];
        }
        return mediumMsgs[QRandomGenerator::global()->bounded(mediumMsgs.size())];
    }

    // Длинное сообщение (>200 символов)
    QString longMsg =
        "LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOONG"
        "MESSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSAGEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE "
        "HEREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
        "REEEEEEEEEEEEEEEEEEEEEEEEEEALLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLYYYYYYYYYYYYYYYY"
        "VERRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRYYYYYYYYYYYYYYYYYYYYYYYYYY"
        "LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOONG"
        "BYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYEEEEEEEEEEEEEEEE";
    return longMsg;
}
