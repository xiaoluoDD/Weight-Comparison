#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QDateTime>
#include <QFormLayout>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_tcpClient(nullptr)
{
    ui->setupUi(this);
    
    // 创建TCP客户端
    m_tcpClient = new TcpClient(this);
    
    // 初始化UI设置
    initializeUI();
    
    setupConnections();
    
    // 初始化状态
    updateConnectionStatus(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initializeUI()
{
    // 设置表格列标题和属性
    ui->weightTable1->horizontalHeader()->setStretchLastSection(true);
    ui->weightTable2->horizontalHeader()->setStretchLastSection(true);
    
    // 设置称重数据标签页的布局拉伸比例
    // 列拉伸：左列(0)为1，右列(1)为2（左边窄，右边宽）
    ui->gridLayout_weightData->setColumnStretch(0, 1);
    ui->gridLayout_weightData->setColumnStretch(1, 2);
    // 行拉伸：上下两行各为1
    ui->gridLayout_weightData->setRowStretch(0, 1);
    ui->gridLayout_weightData->setRowStretch(1, 1);
    
    // 设置左侧可视化区域的布局拉伸比例
    // 第一托可视化：2列各为1，4行各为1
    ui->gridLayout_leftTop->setColumnStretch(0, 1);
    ui->gridLayout_leftTop->setColumnStretch(1, 1);
    ui->gridLayout_leftTop->setRowStretch(0, 1);
    ui->gridLayout_leftTop->setRowStretch(1, 1);
    ui->gridLayout_leftTop->setRowStretch(2, 1);
    ui->gridLayout_leftTop->setRowStretch(3, 1);
    
    // 第二托可视化：2列各为1，4行各为1
    ui->gridLayout_leftBottom->setColumnStretch(0, 1);
    ui->gridLayout_leftBottom->setColumnStretch(1, 1);
    ui->gridLayout_leftBottom->setRowStretch(0, 1);
    ui->gridLayout_leftBottom->setRowStretch(1, 1);
    ui->gridLayout_leftBottom->setRowStretch(2, 1);
    ui->gridLayout_leftBottom->setRowStretch(3, 1);
}

void MainWindow::setupConnections()
{
    // TCP连接相关
    connect(ui->connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_tcpClient, &TcpClient::connected, this, &MainWindow::onConnected);
    connect(m_tcpClient, &TcpClient::disconnected, this, &MainWindow::onDisconnected);
    connect(m_tcpClient, &TcpClient::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(m_tcpClient, &TcpClient::dataReceived, this, &MainWindow::onDataReceived);
    
    // 物品绑定相关
    connect(ui->addBindingBtn, &QPushButton::clicked, this, &MainWindow::onAddBindingClicked);
    connect(ui->removeBindingBtn, &QPushButton::clicked, this, &MainWindow::onRemoveBindingClicked);
    connect(ui->clearBindingsBtn, &QPushButton::clicked, this, &MainWindow::onClearBindingsClicked);
    
    // 称重数据表格相关
    connect(ui->clearTable1Btn, &QPushButton::clicked, this, &MainWindow::onClearTable1Clicked);
    connect(ui->clearTable2Btn, &QPushButton::clicked, this, &MainWindow::onClearTable2Clicked);
    connect(ui->exportTable1Btn, &QPushButton::clicked, this, &MainWindow::onExportTable1Clicked);
    connect(ui->exportTable2Btn, &QPushButton::clicked, this, &MainWindow::onExportTable2Clicked);
}

void MainWindow::onConnectClicked()
{
    QString address = ui->serverAddressEdit->text().trimmed();
    int port = ui->serverPortEdit->text().toInt();
    
    if (address.isEmpty() || port <= 0) {
        QMessageBox::warning(this, "警告", "请输入有效的服务器地址和端口！");
        return;
    }
    
    m_tcpClient->connectToServer(address, port);
    ui->statusbar->showMessage(QString("正在连接到 %1:%2...").arg(address).arg(port));
}

void MainWindow::onDisconnectClicked()
{
    m_tcpClient->disconnectFromServer();
}

void MainWindow::onConnected()
{
    updateConnectionStatus(true);
    ui->statusbar->showMessage("已连接到服务器", 3000);
}

void MainWindow::onDisconnected()
{
    updateConnectionStatus(false);
    ui->statusbar->showMessage("已断开连接", 3000);
}

void MainWindow::onErrorOccurred(const QString &error)
{
    updateConnectionStatus(false);
    ui->statusbar->showMessage(QString("错误: %1").arg(error), 5000);
    QMessageBox::critical(this, "连接错误", error);
}

void MainWindow::onDataReceived(const QByteArray &data)
{
    parseReceivedData(data);
    ui->statusbar->showMessage(QString("收到数据: %1 字节").arg(data.size()), 2000);
}

void MainWindow::updateConnectionStatus(bool connected)
{
    ui->connectBtn->setEnabled(!connected);
    ui->disconnectBtn->setEnabled(connected);
    ui->serverAddressEdit->setEnabled(!connected);
    ui->serverPortEdit->setEnabled(!connected);
    
    if (connected) {
        ui->connectionStatusLabel->setText("已连接");
        ui->connectionStatusLabel->setStyleSheet("color: green; font-weight: bold;");
    } else {
        ui->connectionStatusLabel->setText("未连接");
        ui->connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    }
}

void MainWindow::parseReceivedData(const QByteArray &data)
{
    // TODO: 根据实际协议解析数据
    // 这里先提供一个简单的示例框架
    QString dataStr = QString::fromUtf8(data).trimmed();
    
    // 示例：假设数据格式为 "指令,重量值,单位,状态"
    // 或者根据指令查找绑定的物品名称
    QStringList parts = dataStr.split(',');
    
    WeightData weightData;
    weightData.setTimestamp(QDateTime::currentDateTime());
    
    if (parts.size() >= 2) {
        // 假设第一部分是指令
        QString command = parts[0].trimmed();
        QString itemName = getItemNameByCommand(command);
        
        weightData.setItemName(itemName);
        weightData.setValue(parts[1].toDouble());
        weightData.setUnit(parts.size() > 2 ? parts[2].trimmed() : "kg");
        weightData.setStatus(parts.size() > 3 ? parts[3].trimmed() : "正常");
        
        // 这里可以根据指令或其他逻辑决定添加到哪个表格
        // 暂时默认添加到表格1
        addWeightData(weightData, 1);
    } else {
        // 简化处理：直接解析为重量值
        weightData.setValue(dataStr.toDouble());
        weightData.setUnit("kg");
        weightData.setStatus("正常");
        weightData.setItemName("");
        addWeightData(weightData, 1);
    }
}

void MainWindow::addWeightData(const WeightData &data, int tableIndex)
{
    if (tableIndex == 1) {
        m_weightDataList1.append(data);
        updateWeightTable(ui->weightTable1, m_weightDataList1);
    } else if (tableIndex == 2) {
        m_weightDataList2.append(data);
        updateWeightTable(ui->weightTable2, m_weightDataList2);
    }
}

void MainWindow::updateWeightTable(QTableWidget *table, const QList<WeightData> &dataList)
{
    table->setRowCount(dataList.size());
    
    for (int i = 0; i < dataList.size(); ++i) {
        const WeightData &data = dataList[i];
        QList<double> weights = data.weights();
        
        int col = 0;
        // 序号
        table->setItem(i, col++, new QTableWidgetItem(QString::number(i + 1)));
        // 车型名称
        table->setItem(i, col++, new QTableWidgetItem(data.vehicleModel()));
        // 条码
        table->setItem(i, col++, new QTableWidgetItem(data.barcode()));
        // 8个重量列（重量1-8）
        for (int j = 0; j < 8; ++j) {
            double weight = (j < weights.size()) ? weights[j] : 0.0;
            table->setItem(i, col++, new QTableWidgetItem(QString::number(weight, 'f', 3)));
        }
        // 时间
        table->setItem(i, col++, new QTableWidgetItem(data.timestamp().toString("yyyy-MM-dd hh:mm:ss")));
    }
}

QString MainWindow::getItemNameByCommand(const QString &command)
{
    // 根据指令查找绑定的物品名称
    if (m_bindingMap.contains(command)) {
        return m_bindingMap[command];
    }
    return "";  // 未找到绑定
}

// ========== 物品绑定相关槽函数 ==========
void MainWindow::onAddBindingClicked()
{
    QString command = ui->commandEdit->text().trimmed();
    QString itemName = ui->itemNameEdit->text().trimmed();
    
    if (command.isEmpty() || itemName.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入指令和物品名称！");
        return;
    }
    
    if (m_bindingMap.contains(command)) {
        int ret = QMessageBox::question(this, "确认", 
                                        QString("指令 '%1' 已存在，是否覆盖？").arg(command),
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No) {
            return;
        }
    }
    
    m_bindingMap[command] = itemName;
    
    // 更新列表显示
    ui->bindingListWidget->clear();
    QMapIterator<QString, QString> it(m_bindingMap);
    while (it.hasNext()) {
        it.next();
        ui->bindingListWidget->addItem(QString("%1 -> %2").arg(it.key(), it.value()));
    }
    
    ui->commandEdit->clear();
    ui->itemNameEdit->clear();
    ui->statusbar->showMessage(QString("已添加绑定: %1 -> %2").arg(command, itemName), 2000);
}

void MainWindow::onRemoveBindingClicked()
{
    QListWidgetItem *item = ui->bindingListWidget->currentItem();
    if (!item) {
        QMessageBox::information(this, "提示", "请先选择要删除的绑定项！");
        return;
    }
    
    QString text = item->text();
    QString command = text.split(" -> ").first();
    
    int ret = QMessageBox::question(this, "确认", 
                                    QString("确定要删除绑定 '%1' 吗？").arg(text),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_bindingMap.remove(command);
        delete item;
        ui->statusbar->showMessage("已删除绑定", 2000);
    }
}

void MainWindow::onClearBindingsClicked()
{
    int ret = QMessageBox::question(this, "确认", "确定要清空所有绑定吗？",
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_bindingMap.clear();
        ui->bindingListWidget->clear();
        ui->statusbar->showMessage("已清空所有绑定", 2000);
    }
}

// ========== 称重数据表格相关槽函数 ==========
void MainWindow::onClearTable1Clicked()
{
    int ret = QMessageBox::question(this, "确认", "确定要清空表格1的所有数据吗？",
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_weightDataList1.clear();
        updateWeightTable(ui->weightTable1, m_weightDataList1);
        ui->statusbar->showMessage("表格1数据已清空", 2000);
    }
}

void MainWindow::onClearTable2Clicked()
{
    int ret = QMessageBox::question(this, "确认", "确定要清空表格2的所有数据吗？",
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_weightDataList2.clear();
        updateWeightTable(ui->weightTable2, m_weightDataList2);
        ui->statusbar->showMessage("表格2数据已清空", 2000);
    }
}

void MainWindow::onExportTable1Clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出表格1数据", "", 
                                                     "CSV文件 (*.csv);;所有文件 (*.*)");
    if (fileName.isEmpty()) {
        return;
    }
    
    // TODO: 实现导出功能
    QMessageBox::information(this, "提示", "导出功能待实现");
}

void MainWindow::onExportTable2Clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出表格2数据", "", 
                                                     "CSV文件 (*.csv);;所有文件 (*.*)");
    if (fileName.isEmpty()) {
        return;
    }
    
    // TODO: 实现导出功能
    QMessageBox::information(this, "提示", "导出功能待实现");
}
