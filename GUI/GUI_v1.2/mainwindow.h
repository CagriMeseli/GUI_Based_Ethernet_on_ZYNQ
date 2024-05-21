#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void startTcpServer();
    void on_ConnectButton_clicked();
    void on_DisconnectButton_clicked();
    void onClientConnected();
    void onTCP_ReadyRead();
    void onSocketStateChanged();
    void on_SendMessageButton_clicked();
    void on_checkBox_toggled(bool checked);
    void onSocketDisconnected();
    void on_LED_1_clicked();
    void on_LED_2_clicked();
    void on_LED_3_clicked();
    void on_LED_4_clicked();
    void on_All_LEDS_clicked();

private:

    Ui::MainWindow *ui;
    QTcpServer *tcpServer;
    QTcpSocket *sendSocket;
    QTcpSocket *receiveSocket;
    QString hostIpAddress;
    QString guestIpAddress;
    quint16 send_port;
    quint16 receive_port;

    void sendTcpMessage(const QByteArray &message);  // Fonksiyonun deklarasyonunu ekleyin

    bool ledState[4] = {false, false, false, false};
};

#endif // MAINWINDOW_H
