#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->
    setWindowFlags(Qt::Window | Qt::MSWindowsFixedSizeDialogHint);

    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &MainWindow::NewConnection);
    if (!server->listen(QHostAddress::Any, port))
    {
        qDebug("Server could not start!");
        exit(1);
    }

    foreach (const QHostAddress &address, QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
            ip = address.toString();
    }

    UpdateStatus();

    connect(ui->sliderMotor, &QSlider::valueChanged, this, &MainWindow::SendControlValues);
    connect(ui->rdLeftToRight, &QRadioButton::toggled, this, &MainWindow::SendControlValues);
    connect(ui->rdRed, &QRadioButton::toggled, this, &MainWindow::SendControlValues);
    connect(ui->rdGreen, &QRadioButton::toggled, this, &MainWindow::SendControlValues);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (clientConnected)
    {
        socket->close();
    }
    server->close();
}

void MainWindow::NewConnection()
{
    if (clientConnected)
        return;

    socket = server->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::ReadMonitoringValues);
    connect(socket,&QTcpSocket::stateChanged, this, [&](QTcpSocket::SocketState state)
    {
        if (state == QTcpSocket::ConnectedState)
        {
            clientConnected = true;
            SendControlValues();
        }
        else if (clientConnected && state != QTcpSocket::ConnectedState)
        {
            clientConnected = false;
            socket->close();
        }
        UpdateStatus();
    });

    clientConnected = true;
    UpdateStatus();
    SendControlValues();
}

void MainWindow::SendControlValues()
{
    /*
     * send to client control information from UI
     */
    if (!clientConnected)
        return;
    /*
     * update struct with new values from UI
     */
    sprintf((char*)&cv.servoDirection, "%d", ui->sliderMotor->value());

    if (ui->rdLeftToRight->isChecked())
        strcpy((char*)&cv.LEDDirection, "0");
    else
        strcpy((char*)&cv.LEDDirection, "1");

    if (ui->rdRed->isChecked())
        strcpy((char*)&cv.LEDColor, "0");
    else if (ui->rdGreen->isChecked())
        strcpy((char*)&cv.LEDColor, "1");
    else
        strcpy((char*)&cv.LEDColor, "2");

    socket->write((char*)&cv, sizeof cv);
    socket->flush();

    socket->waitForBytesWritten(30);
}

void MainWindow::ReadMonitoringValues()
{
    /*
     * read monitoring values from ESP8266
     */
    socket->read((char*)&mv, sizeof mv);

    /*
     * update ui
     */
    QString temp = mv.T;
    temp.append(" C");
    ui->lcdTemp->display(temp);
    temp = mv.H;
    temp.append(" %");
    ui->lcdUmid->display(temp);
    temp = mv.P;
    temp.append(" hPa");
    ui->lcdPress->display(temp);
    temp = mv.D;
    temp.append(" cm");
    ui->lcdDist->display(temp);
}

void MainWindow::UpdateStatus()
{
    if (clientConnected)
    {
        ui->lblStatus->setText("Connected");
        ui->lblStatus->setStyleSheet("color: green;");
    }
    else
    {
        ui->lblStatus->setText("Disconnected");
        ui->lblStatus->setStyleSheet("color: red;");
    }
}
