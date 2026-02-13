QT += core network
CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = FTPproxy

SOURCES += \
    main.cpp \
    proxy.cpp

HEADERS += \
    proxy.h
