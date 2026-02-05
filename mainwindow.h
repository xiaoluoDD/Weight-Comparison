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
#include <QPair>
#include <QTimer>
#include "tcpclient.h"
#include "weightdata.h"
#include "plcprotocol.h"

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
    
    // 车型绑定相关
    void onAddBindingClicked();
    void onRemoveBindingClicked();
    
    // 称重数据相关
    void onClearTable1Clicked();
    void onClearTable2Clicked();
    void onExportTable1Clicked();
    void onExportTable2Clicked();

    // 槽位双击
    void onSlotDoubleClicked(int carIndex, int slotIndex);

    // NG品表格操作
    void onNgAddClicked();
    void onProductionSupplementClicked();
    void onNgDeleteClicked();
    void onNgUseClicked();

    // 当前表格完成操作
    void onCompleteCurrent1Clicked();
    void onCompleteCurrent2Clicked();

    // 系统设置保存
    void onSaveDeviationClicked();

    void onDetectionOkTimerFired();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::MainWindow *ui;
    
    // TCP客户端
    TcpClient *m_tcpClient;
    
    // 数据存储
    QMap<QString, QString> m_bindingMap;  // 车型代码 -> 车型名称的映射
    QList<WeightData> m_weightDataList1;  // 表格1的数据
    QList<WeightData> m_weightDataList2;  // 表格2的数据
    QByteArray m_receiveBuffer;           // TCP 接收缓冲，凑满 428 字节再解析
    PlcProtocol::FirstCarData m_car1Data; // 第一托当前数据（用于双击弹窗）
    PlcProtocol::FirstCarData m_car2Data; // 第二托当前数据（用于双击弹窗）
    QMap<QLabel *, QPair<int, int>> m_slotMap;  // QLabel* -> (carIndex, slotIndex)
    int m_lastSupplementTrayIndex = 0;   // 上次发送补充请求的托(1或2)，用于识别PLC返回
    int m_lastSupplementQuantity = 0;   // 上次请求的补充数量，用于读取前N个工件
    bool m_lastCar1HadDeviation = false; // 第一托上次是否有超差，用于检测OK发送
    QTimer *m_detectionOkTimer = nullptr;  // 延迟发送OK，避免连续放入NG时过早发送

    // 初始化函数
    void initializeUI();
    void setupConnections();
    void setupHistoryTableColumns(QTableWidget *table);
    void setupCurrentTableColumns(QTableWidget *table);
    void updateConnectionStatus(bool connected);
    void parseReceivedData(const QByteArray &data);
    void addWeightData(const WeightData &data, int tableIndex = 1);
    void appendToCurrentTable(const WeightData &data, int carIndex);  // 追加到当前表格（不写入历史）
    WeightData currentTableRowToWeightData(QTableWidget *table, int row);  // 当前表格行转WeightData
    void completeCurrentTable(QTableWidget *table, int tableIndex);  // 完成：发送到历史、保存、清空
    void updateWeightTable(QTableWidget *table, const QList<WeightData> &dataList);
    QString getItemNameByCommand(const QString &command);
    QString vehicleTypeToString(int vehicleType);  // 1=12V机型 2=16V机型
    int vehicleModelToType(const QString &model);  // 车型名称转车型代码
    WeightData firstCarDataToWeightData(const PlcProtocol::FirstCarData &car);
    void updateCarVisualization(int carIndex, const PlcProtocol::FirstCarData &car);  // 正常生产时更新可视化槽位
    void applySlotDeviationStyle(int carIndex, const QList<double> &weights);  // 根据偏差设置槽位字体颜色
    void refreshAllVisualizationDeviation();  // 每次物品移动后刷新两托的60g偏差判断
    QString formatSlotText(const QString &vehicleModel, double weightKg, const QString &barcode) const;  // 车型/重量/条码换行显示
    void setupSlotDoubleClick();  // 为可视化槽位安装双击事件过滤
    void addNgRecord(const QString &vehicleModel, const QString &barcode, double weightG);  // 添加到NG表格并保存(克)
    void clearCurrentTableSlot(int carIndex, int slotIndex);  // 可视化添加到NG时，同步清空当前表格对应槽位
    void clearCarDataAndVisualization(int carIndex);  // 生产补充时清空指定托的所有数据和可视化
    void doCompleteAndClearBothTrays();  // 保存两托到历史、清空表格和可视化
    void mergeSupplementIntoCar(int carIndex, const PlcProtocol::FirstCarData &car, int count);  // 将PLC返回的补充生产前N个工件合并到指定托
    void updateCurrentTableSlot(int carIndex, int slotIndex, const QString &vehicleModel, double weightG, const QString &barcode);  // 备用调入时，同步更新当前表格(克)
    void applyCurrentTableDeviationStyle(int carIndex, int row, const QList<double> &weights);  // 当前表格偏差格显示红色
};

#endif // MAINWINDOW_H
