#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkInterface>
#include <QHostAddress>
#include <iostream>
#include <qmqtt.h>
#include <chrono>

#ifdef QT_NO_SSL
#undef QT_NO_SSL
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void UpdateStatus();

    void SubscribeClient(std::unique_ptr<QMQTT::Client> &client, bool isLocal);
    void OnLocalConnected();
    void OnAdafruitConnected();
    void OnLocalDisconnected();
    void OnAdafruitDisconnected();
    void OnLocalReceived(const QMQTT::Message &message);
    void OnAdafruitReceived(const QMQTT::Message &message);
    void SendControlValues();
    void PublishMessage(QString topic, QString payload, std::unique_ptr<QMQTT::Client> &broker, bool isLocal);
    void UpdateUI(const QMQTT::Message &message);

private:
    Ui::MainWindow *ui;

    bool localConnected = false;
    bool adafruitConnected = false;

    /*
     * MQTT
     */
    std::unique_ptr<QMQTT::Client> localClient;
    std::unique_ptr<QMQTT::Client> adafruitClient;
    const QString mqttLocalHost{"192.168.0.18"};
    const QString mqttLocalUser{"embarcados"};

    const QString mqttAdafruitHost{"io.adafruit.com"};
    const QString mqttAdafruitUser{"vrocha"};
    const QString mqttAdafruitDashboard{"server-monitoring-tool"};

    const quint16 mqttPort = 1883;

    std::chrono::steady_clock::time_point lastSendToAdafruit;
};
#endif // MAINWINDOW_H
