#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Window | Qt::MSWindowsFixedSizeDialogHint);

    /*
     * MQTT
     */
    localClient = std::make_unique<QMQTT::Client>();
    adafruitClient = std::make_unique<QMQTT::Client>();

    localClient->setHostName(mqttLocalHost);
    localClient->setPort(mqttPort);
    localClient->setUsername(mqttLocalUser);
    localClient->setPassword(mqttLocalUser.toLatin1());
    localClient->connectToHost();
    connect(&(*localClient), &QMQTT::Client::connected, this, &MainWindow::OnLocalConnected);
    connect(&(*localClient), &QMQTT::Client::disconnected, this, &MainWindow::OnLocalDisconnected);

    adafruitClient->setHostName(mqttAdafruitHost);
    adafruitClient->setPort(mqttPort);
    adafruitClient->setClientId("esp01");
    adafruitClient->setUsername(mqttAdafruitUser);
    adafruitClient->setPassword("aio_ugcz75Q1EvXQAqvgVY9WaqpEwN1S");
    adafruitClient->connectToHost();
    connect(&(*adafruitClient), &QMQTT::Client::connected, this, &MainWindow::OnAdafruitConnected);
    connect(&(*adafruitClient), &QMQTT::Client::disconnected, this, &MainWindow::OnAdafruitDisconnected);

    UpdateStatus();

    connect(ui->sliderMotor, &QSlider::valueChanged, this, &MainWindow::SendControlValues);
    connect(ui->rdLeftToRight, &QRadioButton::toggled, this, &MainWindow::SendControlValues);
    connect(ui->rdRed, &QRadioButton::toggled, this, &MainWindow::SendControlValues);
    connect(ui->rdGreen, &QRadioButton::toggled, this, &MainWindow::SendControlValues);

    lastSendToAdafruit = std::chrono::steady_clock::now();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::UpdateStatus()
{
    if (localConnected)
    {
        ui->lblLocalStatus->setText("Connected");
        ui->lblLocalStatus->setStyleSheet("color: green;");
    }
    else
    {
        ui->lblLocalStatus->setText("Disconnected");
        ui->lblLocalStatus->setStyleSheet("color: red;");
    }

    if (adafruitConnected)
    {
        ui->lblAdafruitStatus->setText("Connected");
        ui->lblAdafruitStatus->setStyleSheet("color: green;");
    }
    else
    {
        ui->lblAdafruitStatus->setText("Disconnected");
        ui->lblAdafruitStatus->setStyleSheet("color: red;");
    }
}

void MainWindow::OnLocalConnected()
{
    localConnected = true;
    connect(&(*localClient), &QMQTT::Client::received, this, &MainWindow::OnLocalReceived);
    SubscribeClient(localClient, true);
    UpdateStatus();
}

void MainWindow::OnAdafruitConnected()
{
    adafruitConnected = true;
    connect(&(*adafruitClient), &QMQTT::Client::received, this, &MainWindow::OnAdafruitReceived);
    SubscribeClient(adafruitClient, false);
    UpdateStatus();
}

void MainWindow::OnLocalDisconnected()
{
    localConnected = false;
    localClient->connectToHost();
    UpdateStatus();
}

void MainWindow::OnAdafruitDisconnected()
{
    adafruitConnected = false;
    adafruitClient->connectToHost();
    UpdateStatus();
}

void MainWindow::OnLocalReceived(const QMQTT::Message &message)
{
    UpdateUI(message);

    auto now = std::chrono::steady_clock::now();

    // publica no adafruit no intervalo minimo de 1 segundo
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastSendToAdafruit).count() < 2)
        return;

    lastSendToAdafruit = std::chrono::steady_clock::now();

    // publish into Adafruit broker
    if (adafruitConnected)
    {
        QMQTT::Message adafruitMessage;
        QString adafruitTopic = mqttAdafruitUser + QString::fromUtf8("/feeds/") + mqttAdafruitDashboard + QString::fromUtf8(".") + message.topic();
        adafruitMessage.setTopic(adafruitTopic);
        adafruitMessage.setPayload(message.payload());
        adafruitClient->publish(adafruitMessage);
    }
}

void MainWindow::OnAdafruitReceived(const QMQTT::Message &message)
{
    UpdateUI(message);

    // publish into Local broker
    if (localConnected)
    {
        QMQTT::Message localMessage;
        //TODO montar o topic
        QString topic = "";
        localMessage.setTopic(topic);
        localMessage.setPayload(message.payload());
        localClient->publish(message);
    }
}

void MainWindow::SendControlValues()
{
    auto servo_direction = QString::number(ui->sliderMotor->value());

    QString led_direction;
    if (ui->rdLeftToRight->isChecked())
        led_direction = "0";
    else
        led_direction = "1";

    QString led_color;
    if (ui->rdRed->isChecked())
        led_color = "0";
    else if (ui->rdGreen->isChecked())
        led_color = "1";
    else
        led_color = "2";

    if (localConnected)
    {
        // publish values to local broker
        PublishMessage("servo-direction", servo_direction, localClient, true);
        PublishMessage("led-direction", led_direction, localClient, true);
        PublishMessage("led-color", led_color, localClient, true);
    }
    if (adafruitConnected)
    {
        // publish values to adafruit broker
        PublishMessage("servo-direction", servo_direction, adafruitClient, false);
        PublishMessage("led-direction", led_direction, adafruitClient, false);
        PublishMessage("led-color", led_color, adafruitClient, false);
    }
}

void MainWindow::PublishMessage(QString topic, QString payload, std::unique_ptr<QMQTT::Client> &broker, bool isLocal)
{
    QMQTT::Message message;
    QString adafruitTopic = mqttAdafruitUser + QString::fromUtf8("/feeds/") + mqttAdafruitDashboard + QString::fromUtf8(".") + topic;
    if (isLocal)
        message.setTopic(topic);
    else
        message.setTopic(adafruitTopic);
    message.setPayload(payload.toLatin1());
    broker->publish(message);
}

void MainWindow::UpdateUI(const QMQTT::Message &message)
{
    QString str = message.payload();
    auto topic = message.topic();
    if (topic.contains("temperature"))
    {
        str += " ºC";
        ui->lcdTemp->display(str);
    }
    else if (topic.contains("pressure"))
    {
        str += " hPa";
        ui->lcdPress->display(str);
    }
    else if (topic.contains("humidity"))
    {
        str += " %";
        ui->lcdHumid->display(str);
    }
    else if (topic.contains("distance"))
    {
        str += " cm";
        ui->lcdDist->display(str);
    }
    else if (topic.contains("servo-direction"))
    {
        ui->sliderMotor->setValue(str.toUInt());
    }
    else if (topic.contains("led-color"))
    {
        switch (str.toUInt())
        {
        case 0:
            ui->rdRed->setChecked(true);
            break;
        case 1:
            ui->rdGreen->setChecked(true);
            break;
        case 2:
            ui->rdBlue->setChecked(true);
            break;
        default:
            ui->rdRed->setChecked(true);
            qDebug() << "[ERROR]: invalid payload for led-color: " << str;
            break;
        }
    }
    else // led-direction
    {
        switch (str.toUInt())
        {
        case 0:
            ui->rdLeftToRight->setChecked(true);
            break;
        case 1:
            ui->rdRightToLeft->setChecked(true);
            break;
        default:
            ui->rdLeftToRight->setChecked(true);
            qDebug() << "[ERROR]: invalid payload for led-direction: " << str;
            break;
        }
    }
}

void MainWindow::SubscribeClient(std::unique_ptr<QMQTT::Client> &client, bool isLocal)
{
    QString adafruitTopic = mqttAdafruitUser + QString::fromUtf8("/feeds/") + mqttAdafruitDashboard + QString::fromUtf8(".");
    QString topics[] = {"temperature", "pressure", "humidity", "distance", "servo-direction", "led-color", "led-direction" };

    for (auto &topic : topics)
    {
        if (isLocal)
            client->subscribe(topic);
        else
            client->subscribe(adafruitTopic + topic);
    }
}
