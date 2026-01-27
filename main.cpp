#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置应用程序信息
    a.setApplicationName("Weight Comparison");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationName("TOYOTA");
    
    MainWindow w;
    w.show();
    
    return a.exec();
}
