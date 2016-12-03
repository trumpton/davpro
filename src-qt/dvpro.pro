#-------------------------------------------------
#
# Project created by QtCreator 2016-08-15T12:38:24
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dvpro
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    printerinterface.cpp \
    configuration.cpp

HEADERS  += mainwindow.h \
    printerinterface.h \
    configuration.h

FORMS    += mainwindow.ui \
    configuration.ui

DISTFILES +=

RESOURCES += \
    resources.qrc
