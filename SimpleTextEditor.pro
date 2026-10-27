QT += core gui widgets
CONFIG += c++17
TEMPLATE = app
TARGET = SimpleTextEditor

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    codeeditor.cpp \
    searchdialog.cpp \
    aboutdialog.cpp

HEADERS += \
    mainwindow.h \
    codeeditor.h \
    searchdialog.h \
    aboutdialog.h

FORMS += mainwindow.ui searchdialog.ui aboutdialog.ui
RESOURCES += resources.qrc
DISTFILES += README.md scripts/build.ps1 tests/editor_tests.pro tests/editor_tests.cpp

# Keep generated build intermediates inside the shadow build directory.
OBJECTS_DIR = .obj
MOC_DIR = .moc
RCC_DIR = .rcc
UI_DIR = .ui
