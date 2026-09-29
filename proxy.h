#ifndef PROXY_H
#define PROXY_H

#include <QObject>
#include <QTcpServer>
#include <QMap>
#include <QUuid>
#include "proxy_session.h"

struct ProxyConfig {
    quint16 listenPort{2121};
    QString targetHost{"127.0.0.1"};
    quint16 targetPort{21};
};

class FTPProxy : public QObject
{
    Q_OBJECT
public:
    explicit FTPProxy(const ProxyConfig &config, QObject *parent = nullptr);
    ~FTPProxy() override;

    bool start();
    void stop();

private slots:
    void acceptClient();
    void onSessionFinished(QUuid id);

private:
    ProxyConfig m_config;
    QTcpServer m_proxyServer;
    QMap<QUuid, ProxySession*> m_sessions;
};

#endif // PROXY_H
