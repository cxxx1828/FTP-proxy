#include <QCoreApplication>
#include <QCommandLineParser>
#include "proxy.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("Qt-FTP-Proxy");
    QCoreApplication::setApplicationVersion("2.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Production Multithreaded FTP Control and Data Proxy");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(QStringList() << "p" << "port", "Listening port", "port", "2121");
    QCommandLineOption targetHostOption(QStringList() << "t" << "target", "Target FTP Server IP/Host", "host", "127.0.0.1");
    QCommandLineOption targetPortOption(QStringList() << "r" << "remote-port", "Target FTP Server Port", "remote-port", "21");

    parser.addOption(portOption);
    parser.addOption(targetHostOption);
    parser.addOption(targetPortOption);
    parser.process(app);

    ProxyConfig config;
    config.listenPort = parser.value(portOption).toUShort();
    config.targetHost = parser.value(targetHostOption);
    config.targetPort = parser.value(targetPortOption).toUShort();

    FTPProxy proxy(config);
    if (!proxy.start()) {
        return -1;
    }

    return app.exec();
}
