#include "proxy.h"
#include <QDebug>

#define PROXY_PORT 2121
#define FTP_SERVER_IP "127.0.0.1"
#define FTP_SERVER_PORT 21

FTPProxy::FTPProxy(QObject *parent) 
    : QObject(parent),
    clientSocket(nullptr),
    serverSocket(nullptr)
{
    connect(&proxyServer, &QTcpServer::newConnection,
            this, &FTPProxy::acceptClient); 
}

bool FTPProxy::start()
{
    if (!proxyServer.listen(QHostAddress::Any, PROXY_PORT)) { 
        qCritical() << "Proxy failed to start";
        return false;
    }

    qDebug() << "FTP Proxy listening on port" << PROXY_PORT;
    return true;
}



void FTPProxy::acceptClient()
{
   
    if (clientSocket || serverSocket) {
        cleanupConnections();
    }

    clientSocket = proxyServer.nextPendingConnection(); 
    qDebug() << "Client connected from"
             << clientSocket->peerAddress().toString();

    serverSocket = new QTcpSocket(this);
    serverSocket->connectToHost(FTP_SERVER_IP, FTP_SERVER_PORT);
    connect(clientSocket, &QTcpSocket::readyRead,
            this, &FTPProxy::fromClient);
    connect(serverSocket, &QTcpSocket::readyRead,
            this, &FTPProxy::fromServer);

    connect(clientSocket, &QTcpSocket::disconnected,
            this, &FTPProxy::clientDisconnected);
    connect(serverSocket, &QTcpSocket::disconnected,
            this, &FTPProxy::serverDisconnected);
}

void FTPProxy::fromClient() 
{
    if (!clientSocket || !serverSocket) return;

    QByteArray data = clientSocket->readAll();
    QString cmd = QString::fromUtf8(data).trimmed();

    qDebug() << "CLIENT -> SERVER:" << cmd;

  
    if (cmd.startsWith("AUTH TLS") || cmd.startsWith("AUTH SSL")) { 
        clientSocket->write("500 TLS not supported\r\n");
        return;
    }

    
    if (cmd.startsWith("QUIT")) {
        qDebug() << "QUIT command received";
    }

    if (serverSocket->state() == QAbstractSocket::ConnectedState) {
        serverSocket->write(data); 
    }
}

void FTPProxy::fromServer() 
{
    if (!clientSocket || !serverSocket) return;

    QByteArray data = serverSocket->readAll();
    if (data.isEmpty()) return;

    QString response = QString::fromUtf8(data).trimmed();

    qDebug() << "SERVER -> CLIENT:" << response;

    
    if (response.startsWith("221")) {
        qDebug() << "Server sent QUIT response: 221 Goodbye";
    }

    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
        clientSocket->write(data); 
    }
}

void FTPProxy::cleanupConnections()
{
    qDebug() << "Cleaning up connections...";

    if (clientSocket) {
        disconnect(clientSocket, nullptr, this, nullptr);
        clientSocket->close();
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }

    if (serverSocket) {
        disconnect(serverSocket, nullptr, this, nullptr);
        serverSocket->close();
        serverSocket->deleteLater();
        serverSocket = nullptr;
    }
}

void FTPProxy::clientDisconnected()
{
    qDebug() << "Client disconnected signal received";
    cleanupConnections();
}

void FTPProxy::serverDisconnected()
{
    qDebug() << "Server disconnected signal received";
    cleanupConnections();
}
