QT += core gui widgets
CONFIG += c++17
TEMPLATE = app
TARGET = SimpleTextEditor
SOURCES += main.cpp mainwindow.cpp codeeditor.cpp searchdialog.cpp
HEADERS += mainwindow.h codeeditor.h searchdialog.h
FORMS += mainwindow.ui
