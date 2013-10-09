#-------------------------------------------------
#
# Project created by QtCreator 2013-08-03T03:44:59
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tt4light
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    videowidget.cpp \
    userwidget.cpp

HEADERS  += mainwindow.h \
    videowidget.h \
    userwidget.h \
    constants.h \
    common.h

FORMS    += mainwindow.ui

LIBS += -L../../../tt4sdk_debian6_i386/Client/ -lTeamTalk4

INCLUDEPATH +=  ../../../tt4sdk_debian6_i386/Client/C-API
