#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QCheckBox>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , sendSocket(nullptr)
    , receiveSocket(nullptr)
{
    ui->setupUi(this);

    hostIpAddress = "192.168.1.100";
    guestIpAddress = "192.168.1.10";
    send_port = 5003;  // Mesaj gönderme portu
    receive_port = 5004;  // Mesaj alma portu

    startTcpServer();

    connect(ui->ConnectButton, &QPushButton::clicked, this, &MainWindow::on_ConnectButton_clicked);
    connect(ui->DisconnectButton, &QPushButton::clicked, this, &MainWindow::on_DisconnectButton_clicked);
    connect(ui->SendMessageButton, &QPushButton::clicked, this, &MainWindow::on_SendMessageButton_clicked);
    connect(ui->checkBox, &QCheckBox::toggled, this, &MainWindow::on_checkBox_toggled);
    connect(ui->LED_1, &QPushButton::clicked, this, &MainWindow::on_LED_1_clicked);
    connect(ui->LED_2, &QPushButton::clicked, this, &MainWindow::on_LED_2_clicked);
    connect(ui->LED_3, &QPushButton::clicked, this, &MainWindow::on_LED_3_clicked);
    connect(ui->LED_4, &QPushButton::clicked, this, &MainWindow::on_LED_4_clicked);
    connect(ui->All_LEDS, &QPushButton::clicked, this, &MainWindow::on_All_LEDS_clicked);

}

void MainWindow::sendTcpMessage(const QByteArray &message)
{
    if (sendSocket && sendSocket->state() == QAbstractSocket::ConnectedState) {
        sendSocket->write(message);
        qDebug() << "TCP üzerinden mesaj gönderildi:" << message.toHex();
    } else {
        qDebug() << "TCP ana bilgisayarına bağlı değil, mesaj gönderilemiyor.";
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    if (sendSocket) {
        sendSocket->abort();
        delete sendSocket;
    }
    if (receiveSocket) {
        receiveSocket->abort();
        delete receiveSocket;
    }
}

void MainWindow::startTcpServer()
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::onClientConnected);

    if (!tcpServer->listen(QHostAddress::AnyIPv4, receive_port)) {
        qDebug() << "TCP sunucusu başlatılamadı!";
    } else {
        qDebug() << "TCP sunucusu başlatıldı, port üzerinde dinleniyor" << receive_port;
    }
}

void MainWindow::onClientConnected()
{
    while (tcpServer->hasPendingConnections()) {
        receiveSocket = tcpServer->nextPendingConnection();
        connect(receiveSocket, &QTcpSocket::disconnected, this, &MainWindow::onSocketDisconnected);
        connect(receiveSocket, &QTcpSocket::readyRead, this, &MainWindow::onTCP_ReadyRead);

        qDebug() << "Client bağlandı!";
    }
}


connect(ui->DisconnectButton, &QPushButton::clicked, this, &MainWindow::on_DisconnectButton_clicked);
void MainWindow::onSocketDisconnected()
{
    if (receiveSocket) {
        qDebug() << "TCP sunucusundan bağlantı kesildi";
        receiveSocket->deleteLater();
        receiveSocket = nullptr;
    }
}

void MainWindow::onSocketStateChanged()
{
    if (!sendSocket)
        return;

    qDebug() << "TCP Soket durumu değişti:" << sendSocket->state();
    switch (sendSocket->state()) {
        case QAbstractSocket::ConnectingState:
            qDebug() << "TCP Soket bağlanıyor...";
            break;
        case QAbstractSocket::ConnectedState:
            qDebug() << "TCP Soket bağlı!";
            break;
        case QAbstractSocket::ClosingState:
            qDebug() << "TCP Soket kapanıyor...";
            break;
        case QAbstractSocket::UnconnectedState:
            qDebug() << "TCP Soket bağlantısı kesildi.";
            break;
        default:
            qDebug() << "TCP Soket durumu bilinmeyen bir duruma değişti.";
            break;
    }
}

void MainWindow::onTCP_ReadyRead()
{
    if (receiveSocket && receiveSocket->bytesAvailable() > 0) {
        QByteArray data = receiveSocket->readAll();
        qDebug() << "Veri alındı:" << data;
        QString TCP_receivedData = QString::fromUtf8(data);

        if (ui->checkBox->isChecked()) {
            ui->lineEdit_Server->setText(TCP_receivedData);
        }
    }
}

void MainWindow::on_ConnectButton_clicked()
{
    // Eğer önceki soket varsa, onu temizle
    if (sendSocket) {
        if (sendSocket->state() == QAbstractSocket::ConnectedState) {
            sendSocket->disconnectFromHost();
        }
        sendSocket->abort();
        delete sendSocket;
        sendSocket = nullptr;
    }

    // Yeni soket oluştur ve bağlan
    sendSocket = new QTcpSocket(this);
    connect(sendSocket, &QTcpSocket::connected, this, &MainWindow::onSocketStateChanged);
    connect(sendSocket, &QTcpSocket::disconnected, this, &MainWindow::onSocketStateChanged);
    connect(sendSocket, static_cast<void (QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),
            this, [&](QTcpSocket::SocketError socketError){
                qDebug() << "Soket hatası:" << sendSocket->errorString();
            });

    sendSocket->connectToHost(guestIpAddress, send_port);
}


void MainWindow::on_DisconnectButton_clicked()
{
    if (sendSocket && sendSocket->state() == QAbstractSocket::ConnectedState) {
        sendSocket->disconnectFromHost();
        sendSocket->deleteLater();
        sendSocket = nullptr;
        qDebug() << "TCP ana bilgisayarından bağlantı kesildi";
    } else {
        qDebug() << "TCP ana bilgisayarına bağlı değil";
    }

    if (receiveSocket && receiveSocket->state() == QAbstractSocket::ConnectedState) {
        receiveSocket->disconnectFromHost();
        receiveSocket->deleteLater();
        receiveSocket = nullptr;
        qDebug() << "TCP sunucusundan bağlantı kesildi";
    }
}

void MainWindow::on_SendMessageButton_clicked()
{
    static QDateTime lastClickTime;

    QDateTime now = QDateTime::currentDateTime();
    if (lastClickTime.isValid() && lastClickTime.msecsTo(now) < 500) {
        return;
    }

    lastClickTime = now;

    if (!sendSocket) {
        qDebug() << "Soket yok, mesaj gönderilemiyor.";
        return;
    }

    if (sendSocket->state() == QAbstractSocket::ConnectedState) {
        sendSocket->write(ui->lineEdit_Client->text().toUtf8());
        qDebug() << "Mesaj gönderildi:" << ui->lineEdit_Client->text();
    } else {
        qDebug() << "TCP ana bilgisayarına bağlı değil, mesaj gönderilemiyor.";
    }
}

void MainWindow::on_checkBox_toggled(bool checked)
{
    if (checked) {
        qDebug() << "Dinleme aktif";
    } else {
        qDebug() << "Dinleme kapalı";
    }
}

void MainWindow::on_LED_1_clicked()
{
    static QDateTime Led1Click;

    QDateTime now = QDateTime::currentDateTime();
    if (Led1Click.isValid() && Led1Click.msecsTo(now) < 500) {
        return;
    }

    Led1Click = now;
    QByteArray message = ledState[0] ? QByteArray::fromHex("00") : QByteArray::fromHex("01");
    sendTcpMessage(message);
    ledState[0] = !ledState[0];
}

void MainWindow::on_LED_2_clicked()
{
    static QDateTime Led2Click;

    QDateTime now = QDateTime::currentDateTime();
    if (Led2Click.isValid() && Led2Click.msecsTo(now) < 500) {
        return;
    }

    Led2Click = now;
    QByteArray message = ledState[1] ? QByteArray::fromHex("00") : QByteArray::fromHex("02");
    sendTcpMessage(message);
    ledState[1] = !ledState[1];
}

void MainWindow::on_LED_3_clicked()
{
    static QDateTime Led3Click;

    QDateTime now = QDateTime::currentDateTime();
    if (Led3Click.isValid() && Led3Click.msecsTo(now) < 500) {
        return;
    }

    Led3Click = now;
    QByteArray message = ledState[2] ? QByteArray::fromHex("00") : QByteArray::fromHex("04");
    sendTcpMessage(message);
    ledState[2] = !ledState[2];
}

void MainWindow::on_LED_4_clicked()
{
    static QDateTime Led4Click;

    QDateTime now = QDateTime::currentDateTime();
    if (Led4Click.isValid() && Led4Click.msecsTo(now) < 500) {
        return;
    }

    Led4Click = now;
    QByteArray message = ledState[3] ? QByteArray::fromHex("00") : QByteArray::fromHex("08");
    sendTcpMessage(message);
    ledState[3] = !ledState[3];
}

void MainWindow::on_All_LEDS_clicked()
{
    static QDateTime AllLedsClick;

    QDateTime now = QDateTime::currentDateTime();
    if (AllLedsClick.isValid() && AllLedsClick.msecsTo(now) < 500) {
        return;
    }

    AllLedsClick = now;
    QByteArray message;

    if (ledState[0] && ledState[1] && ledState[2] && ledState[3]) {
        message = QByteArray::fromHex("00");  // Tüm LED'leri kapat
    } else {
        message = QByteArray::fromHex("0F");  // Tüm LED'leri aç
    }
    sendTcpMessage(message);
    bool newState = !(ledState[0] && ledState[1] && ledState[2] && ledState[3]);
    for (int i = 0; i < 4; i++) {
        ledState[i] = newState;
    }
}
