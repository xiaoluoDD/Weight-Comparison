#include "mainwindow.h"
#include "logger.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 初始化日志（exe 同级目录/log，按日期保存）
    Logger::init();

    // 设置应用程序信息
    a.setApplicationName("Weight Comparison");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationName("TOYOTA");

    Logger::info("程序启动");

    MainWindow w;
    w.show();

    return a.exec();
}
