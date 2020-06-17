#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <thread>
#include <iostream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct MonitoringValues {
  char P[10];
  char T[10];
  char H[10];
  char D[5];
  char flag = '-';
};

struct ControlValues {
  char servoDirection[2];
  char LEDColor[2];
  char LEDDirection[2];
  char flag = '-';
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void NewConnection();
    void ReadMonitoringValues();
    void SendControlValues();
    void UpdateStatus();

private:
    Ui::MainWindow *ui;

    QTcpServer *server;
    QTcpSocket *socket;
    const int port = 30000;

    bool clientConnected = false;
    QString ip;

    MonitoringValues mv;
    ControlValues cv;
};
#endif // MAINWINDOW_H
