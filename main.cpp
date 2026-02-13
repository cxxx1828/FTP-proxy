#include <QCoreApplication>
#include "proxy.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv); //Pošto nema GUI, koristi se QCoreApplication, aka nema GUI samo event loop
    //zasto event loop? -> zato što Qt koristi asinhroni signal-slot mehanizam za obradu mrežnih događaja
    FTPProxy proxy;
    if (!proxy.start()) { //Event loop čeka TCP događaje
        return -1;
    }

    return a.exec();
}
