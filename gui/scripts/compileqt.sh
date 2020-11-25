#!/bin/bash
#uic-qt5 -o ui_mainwindow.h mainwindow.ui
uic -o ui_mainwindow.h mainwindow.ui
qmake librteye_gui.pro 
make -j4
