#ifndef PROXY_H
#define PROXY_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class FTPProxy : public QObject //nasleđuje QObject jer koristi Qt signal-slot mehanizam, QObject je event driven arhitektura
{
    Q_OBJECT
public:
    explicit FTPProxy(QObject *parent = nullptr);
    bool start();

private slots: //slots = funkcije koje se automatski pozivaju kad se desi neki događaj (signal)
    void acceptClient();
    void fromClient();
    void fromServer();
    void clientDisconnected();
    void serverDisconnected();

private:
    void cleanupConnections();

    QTcpServer proxyServer; // sluša port 2121 i čeka da se klijent poveže
    QTcpSocket *clientSocket; // veza ka FTP klijentu, proxy se prema njemu ponaša kao server
    QTcpSocket *serverSocket; // veza ka pravom FTP serveru (127.0.0.1:21), proxy se prema njemu ponaša kao klijent
};

#endif // PROXY_H
