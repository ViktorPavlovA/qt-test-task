#include "ServerWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QMessageBox>
#include <QFormLayout>
#include <QSplitter>
#include <QStatusBar>

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("NPO Kalibri Server — Управление устройствами");
    showMaximized();

    setupUi();
    createNetworkThread();
}

ServerWindow::~ServerWindow()
{
    destroyNetworkThread();
}

void ServerWindow::setupUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);

    // кнопки
    auto *btnLayout = new QHBoxLayout;
    m_btnStartServer  = new QPushButton("Запустить сервер", this);
    m_btnStopServer   = new QPushButton("Остановить сервер", this);
    m_btnStartClients = new QPushButton("Старт клиентов", this);
    m_btnStopClients  = new QPushButton("Стоп клиентов", this);
    m_btnSendConfig   = new QPushButton("Отправить конфигурацию", this);

    m_btnStopServer->setEnabled(false);
    m_btnStartClients->setEnabled(false);
    m_btnStopClients->setEnabled(false);
    m_btnSendConfig->setEnabled(false);

    btnLayout->addWidget(m_btnStartServer);
    btnLayout->addWidget(m_btnStopServer);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(m_btnStartClients);
    btnLayout->addWidget(m_btnStopClients);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(m_btnSendConfig);
    btnLayout->addStretch();

    mainLayout->addLayout(btnLayout);

    // Добавляем формы
    auto *settingsGroup = new QGroupBox("Критические пороги (отправляются клиентам)", this);
    auto *form = new QFormLayout(settingsGroup);

    m_spinCriticalLatency = new QDoubleSpinBox(this);
    m_spinCriticalLatency->setRange(1.0, 1000.0);
    m_spinCriticalLatency->setValue(50.0);
    m_spinCriticalLatency->setSuffix(" ms");

    m_spinCriticalPacketLoss = new QDoubleSpinBox(this);
    m_spinCriticalPacketLoss->setRange(0.0, 1.0);
    m_spinCriticalPacketLoss->setDecimals(3);
    m_spinCriticalPacketLoss->setSingleStep(0.01);
    m_spinCriticalPacketLoss->setValue(0.05);

    m_spinCriticalCpu = new QSpinBox(this);
    m_spinCriticalCpu->setRange(1, 100);
    m_spinCriticalCpu->setValue(80);
    m_spinCriticalCpu->setSuffix(" %");

    form->addRow("Крит. latency:", m_spinCriticalLatency);
    form->addRow("Крит. packet_loss:", m_spinCriticalPacketLoss);
    form->addRow("Крит. CPU usage:", m_spinCriticalCpu);

    mainLayout->addWidget(settingsGroup);

    auto *splitter = new QSplitter(Qt::Vertical, this);

    // добавляем таблицу подключения 
    auto *clientsWidget = new QWidget;
    auto *clientsLayout = new QVBoxLayout(clientsWidget);
    clientsLayout->addWidget(new QLabel("<b>Подключённые клиенты</b>"));
    m_clientsTable = new QTableWidget(0, 4, this);
    m_clientsTable->setHorizontalHeaderLabels({"Client ID", "IP", "Port", "Status"});
    m_clientsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_clientsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_clientsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    clientsLayout->addWidget(m_clientsTable);
    splitter->addWidget(clientsWidget);

    // Таблица данных 
    auto *dataWidget = new QWidget;
    auto *dataLayout = new QVBoxLayout(dataWidget);
    dataLayout->addWidget(new QLabel("<b>Полученные данные</b>"));
    m_dataTable = new QTableWidget(0, 4, this);
    m_dataTable->setHorizontalHeaderLabels({"Client ID", "Type", "Content", "Time"});
    m_dataTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_dataTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_dataTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_dataTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dataTable->setWordWrap(true);
    dataLayout->addWidget(m_dataTable);
    splitter->addWidget(dataWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);

    // Логи
    mainLayout->addWidget(new QLabel("<b>Лог событий</b>"));
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(160);
    mainLayout->addWidget(m_logEdit);

    // Статус сервера
    m_statusLabel = new QLabel("Сервер остановлен", this);
    statusBar()->addWidget(m_statusLabel);

    // Подключение кнопок
    connect(m_btnStartServer, &QPushButton::clicked, this, &ServerWindow::onStartServerClicked);
    connect(m_btnStopServer, &QPushButton::clicked, this, &ServerWindow::onStopServerClicked);
    connect(m_btnStartClients, &QPushButton::clicked, this, &ServerWindow::onStartClientsClicked);
    connect(m_btnStopClients, &QPushButton::clicked, this, &ServerWindow::onStopClientsClicked);
    connect(m_btnSendConfig, &QPushButton::clicked, this, &ServerWindow::onSendConfigClicked);
}

void ServerWindow::createNetworkThread()
{
    m_networkThread = new QThread(this);
    m_networkServer = new NetworkServer();      
    m_networkServer->moveToThread(m_networkThread);

    connect(m_networkServer, &NetworkServer::started,
            this, &ServerWindow::onServerStarted);
    connect(m_networkServer, &NetworkServer::stopped,
            this, &ServerWindow::onServerStopped);
    connect(m_networkServer, &NetworkServer::clientConnected,
            this, &ServerWindow::onClientConnected);
    connect(m_networkServer, &NetworkServer::clientDisconnected,
            this, &ServerWindow::onClientDisconnected);
    connect(m_networkServer, &NetworkServer::dataReceived,
            this, &ServerWindow::onDataReceived);
    connect(m_networkServer, &NetworkServer::logMessage,
            this, &ServerWindow::onLogMessage);
    connect(m_networkServer, &NetworkServer::errorOccurred,
            this, &ServerWindow::onServerError);

    connect(m_networkThread, &QThread::finished, m_networkServer, &QObject::deleteLater);

    m_networkThread->start();
}

void ServerWindow::destroyNetworkThread()
{
    if (m_networkServer) {
        // Останавливаем сервер в его потоке
        QMetaObject::invokeMethod(m_networkServer, "stop", Qt::BlockingQueuedConnection);
    }

    if (m_networkThread) {
        m_networkThread->quit();
        m_networkThread->wait(3000);
        m_networkThread = nullptr;
        m_networkServer = nullptr;
    }
}

void ServerWindow::onStartServerClicked()
{
    QMetaObject::invokeMethod(m_networkServer, "start",
                              Qt::QueuedConnection,
                              Q_ARG(quint16, 12345));
}

void ServerWindow::onStopServerClicked()
{
    QMetaObject::invokeMethod(m_networkServer, "stop", Qt::QueuedConnection);
}

void ServerWindow::onStartClientsClicked()
{
    QMetaObject::invokeMethod(m_networkServer, "sendStartToAll", Qt::QueuedConnection);
}

void ServerWindow::onStopClientsClicked()
{
    QMetaObject::invokeMethod(m_networkServer, "sendStopToAll", Qt::QueuedConnection);
}

void ServerWindow::onSendConfigClicked()
{
    QJsonObject config{
        {"critical_latency", m_spinCriticalLatency->value()},
        {"critical_packet_loss", m_spinCriticalPacketLoss->value()},
        {"critical_cpu", m_spinCriticalCpu->value()}
    };
    QMetaObject::invokeMethod(m_networkServer, "sendConfigToAll",
                              Qt::QueuedConnection,
                              Q_ARG(QJsonObject, config));
}

void ServerWindow::onServerStarted(quint16 port)
{
    m_btnStartServer->setEnabled(false);
    m_btnStopServer->setEnabled(true);
    m_btnStartClients->setEnabled(true);
    m_btnStopClients->setEnabled(true);
    m_btnSendConfig->setEnabled(true);
    m_statusLabel->setText(QString("Сервер работает на порту %1").arg(port));
    appendLog(QString("=== Сервер запущен на порту %1 ===").arg(port));
}

void ServerWindow::onServerStopped()
{
    m_btnStartServer->setEnabled(true);
    m_btnStopServer->setEnabled(false);
    m_btnStartClients->setEnabled(false);
    m_btnStopClients->setEnabled(false);
    m_btnSendConfig->setEnabled(false);

    // Очищаем таблицу клиентов
    m_clientsTable->setRowCount(0);
    m_clientRows.clear();

    m_statusLabel->setText("Сервер остановлен");
    appendLog("=== Сервер остановлен ===");
}

void ServerWindow::onClientConnected(const QString &clientId, const QString &ip, quint16 port)
{
    int row = m_clientsTable->rowCount();
    m_clientsTable->insertRow(row);
    m_clientsTable->setItem(row, 0, new QTableWidgetItem(clientId));
    m_clientsTable->setItem(row, 1, new QTableWidgetItem(ip));
    m_clientsTable->setItem(row, 2, new QTableWidgetItem(QString::number(port)));
    m_clientsTable->setItem(row, 3, new QTableWidgetItem("Подключён"));

    m_clientRows[clientId] = row;
}

void ServerWindow::onClientDisconnected(const QString &clientId)
{
    if (!m_clientRows.contains(clientId)) {
        return;
    }
    int row = m_clientRows[clientId];
    // Помечаем как отключённый (не удаляем сразу, чтобы было видно)
    if (row < m_clientsTable->rowCount()) {
        m_clientsTable->setItem(row, 3, new QTableWidgetItem("Отключён"));
    }
    m_clientRows.remove(clientId);
}

void ServerWindow::onDataReceived(const QString &clientId, const QJsonObject &data)
{
    QString type = data.value("type").toString("Unknown");
    QString content = formatJsonContent(data);
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    int row = m_dataTable->rowCount();
    m_dataTable->insertRow(row);
    m_dataTable->setItem(row, 0, new QTableWidgetItem(clientId));
    m_dataTable->setItem(row, 1, new QTableWidgetItem(type));
    m_dataTable->setItem(row, 2, new QTableWidgetItem(content));
    m_dataTable->setItem(row, 3, new QTableWidgetItem(time));

    // Ограничиваем размер таблицы (последние 500 записей)
    if (m_dataTable->rowCount() > 500) {
        m_dataTable->removeRow(0);
    }

    // Автопрокрутка вниз
    m_dataTable->scrollToBottom();
}

void ServerWindow::onLogMessage(const QString &message)
{
    appendLog(message);
}

void ServerWindow::onServerError(const QString &message)
{
    appendLog("[ERROR] " + message);
    QMessageBox::warning(this, "Ошибка сервера", message);
}

QString ServerWindow::formatJsonContent(const QJsonObject &obj) const
{
    QString type = obj.value("type").toString();
    QStringList parts;

    if (type == "NetworkMetrics") {
        if (obj.contains("bandwidth"))
            parts << QString("bandwidth=%1").arg(obj["bandwidth"].toDouble(), 0, 'f', 1);
        if (obj.contains("latency"))
            parts << QString("latency=%1 ms").arg(obj["latency"].toDouble(), 0, 'f', 1);
        if (obj.contains("packet_loss"))
            parts << QString("packet_loss=%1").arg(obj["packet_loss"].toDouble(), 0, 'f', 4);
    } else if (type == "DeviceStatus") {
        if (obj.contains("uptime"))
            parts << QString("uptime=%1 s").arg(obj["uptime"].toInt());
        if (obj.contains("cpu_usage"))
            parts << QString("cpu=%1%").arg(obj["cpu_usage"].toInt());
        if (obj.contains("memory_usage"))
            parts << QString("mem=%1%").arg(obj["memory_usage"].toInt());
    } else if (type == "Log") {
        QString level = obj.value("severity").toString("INFO");
        QString msg = obj.value("message").toString();
        parts << QString("[%1] %2").arg(level, msg);
    } else {
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            if (it.key() == "type") continue;
            parts << QString("%1=%2").arg(it.key(), it.value().toVariant().toString());
        }
    }

    return parts.join(" | ");
}

void ServerWindow::appendLog(const QString &text)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logEdit->append(QString("[%1] %2").arg(timestamp, text));
}
