QT += core widgets network sql

CONFIG += c++17

TARGET = WeightComparison
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    industrialtheme.cpp \
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
    industrialtheme.h \
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

RESOURCES += resources.qrc

# 发布时备用：exe 旁 themes/industrial.qss（资源加载失败时回退）
themes.files = $$PWD/themes/industrial.qss
themes.path = $$DESTDIR/themes
INSTALLS += themes

# 复用包：与 img/industrial-theme 同步，便于拷贝到其它工业软件
industrial_theme.files = $$PWD/img/industrial-theme/industrial.qss \
                       $$PWD/img/industrial-theme/industrialtheme.h \
                       $$PWD/img/industrial-theme/industrialtheme.cpp \
                       $$PWD/img/industrial-theme/使用说明.txt \
                       $$PWD/img/industrial-theme/color-palette.txt
industrial_theme.path = $$DESTDIR/img/industrial-theme
INSTALLS += industrial_theme

# 默认构建目录
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc
UI_DIR = $$PWD/build/ui
