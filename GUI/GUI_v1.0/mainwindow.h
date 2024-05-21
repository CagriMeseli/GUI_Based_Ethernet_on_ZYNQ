#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>

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
    void onConnectButtonClicked();
    void onClientConnected();
    void onSocketStateChanged();
    void onReadyRead();


private:
    Ui::MainWindow *ui;
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket;

    QString ipAddress;
    quint16 port;

    void startTcpServer();
};

#endif // MAINWINDOW_H
