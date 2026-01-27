#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QMap>
#include "tcpclient.h"
#include "weightdata.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // TCP连接相关槽函数
    void onConnectClicked();
    void onDisconnectClicked();
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(const QString &error);
    
    // 数据接收处理
    void onDataReceived(const QByteArray &data);
    
    // 物品绑定相关
    void onAddBindingClicked();
    void onRemoveBindingClicked();
    void onClearBindingsClicked();
    
    // 称重数据相关
    void onClearTable1Clicked();
    void onClearTable2Clicked();
    void onExportTable1Clicked();
    void onExportTable2Clicked();

private:
    Ui::MainWindow *ui;
    
    // TCP客户端
    TcpClient *m_tcpClient;
    
    // 数据存储
    QMap<QString, QString> m_bindingMap;  // 指令 -> 物品名称的映射
    QList<WeightData> m_weightDataList1;  // 表格1的数据
    QList<WeightData> m_weightDataList2;  // 表格2的数据
    
    // 初始化函数
    void initializeUI();
    void setupConnections();
    void updateConnectionStatus(bool connected);
    void parseReceivedData(const QByteArray &data);
    void addWeightData(const WeightData &data, int tableIndex = 1);
    void updateWeightTable(QTableWidget *table, const QList<WeightData> &dataList);
    QString getItemNameByCommand(const QString &command);
};

#endif // MAINWINDOW_H
