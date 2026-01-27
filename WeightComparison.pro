QT += core widgets network

CONFIG += c++17

TARGET = WeightComparison
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    tcpclient.cpp \
    weightdata.cpp

HEADERS += \
    mainwindow.h \
    tcpclient.h \
    weightdata.h

FORMS += \
    mainwindow.ui

# 默认构建目录
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc
UI_DIR = $$PWD/build/ui
