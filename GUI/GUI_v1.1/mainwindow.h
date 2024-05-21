#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QUdpSocket>
#include <QDebug>
#include <QString>
#include <QHostAddress>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onClientConnected();
    void onSocketStateChanged();
    void onTCP_ReadyRead();
    void onUDP_ReadyRead();
    void on_UDPConnectButton_clicked();
    void on_TCPConnectButton_clicked();
    void on_DisconnectButton_clicked();



private:
    Ui::MainWindow *ui;
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket;
    QUdpSocket *udpSocket;
    quint16 tcp_port;
    quint16 udp_port;
    QString hostIpAddress;
    QString guestIpAddress;
    QByteArray hexMessage;

    void startTcpServer();
    void startUdpServer();
};

#endif // MAINWINDOW_H
