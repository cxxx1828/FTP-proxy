#include "proxy.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcProxy, "ftp.proxy.main")

FTPProxy::FTPProxy(const ProxyConfig &config, QObject *parent)
    : QObject(parent), m_config(config)
{
    connect(&m_proxyServer, &QTcpServer::newConnection, this, &FTPProxy::acceptClient);
}

FTPProxy::~FTPProxy()
{
    stop();
}

bool FTPProxy::start()
{
    if (!m_proxyServer.listen(QHostAddress::Any, m_config.listenPort)) {
        qCCritical(lcProxy) << "Failed to bind proxy listener on port" << m_config.listenPort
                            << "Error:" << m_proxyServer.errorString();
        return false;
    }

    qCInfo(lcProxy) << "FTP Proxy server started on port" << m_config.listenPort 
                    << "-> Routing to" << m_config.targetHost << ":" << m_config.targetPort;
    return true;
}

void FTPProxy::stop()
{
    m_proxyServer.close();
    for (auto *session : std::as_const(m_sessions)) {
        session->deleteLater();
    }
    m_sessions.clear();
    qCInfo(lcProxy) << "FTP Proxy server stopped.";
}

void FTPProxy::acceptClient()
{
    while (m_proxyServer.hasPendingConnections()) {
        QTcpSocket *clientSocket = m_proxyServer.nextPendingConnection();
        QUuid sessionId = QUuid::createUuid();

        qCInfo(lcProxy) << "Accepted incoming connection from" 
                        << clientSocket->peerAddress().toString() 
                        << "Session ID:" << sessionId.toString();

        auto *session = new ProxySession(sessionId, clientSocket, m_config.targetHost, m_config.targetPort, this);
        connect(session, &ProxySession::finished, this, &FTPProxy::onSessionFinished);
        
        m_sessions.insert(sessionId, session);
        session->start();
    }
}

void FTPProxy::onSessionFinished(QUuid id)
{
    if (m_sessions.contains(id)) {
        ProxySession *session = m_sessions.take(id);
        session->deleteLater();
        qCInfo(lcProxy) << "Cleaned up active session:" << id.toString();
    }
}
