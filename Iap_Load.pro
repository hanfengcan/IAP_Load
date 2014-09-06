#-------------------------------------------------
#
# Project created by QtCreator 2014-08-30T14:32:51
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Iap_Load
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
    crc_cal.cpp

HEADERS  += widget.h \
    crc_cal.h

FORMS    += widget.ui

RESOURCES += \
    src/src.qrc
