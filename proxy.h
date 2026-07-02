#ifndef PROXY_H
#define PROXY_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class FTPProxy : public QObject
{
    Q_OBJECT
public:
    explicit FTPProxy(QObject *parent = nullptr);
    bool start();

private slots: 
    void acceptClient();
    void fromClient();
    void fromServer();
    void clientDisconnected();
    void serverDisconnected();

private:
    void cleanupConnections();

    QTcpServer proxyServer; 
    QTcpSocket *clientSocket; 
    QTcpSocket *serverSocket; 
};

#endif // PROXY_H
