#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    hostIpAddress = "192.168.1.12";
    guestIpAddress = "192.168.1.10";
    startTcpServer();
    startUdpServer();
    tcp_port = 61250;
    udp_port = 62371;

    connect(ui->TCPConnectButton, &QPushButton::clicked, this, &MainWindow::on_TCPConnectButton_clicked);
    connect(ui->UDPConnectButton, &QPushButton::clicked, this, &MainWindow::on_UDPConnectButton_clicked);
    connect(ui->DisconnectButton, &QPushButton::clicked, this, &MainWindow::on_DisconnectButton_clicked);
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startTcpServer()
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::onClientConnected);

    if (!tcpServer->listen(QHostAddress::AnyIPv4, tcp_port)) {
        qDebug() << "TCP server could not start!";
    } else {
        qDebug() << "TCP server started, listening on port" << tcp_port;
    }
}

void MainWindow::startUdpServer()
{
    udpSocket = new QUdpSocket(this);
    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::onUDP_ReadyRead);

    if (!udpSocket->bind(QHostAddress::AnyIPv4, udp_port)) {
        qDebug() << "UDP server could not start!";
    } else {
        qDebug() << "UDP server started, listening on port" << udp_port;
    }
}

void MainWindow::onClientConnected()
{
    while (tcpServer->hasPendingConnections()) {
        tcpSocket = tcpServer->nextPendingConnection();
        connect(tcpSocket, &QTcpSocket::disconnected, tcpSocket, &QTcpSocket::deleteLater);
        connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onTCP_ReadyRead);

        qDebug() << "Client connected!";
    }
}

void MainWindow::onSocketStateChanged()
{

    QAbstractSocket *socket = qobject_cast<QAbstractSocket*>(sender());
    if (!socket)
        return;

    if (QTcpSocket *tcpSocket = qobject_cast<QTcpSocket*>(socket)) {
        qDebug() << "TCP Socket state changed:" << tcpSocket->state();
        switch (tcpSocket->state()) {
            case QAbstractSocket::ConnectingState:
                qDebug() << "TCP Socket is connecting...";
                break;
            case QAbstractSocket::ConnectedState:
                qDebug() << "TCP Socket is connected!";
                break;
            case QAbstractSocket::ClosingState:
                qDebug() << "TCP Socket is closing...";
                break;
            case QAbstractSocket::UnconnectedState:
                qDebug() << "TCP Socket is disconnected.";
                break;
            default:
                qDebug() << "TCP Socket state changed to unknown state.";
                break;
        }
    }

    if (QUdpSocket *udpSocket = qobject_cast<QUdpSocket*>(socket)) {
        qDebug() << "UDP Socket state changed:" << udpSocket->state();
        switch (udpSocket->state()) {
            case QAbstractSocket::ConnectingState:
                qDebug() << "UDP Socket is connecting...";
                break;
            case QAbstractSocket::ConnectedState:
                qDebug() << "UDP Socket is connected!";
                break;
            case QAbstractSocket::ClosingState:
                qDebug() << "UDP Socket is closing...";
                break;
            case QAbstractSocket::UnconnectedState:
                qDebug() << "UDP Socket is disconnected.";
                break;
            default:
                qDebug() << "UDP Socket state changed to unknown state.";
                break;
        }
    }
}

void MainWindow::onTCP_ReadyRead()
{
    if (tcpSocket->bytesAvailable() > 0) {
        QByteArray data = tcpSocket->readAll();
        qDebug() << "Received data:" << data;
        QString TCP_receivedData = QString::fromUtf8(data);

        ui->TCPlabel->setText(TCP_receivedData);
    }
}

void MainWindow::on_UDPConnectButton_clicked()
{
    connect(udpSocket, &QUdpSocket::connected, this, &MainWindow::onSocketStateChanged);
    connect(udpSocket, &QUdpSocket::disconnected, this, &MainWindow::onSocketStateChanged);

    udpSocket->connectToHost(guestIpAddress, udp_port);
    //QString message = "Hello from GUI!";
    hexMessage.clear();
    hexMessage.append((char)0x04);
    udpSocket->write(hexMessage);

}

void MainWindow::onUDP_ReadyRead()
{
    while (udpSocket->bytesAvailable() > 0) {
       QByteArray datagram;
       //QHostAddress sender;
       //quint16 senderPort;
       //udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

       qDebug() << "Received data:" << datagram;
       QString UDP_receivedData = QString::fromUtf8(datagram);

       ui->UDPlabel->setText(UDP_receivedData);
   }
}

void MainWindow::on_TCPConnectButton_clicked()
{
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::connected, this, &MainWindow::onSocketStateChanged);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::onSocketStateChanged);

    tcpSocket->connectToHost(guestIpAddress, tcp_port);
    QString message = "Hello from GUI!";
    hexMessage.clear();
    hexMessage.append((char)0x01);
    tcpSocket->write(hexMessage);
}

void MainWindow::on_DisconnectButton_clicked()
{
    if (tcpSocket && tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->disconnectFromHost();
        tcpSocket->deleteLater(); // Socket'i sil
        qDebug() << "Disconnected from TCP host";
    } else {
        qDebug() << "Not connected to TCP host";
    }

    if (udpSocket) {
        udpSocket->close();
        udpSocket->deleteLater(); // Socket'i sil
        qDebug() << "UDP socket closed";
    }
}

