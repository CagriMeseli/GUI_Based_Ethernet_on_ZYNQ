#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // IP adresi ve portu ayarlayın
    ipAddress = "192.168.200.10";
    port = 42000;

    // TCP sunucuyu başlat
    startTcpServer();

    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startTcpServer()
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::onClientConnected);

    if (!tcpServer->listen(QHostAddress::AnyIPv4, port)) {
        qDebug() << "TCP server could not start!";
    } else {
        qDebug() << "TCP server started, listening on port" << port;
    }
}

void MainWindow::onConnectButtonClicked()
{
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::connected, this, &MainWindow::onSocketStateChanged);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::onSocketStateChanged);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);

    // Sunucuya bağlan
    tcpSocket->connectToHost(ipAddress, port);
    QString message = "Hello from GUI!";
    tcpSocket->write(message.toUtf8());
}

void MainWindow::onClientConnected()
{
    while (tcpServer->hasPendingConnections()) {
        tcpSocket = tcpServer->nextPendingConnection();
        connect(tcpSocket, &QTcpSocket::disconnected, tcpSocket, &QTcpSocket::deleteLater);
        connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);

        qDebug() << "Client connected!";
    }
}

void MainWindow::onSocketStateChanged()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    qDebug() << "Socket state changed:" << socket->state();

    switch (socket->state()) {
    case QAbstractSocket::ConnectingState:
        qDebug() << "Socket is connecting...";
        break;
    case QAbstractSocket::ConnectedState:
        qDebug() << "Socket is connected!";
        break;
    case QAbstractSocket::ClosingState:
        qDebug() << "Socket is closing...";
        break;
    case QAbstractSocket::UnconnectedState:
        qDebug() << "Socket is disconnected.";
        break;
    default:
        qDebug() << "Socket state changed to unknown state.";
        break;
    }
}

void MainWindow::onReadyRead()
{
    if (tcpSocket->bytesAvailable() > 0) {
        QByteArray data = tcpSocket->readAll();
        qDebug() << "Received data:" << data;
        // Burada gelen veriyi işleyebilirsiniz
    }
}

