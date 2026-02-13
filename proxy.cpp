#include "proxy.h"
#include <QDebug>

#define PROXY_PORT 2121
#define FTP_SERVER_IP "127.0.0.1"
#define FTP_SERVER_PORT 21

FTPProxy::FTPProxy(QObject *parent) //konstruktor
    : QObject(parent),
    clientSocket(nullptr),
    serverSocket(nullptr)
{
    connect(&proxyServer, &QTcpServer::newConnection,
            this, &FTPProxy::acceptClient); // kad neko pokusa da se poveze na proksi, pozovi funkciju acceptClient()
}

bool FTPProxy::start()
{
    if (!proxyServer.listen(QHostAddress::Any, PROXY_PORT)) { //proxy počinje da sluša port 2121
        qCritical() << "Proxy failed to start";
        return false;
    }

    qDebug() << "FTP Proxy listening on port" << PROXY_PORT;
    return true;
}



void FTPProxy::acceptClient()
{
    // Očisti prethodnu konekciju ako postoji
    if (clientSocket || serverSocket) {
        cleanupConnections();
    }

    clientSocket = proxyServer.nextPendingConnection(); //sada proxy ima vezu ka klijentu.
    qDebug() << "Client connected from"
             << clientSocket->peerAddress().toString();

    serverSocket = new QTcpSocket(this);
    serverSocket->connectToHost(FTP_SERVER_IP, FTP_SERVER_PORT);// Proxy se povezuje na 127.0.0.1:21, to je pravi FTP server (npr. vsftpd).

    connect(clientSocket, &QTcpSocket::readyRead,
            this, &FTPProxy::fromClient);// kad klijent salje podatke
    connect(serverSocket, &QTcpSocket::readyRead,
            this, &FTPProxy::fromServer);// kad server posalje podatke

    connect(clientSocket, &QTcpSocket::disconnected,
            this, &FTPProxy::clientDisconnected);// -||- samo za diskonektovanje
    connect(serverSocket, &QTcpSocket::disconnected,
            this, &FTPProxy::serverDisconnected);
}

void FTPProxy::fromClient() // funkcija se poziva kada klijent pošalje komandu
{
    if (!clientSocket || !serverSocket) return;

    QByteArray data = clientSocket->readAll();
    QString cmd = QString::fromUtf8(data).trimmed();

    qDebug() << "CLIENT -> SERVER:" << cmd;

    // Blokiraj TLS/SSL
    if (cmd.startsWith("AUTH TLS") || cmd.startsWith("AUTH SSL")) { //ne podrzava TLS, nema enkripcije
        clientSocket->write("500 TLS not supported\r\n");
        return;
    }

    // Log za QUIT komandu
    if (cmd.startsWith("QUIT")) {
        qDebug() << "QUIT command received";
    }

    if (serverSocket->state() == QAbstractSocket::ConnectedState) {
        serverSocket->write(data); //proxy ne menja komandu – samo je prosleđuje.
    }
}

void FTPProxy::fromServer() //funkcija se poziva kada server pošalje odgovor
{
    if (!clientSocket || !serverSocket) return;

    QByteArray data = serverSocket->readAll();
    if (data.isEmpty()) return;

    QString response = QString::fromUtf8(data).trimmed();

    qDebug() << "SERVER -> CLIENT:" << response;

    // Log za QUIT odgovor
    if (response.startsWith("221")) {
        qDebug() << "Server sent QUIT response: 221 Goodbye";
    }

    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
        clientSocket->write(data); // samo prosledjuje klijentu, proxy ne menja odgovor
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
