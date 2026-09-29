#include "proxy_session.h"
#include <QLoggingCategory>
#include <QHostAddress>

Q_LOGGING_CATEGORY(lcSession, "ftp.proxy.session")

ProxySession::ProxySession(QUuid id, QTcpSocket *clientSock, const QString &targetHost, quint16 targetPort, QObject *parent)
    : QObject(parent), m_id(id), m_clientSocket(clientSock), m_targetHost(targetHost), m_targetPort(targetPort)
{
    m_clientSocket->setParent(this);
}

ProxySession::~ProxySession()
{
    qCDebug(lcSession) << "Destroying proxy session:" << m_id.toString();
}

void ProxySession::start()
{
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &ProxySession::onClientReadyRead);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &ProxySession::onClientDisconnected);
    connect(m_clientSocket, &QTcpSocket::errorOccurred, this, &ProxySession::onErrorOccurred);

    m_serverSocket = new QTcpSocket(this);
    connect(m_serverSocket, &QTcpSocket::readyRead, this, &ProxySession::onServerReadyRead);
    connect(m_serverSocket, &QTcpSocket::disconnected, this, &ProxySession::onServerDisconnected);
    connect(m_serverSocket, &QTcpSocket::errorOccurred, this, &ProxySession::onErrorOccurred);

    qCInfo(lcSession) << "[" << m_id.toString() << "] Connecting to upstream FTP server" << m_targetHost << ":" << m_targetPort;
    m_serverSocket->connectToHost(m_targetHost, m_targetPort);
}

void ProxySession::onClientReadyRead()
{
    m_clientBuffer.append(m_clientSocket->readAll());
    
    int index = -1;
    while ((index = m_clientBuffer.indexOf("\r\n")) != -1) {
        QByteArray line = m_clientBuffer.left(index + 2);
        m_clientBuffer.remove(0, index + 2);
        processClientCommand(line);
    }
}

void ProxySession::processClientCommand(const QByteArray &line)
{
    QString cmd = QString::fromUtf8(line).trimmed();
    qCDebug(lcSession) << "[" << m_id.toString() << "] CLIENT -> SERVER:" << cmd;

    // Reject TLS/SSL to prevent unhandled encrypted payload tunneling
    if (cmd.startsWith("AUTH TLS", Qt::CaseInsensitive) || cmd.startsWith("AUTH SSL", Qt::CaseInsensitive)) {
        m_clientSocket->write("500 TLS security extension not supported by proxy\r\n");
        return;
    }

    if (m_serverSocket && m_serverSocket->state() == QAbstractSocket::ConnectedState) {
        m_serverSocket->write(line);
    }
}

void ProxySession::onServerReadyRead()
{
    m_serverBuffer.append(m_serverSocket->readAll());

    int index = -1;
    while ((index = m_serverBuffer.indexOf("\r\n")) != -1) {
        QByteArray line = m_serverBuffer.left(index + 2);
        m_serverBuffer.remove(0, index + 2);
        processServerResponse(line);
    }
}

void ProxySession::processServerResponse(const QByteArray &line)
{
    QString response = QString::fromUtf8(line).trimmed();
    qCDebug(lcSession) << "[" << m_id.toString() << "] SERVER -> CLIENT:" << response;

    // Handle PASV Response (227 Entering Passive Mode (h1,h2,h3,h4,p1,p2))
    if (response.startsWith("227")) {
        static const QRegularExpression pasvRegex(R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
        QRegularExpressionMatch match = pasvRegex.match(response);
        if (match.hasCaptured()) {
            m_pasvTargetIp = QString("%1.%2.%3.%4")
                                 .arg(match.captured(1), match.captured(2), match.captured(3), match.captured(4));
            m_pasvTargetPort = (match.captured(5).toUShort() << 8) + match.captured(6).toUShort();
            m_isEpsv = false;

            setupPassiveDataProxy(m_pasvTargetIp, m_pasvTargetPort);
            return;
        }
    }
    // Handle EPSV Response (229 Entering Extended Passive Mode (|||port|))
    else if (response.startsWith("229")) {
        static const QRegularExpression epsvRegex(R"(\(\|\|\|(\d+)\|\))");
        QRegularExpressionMatch match = epsvRegex.match(response);
        if (match.hasCaptured()) {
            m_pasvTargetPort = match.captured(1).toUShort();
            m_pasvTargetIp = m_targetHost;
            m_isEpsv = true;

            setupExtendedPassiveDataProxy(m_pasvTargetPort);
            return;
        }
    }

    if (m_clientSocket && m_clientSocket->state() == QAbstractSocket::ConnectedState) {
        m_clientSocket->write(line);
    }
}

void ProxySession::setupPassiveDataProxy(const QString &ip, quint16 port)
{
    if (m_dataProxyServer) {
        m_dataProxyServer->close();
        m_dataProxyServer->deleteLater();
    }

    m_dataProxyServer = new QTcpServer(this);
    if (!m_dataProxyServer->listen(QHostAddress::Any, 0)) { // Bind on random free port
        m_clientSocket->write("425 Can't open data connection.\r\n");
        return;
    }

    quint16 proxyDataPort = m_dataProxyServer->serverPort();
    connect(m_dataProxyServer, &QTcpServer::newConnection, this, &ProxySession::onDataChannelNewConnection);

    // Rewrite PASV output response to target proxy listener IP/Port
    QHostAddress localProxyAddr = m_clientSocket->localAddress();
    QStringList ipParts = localProxyAddr.toString().remove("::ffff:").split('.');
    if (ipParts.size() != 4) ipParts = QStringList{"127", "0", "0", "1"};

    quint8 p1 = static_cast<quint8>(proxyDataPort >> 8);
    quint8 p2 = static_cast<quint8>(proxyDataPort & 0xFF);

    QByteArray rewrittenPasv = QString("227 Entering Passive Mode (%1,%2,%3,%4,%5,%6).\r\n")
                                   .arg(ipParts[0], ipParts[1], ipParts[2], ipParts[3])
                                   .arg(p1).arg(p2).toUtf8();

    m_clientSocket->write(rewrittenPasv);
}

void ProxySession::setupExtendedPassiveDataProxy(quint16 port)
{
    if (m_dataProxyServer) {
        m_dataProxyServer->close();
        m_dataProxyServer->deleteLater();
    }

    m_dataProxyServer = new QTcpServer(this);
    if (!m_dataProxyServer->listen(QHostAddress::Any, 0)) {
        m_clientSocket->write("425 Can't open data connection.\r\n");
        return;
    }

    quint16 proxyDataPort = m_dataProxyServer->serverPort();
    connect(m_dataProxyServer, &QTcpServer::newConnection, this, &ProxySession::onDataChannelNewConnection);

    QByteArray rewrittenEpsv = QString("229 Entering Extended Passive Mode (|||%1|).\r\n")
                                   .arg(proxyDataPort).toUtf8();

    m_clientSocket->write(rewrittenEpsv);
}

void ProxySession::onDataChannelNewConnection()
{
    m_clientDataSocket = m_dataProxyServer->nextPendingConnection();
    m_serverDataSocket = new QTcpSocket(this);

    connect(m_clientDataSocket, &QTcpSocket::readyRead, this, [this]() {
        if (m_serverDataSocket->state() == QAbstractSocket::ConnectedState) {
            m_serverDataSocket->write(m_clientDataSocket->readAll());
        }
    });

    connect(m_serverDataSocket, &QTcpSocket::readyRead, this, [this]() {
        if (m_clientDataSocket->state() == QAbstractSocket::ConnectedState) {
            m_clientDataSocket->write(m_serverDataSocket->readAll());
        }
    });

    auto cleanupDataChannel = [this]() {
        if (m_clientDataSocket) { m_clientDataSocket->close(); m_clientDataSocket->deleteLater(); m_clientDataSocket = nullptr; }
        if (m_serverDataSocket) { m_serverDataSocket->close(); m_serverDataSocket->deleteLater(); m_serverDataSocket = nullptr; }
        if (m_dataProxyServer) { m_dataProxyServer->close(); m_dataProxyServer->deleteLater(); m_dataProxyServer = nullptr; }
    };

    connect(m_clientDataSocket, &QTcpSocket::disconnected, this, cleanupDataChannel);
    connect(m_serverDataSocket, &QTcpSocket::disconnected, this, cleanupDataChannel);

    m_serverDataSocket->connectToHost(m_pasvTargetIp, m_pasvTargetPort);
}

void ProxySession::onClientDisconnected()
{
    qCInfo(lcSession) << "[" << m_id.toString() << "] Client disconnected";
    emit finished(m_id);
}

void ProxySession::onServerDisconnected()
{
    qCInfo(lcSession) << "[" << m_id.toString() << "] Server disconnected";
    emit finished(m_id);
}

void ProxySession::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    auto *sock = qobject_cast<QTcpSocket*>(sender());
    qCWarning(lcSession) << "[" << m_id.toString() << "] Socket Error:" 
                         << (sock == m_clientSocket ? "Client" : "Server") 
                         << socketError << sock->errorString();
    emit finished(m_id);
}
