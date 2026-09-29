#ifndef PROXY_SESSION_H
#define PROXY_SESSION_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QUuid>
#include <QRegularExpression>

class ProxySession : public QObject
{
    Q_OBJECT
public:
    explicit ProxySession(QUuid id, QTcpSocket *clientSock, const QString &targetHost, quint16 targetPort, QObject *parent = nullptr);
    ~ProxySession() override;

    QUuid id() const { return m_id; }
    void start();

signals:
    void finished(QUuid id);

private slots:
    void onClientReadyRead();
    void onServerReadyRead();
    void onClientDisconnected();
    void onServerDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);
    void onDataChannelNewConnection();

private:
    void processClientCommand(const QByteArray &line);
    void processServerResponse(const QByteArray &line);
    
    void setupPassiveDataProxy(const QString &ip, quint16 port);
    void setupExtendedPassiveDataProxy(quint16 port);

    QUuid m_id;
    QTcpSocket *m_clientSocket{nullptr};
    QTcpSocket *m_serverSocket{nullptr};
    
    QString m_targetHost;
    quint16 m_targetPort;

    QByteArray m_clientBuffer;
    QByteArray m_serverBuffer;

    // Passive Data Channel Tunneling
    QTcpServer *m_dataProxyServer{nullptr};
    QTcpSocket *m_clientDataSocket{nullptr};
    QTcpSocket *m_serverDataSocket{nullptr};
    
    QString m_pasvTargetIp;
    quint16 m_pasvTargetPort{0};
    
    bool m_isEpsv{false};
};

#endif // PROXY_SESSION_H
