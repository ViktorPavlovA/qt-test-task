#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QThread>
#include <QJsonObject>
#include "NetworkServer.h"

/**
 * @brief Главное окно сервера.
 */
class ServerWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow() override;

private slots:

    // слоты для кнопок
    void onStartServerClicked();
    void onStopServerClicked();
    void onStartClientsClicked();
    void onStopClientsClicked();
    void onSendConfigClicked();

    // слоты от NetworkServer
    void onServerStarted(quint16 port);
    void onServerStopped();
    void onClientConnected(const QString &clientId, const QString &ip, quint16 port);
    void onClientDisconnected(const QString &clientId);
    void onDataReceived(const QString &clientId, const QJsonObject &data);
    void onLogMessage(const QString &message);
    void onServerError(const QString &message);

private:
    void setupUi();
    void createNetworkThread();
    void destroyNetworkThread();
    QString formatJsonContent(const QJsonObject &obj) const;
    void appendLog(const QString &text);

    // графика
    QTableWidget *m_clientsTable = nullptr;
    QTableWidget *m_dataTable = nullptr;
    QTextEdit *m_logEdit = nullptr;

    QPushButton *m_btnStartServer = nullptr;
    QPushButton *m_btnStopServer = nullptr;
    QPushButton *m_btnStartClients = nullptr;
    QPushButton *m_btnStopClients = nullptr;
    QPushButton *m_btnSendConfig = nullptr;

    QDoubleSpinBox *m_spinCriticalLatency = nullptr;
    QDoubleSpinBox *m_spinCriticalPacketLoss = nullptr;
    QSpinBox *m_spinCriticalCpu = nullptr;

    QLabel *m_statusLabel = nullptr;

    QThread *m_networkThread = nullptr;
    NetworkServer *m_networkServer = nullptr;

    // задаем QMap
    QMap<QString, int> m_clientRows;
};

#endif 
