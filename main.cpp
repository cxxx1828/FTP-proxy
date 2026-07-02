#include <QCoreApplication>
#include "proxy.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv); 
    
    FTPProxy proxy;
    if (!proxy.start()) { 
        return -1;
    }

    return a.exec();
}
