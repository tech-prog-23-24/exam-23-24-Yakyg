QT -= gui

QT += network

CONFIG += c++11 console
CONFIG -= app_bundle

SOURCES += \
    main.cpp \
    mytcpserver.cpp

HEADERS += \
    mytcpserver.h
