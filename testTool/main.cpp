#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle(QStringLiteral("PLC 模拟器 - 称重数据 TCP 服务端"));
    w.show();
    return a.exec();
}
