#-------------------------------------------------
#
# Project created by QtCreator 2018-05-15T11:11:08
#
#-------------------------------------------------

QT       += core gui concurrent

include(../Global.pri)
include(../Shared/Shared.pri)
include(../SharedGui/SharedGui.pri)
include(../GraphicsToolsModule/GraphicsToolsModule.pri)
include(../PropertiesModule/PropertiesModule.pri)
include(../ResourcesModule/ResourcesModule.pri)

INCLUDEPATH += ../

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GameEngine
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui
