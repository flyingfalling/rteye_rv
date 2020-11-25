#-------------------------------------------------
#
# Project created by QtCreator 2018-01-25T17:10:42
#
#-------------------------------------------------

QT       += core gui #network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = librteye_gui
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# REV: in case we need FS
QMAKE_CC = gcc-8
QMAKE_CXX = g++-8

SALMAPRVDIR = $$(SALMAPRVDIR)
              
isEmpty(SALMAPRVDIR) {
SALMAPRVDIR = /home/riveale/git/salmap_rv
}

INCLUDEPATH += \
$$PWD/.. $$PWD/../include $$PWD/../cpp $$SALMAPRVDIR/  $$SALMAPRVDIR/salmap_rv  $$SALMAPRVDIR/salmap_rv/include $$SALMAPRVDIR/dtl $$SALMAPRVDIR/build

CONFIG += link_pkgconfig debug
CONFIG += c++17
QMAKE_CXXFLAGS += -std=c++17

OCV_VERS=opencv4 #opencv
PKGCONFIG += $$OCV_VERS libavcodec libavformat libavutil

#-Wl,-rpath,$(DEFAULT_LIB_INSTALL_PATH)

QMAKE_LFLAGS += -Wl,-rpath,$$SALMAPRVDIR/build -Wl,-rpath,$$(PWD)/../build

SOURCES += \
main.cpp \
mainwindow.cpp 

HEADERS += \
mainwindow.h

LIBS += -L$$PWD/.. -L$$PWD/. -L$$SALMAPRVDIR/build -L$$PWD/../build -lboost_system -lboost_thread -lSDL2 -lrteye2 -lsalmap_rv -lstdc++fs

FORMS += \
        mainwindow.ui
