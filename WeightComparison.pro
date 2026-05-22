QT += core widgets network sql

CONFIG += c++17

TARGET = WeightComparison
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    visualizationslotlabel.cpp \
    ngadddialog.cpp \
    ngusedialog.cpp \
    supplementdialog.cpp \
    slotdialog.cpp \
    tcpclient.cpp \
    weightdata.cpp \
    plcprotocol.cpp \
    logger.cpp \
    database.cpp

HEADERS += \
    mainwindow.h \
    visualizationslotlabel.h \
    ngadddialog.h \
    ngusedialog.h \
    supplementdialog.h \
    slotdialog.h \
    tcpclient.h \
    weightdata.h \
    plcprotocol.h \
    logger.h \
    database.h

FORMS += \
    mainwindow.ui

# 默认构建目录
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc
UI_DIR = $$PWD/build/ui
