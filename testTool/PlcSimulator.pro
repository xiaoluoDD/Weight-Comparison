QT += core widgets network

CONFIG += c++17

TARGET = PlcSimulator
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# 构建到上级 bin 目录，便于与称重软件同目录运行
DESTDIR = $$PWD/../bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
UI_DIR = $$PWD/build/ui
