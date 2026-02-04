#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "slotdialog.h"
#include "ngusedialog.h"
#include "logger.h"
#include "database.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QDateTime>
#include <QFormLayout>
#include <QSplitter>
#include <QFont>
#include <QMouseEvent>
#include <QSet>

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

    // 初始化数据库并加载历史记录、NG记录、车型绑定
    if (DatabaseManager::instance().init()) {
        m_bindingMap = DatabaseManager::instance().loadBindings();
        ui->bindingListWidget->clear();
        for (auto it = m_bindingMap.constBegin(); it != m_bindingMap.constEnd(); ++it)
            ui->bindingListWidget->addItem(QString("%1 -> %2").arg(it.key(), it.value()));
        Logger::info(QString("已从数据库加载车型绑定: %1 条").arg(m_bindingMap.size()));

        m_weightDataList1 = DatabaseManager::instance().loadWeightRecords(1);
        m_weightDataList2 = DatabaseManager::instance().loadWeightRecords(2);
        updateWeightTable(ui->weightTable1, m_weightDataList1);
        updateWeightTable(ui->weightTable2, m_weightDataList2);
        Logger::info(QString("已从数据库加载历史记录: 表格1 %1 条, 表格2 %2 条")
                     .arg(m_weightDataList1.size()).arg(m_weightDataList2.size()));

        QList<QList<QVariant>> ngList = DatabaseManager::instance().loadNgRecords();
        ui->ngTable->setRowCount(ngList.size());
        for (int i = 0; i < ngList.size(); ++i) {
            const QList<QVariant> &row = ngList[i];
            ui->ngTable->setItem(i, 0, new QTableWidgetItem(row.value(0).toString()));
            ui->ngTable->setItem(i, 1, new QTableWidgetItem(row.value(1).toString()));
            ui->ngTable->setItem(i, 2, new QTableWidgetItem(row.value(2).toString()));
            ui->ngTable->setItem(i, 3, new QTableWidgetItem(QString::number(row.value(3).toDouble() * 1000.0, 'f', 1)));  // 显示克
            QDateTime dt = QDateTime::fromString(row.value(4).toString(), Qt::ISODate);
            ui->ngTable->setItem(i, 4, new QTableWidgetItem(dt.toString("yyyy-MM-dd hh:mm:ss")));
        }
        Logger::info(QString("已从数据库加载NG记录: %1 条").arg(ngList.size()));
    }

    // 初始化状态
    updateConnectionStatus(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initializeUI()
{
    // 设置NG品表格列标题和属性
    ui->ngTable->horizontalHeader()->setStretchLastSection(true);
    
    // 设置历史记录表格列宽（不拉伸最后一列，设置固定列宽以便一次性显示所有列）
    setupHistoryTableColumns(ui->weightTable1);
    setupHistoryTableColumns(ui->weightTable2);
    
    // 设置称重数据标签页的布局拉伸比例
    // 列拉伸：左列(0)为1，右列(1)为2（左边窄，右边宽）
    ui->gridLayout_weightData->setColumnStretch(0, 1);
    ui->gridLayout_weightData->setColumnStretch(1, 2);
    // 行拉伸：上下两行各为1
    ui->gridLayout_weightData->setRowStretch(0, 1);
    ui->gridLayout_weightData->setRowStretch(1, 1);
    
    // 设置历史记录标签页的布局拉伸比例（上下两表各为1）
    ui->verticalLayout_history->setStretchFactor(ui->historyLeftGroup, 1);
    ui->verticalLayout_history->setStretchFactor(ui->historyRightGroup, 1);
    
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

    // 设置可视化槽位：居中显示、多行文本（车型/重量/条码）
    QList<QLabel *> slotLabels;
    slotLabels << ui->slot1_1 << ui->slot1_2 << ui->slot1_3 << ui->slot1_4
               << ui->slot1_5 << ui->slot1_6 << ui->slot1_7 << ui->slot1_8
               << ui->slot2_1 << ui->slot2_2 << ui->slot2_3 << ui->slot2_4
               << ui->slot2_5 << ui->slot2_6 << ui->slot2_7 << ui->slot2_8;
    QFont slotFont;
    slotFont.setPointSize(10);
    slotFont.setBold(true);
    for (QLabel *lb : slotLabels) {
        lb->setAlignment(Qt::AlignCenter);
        lb->setFont(slotFont);
        lb->setWordWrap(true);
    }

    setupSlotDoubleClick();

    // NG品表格：确保5列(id,车型,条码,重量,时间)，隐藏id列，连接删除/使用按钮
    if (ui->ngTable->columnCount() < 5) {
        ui->ngTable->setColumnCount(5);
        ui->ngTable->setHorizontalHeaderItem(0, new QTableWidgetItem(QStringLiteral("id")));
        ui->ngTable->setHorizontalHeaderItem(1, new QTableWidgetItem(QStringLiteral("车型名称")));
        ui->ngTable->setHorizontalHeaderItem(2, new QTableWidgetItem(QStringLiteral("条码")));
        ui->ngTable->setHorizontalHeaderItem(3, new QTableWidgetItem(QStringLiteral("重量")));
        ui->ngTable->setHorizontalHeaderItem(4, new QTableWidgetItem(QStringLiteral("时间")));
    }
    ui->ngTable->setColumnHidden(0, true);
    connect(ui->ngDeleteBtn, &QPushButton::clicked, this, &MainWindow::onNgDeleteClicked);
    connect(ui->ngUseBtn, &QPushButton::clicked, this, &MainWindow::onNgUseClicked);
}

void MainWindow::setupHistoryTableColumns(QTableWidget *table)
{
    // 关闭最后一列的自动拉伸
    table->horizontalHeader()->setStretchLastSection(false);
    
    // 列顺序：车型名称、重量1、条码1、…、重量8、条码8、时间（共18列）
    table->setColumnWidth(0, 75);   // 车型名称
    for (int i = 0; i < 8; ++i) {
        table->setColumnWidth(1 + i * 2, 55);     // 重量1-8
        table->setColumnWidth(2 + i * 2, 75);     // 条码1-8
    }
    table->setColumnWidth(17, 130); // 时间
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
    
    // 车型绑定相关
    connect(ui->addBindingBtn, &QPushButton::clicked, this, &MainWindow::onAddBindingClicked);
    connect(ui->removeBindingBtn, &QPushButton::clicked, this, &MainWindow::onRemoveBindingClicked);
    
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
    Logger::info(QString("正在连接 %1:%2").arg(address).arg(port));
}

void MainWindow::onDisconnectClicked()
{
    m_tcpClient->disconnectFromServer();
}

void MainWindow::onConnected()
{
    updateConnectionStatus(true);
    ui->statusbar->showMessage("已连接到服务器", 3000);
    Logger::info("已连接到服务器");
}

void MainWindow::onDisconnected()
{
    updateConnectionStatus(false);
    ui->statusbar->showMessage("已断开连接", 3000);
    Logger::info("已断开连接");
}

void MainWindow::onErrorOccurred(const QString &error)
{
    updateConnectionStatus(false);
    ui->statusbar->showMessage(QString("错误: %1").arg(error), 5000);
    Logger::error(QString("连接错误: %1").arg(error));
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
    m_receiveBuffer.append(data);

    while (m_receiveBuffer.size() >= PlcProtocol::FullPacketSize) {
        QByteArray packet = m_receiveBuffer.left(PlcProtocol::FullPacketSize);
        m_receiveBuffer.remove(0, PlcProtocol::FullPacketSize);

        PlcProtocol::TwoCarPacket twoCar;
        if (!PlcProtocol::parseTwoCarPacket(packet, twoCar))
            continue;

        // 正常生产或生产模式为0时，将重量显示在第一托/第二托可视化对应槽位
        // 注：部分设备可能发送 productionMode=0 表示正常生产
        if (twoCar.car1.productionMode == PlcProtocol::NormalProduction || twoCar.car1.productionMode == 0)
            updateCarVisualization(1, twoCar.car1);
        if (twoCar.car2.productionMode == PlcProtocol::NormalProduction || twoCar.car2.productionMode == 0)
            updateCarVisualization(2, twoCar.car2);

        WeightData w1 = firstCarDataToWeightData(twoCar.car1);
        WeightData w2 = firstCarDataToWeightData(twoCar.car2);
        addWeightData(w1, 1);
        addWeightData(w2, 2);
        Logger::info("解析到一帧2车数据并已写入历史记录");
    }
}

QString MainWindow::vehicleTypeToString(int vehicleType)
{
    if (vehicleType == PlcProtocol::Model12V) return QStringLiteral("12V机型");
    if (vehicleType == PlcProtocol::Model16V) return QStringLiteral("16V机型");
    return vehicleType > 0 ? QString::number(vehicleType) : QString();
}

WeightData MainWindow::firstCarDataToWeightData(const PlcProtocol::FirstCarData &car)
{
    WeightData w;
    w.setTimestamp(QDateTime::currentDateTime());
    w.setVehicleModel(vehicleTypeToString(car.vehicleType));
    w.setWeights(car.weights);
    w.setBarcodes(car.barcodes);
    return w;
}

QString MainWindow::formatSlotText(const QString &vehicleModel, double weightKg, const QString &barcode) const
{
    QString vm = vehicleModel.isEmpty() ? QStringLiteral("-") : vehicleModel;
    QString w = QString::number(weightKg * 1000.0, 'f', 1);
    QString bc = barcode.isEmpty() ? QStringLiteral("-") : barcode;
    return QStringLiteral("%1\n%2\n%3").arg(vm).arg(w).arg(bc);
}

void MainWindow::updateCarVisualization(int carIndex, const PlcProtocol::FirstCarData &car)
{
    if (carIndex == 1)
        m_car1Data = car;
    else if (carIndex == 2)
        m_car2Data = car;
    else
        return;

    QLabel *slotLabels[8];
    if (carIndex == 1) {
        slotLabels[0] = ui->slot1_1; slotLabels[1] = ui->slot1_2; slotLabels[2] = ui->slot1_3; slotLabels[3] = ui->slot1_4;
        slotLabels[4] = ui->slot1_5; slotLabels[5] = ui->slot1_6; slotLabels[6] = ui->slot1_7; slotLabels[7] = ui->slot1_8;
    } else {
        slotLabels[0] = ui->slot2_1; slotLabels[1] = ui->slot2_2; slotLabels[2] = ui->slot2_3; slotLabels[3] = ui->slot2_4;
        slotLabels[4] = ui->slot2_5; slotLabels[5] = ui->slot2_6; slotLabels[6] = ui->slot2_7; slotLabels[7] = ui->slot2_8;
    }
    QString vehicleModel = getItemNameByCommand(QString::number(car.vehicleType));
    if (vehicleModel.isEmpty())
        vehicleModel = vehicleTypeToString(car.vehicleType);
    QList<double> weights = car.weights;
    QList<QString> barcodes = car.barcodes;
    for (int i = 0; i < 8; ++i) {
        double w = (i < weights.size()) ? weights[i] : 0.0;
        QString bc = (i < barcodes.size()) ? barcodes[i] : QString();
        slotLabels[i]->setText(formatSlotText(vehicleModel, w, bc));
    }
    applySlotDeviationStyle(carIndex, weights);
}

void MainWindow::applySlotDeviationStyle(int carIndex, const QList<double> &weights)
{
    QLabel *slotLabels[8];
    if (carIndex == 1) {
        slotLabels[0] = ui->slot1_1; slotLabels[1] = ui->slot1_2; slotLabels[2] = ui->slot1_3; slotLabels[3] = ui->slot1_4;
        slotLabels[4] = ui->slot1_5; slotLabels[5] = ui->slot1_6; slotLabels[6] = ui->slot1_7; slotLabels[7] = ui->slot1_8;
    } else {
        slotLabels[0] = ui->slot2_1; slotLabels[1] = ui->slot2_2; slotLabels[2] = ui->slot2_3; slotLabels[3] = ui->slot2_4;
        slotLabels[4] = ui->slot2_5; slotLabels[5] = ui->slot2_6; slotLabels[6] = ui->slot2_7; slotLabels[7] = ui->slot2_8;
    }
    const double deviationThresholdKg = 0.06;  // 60克 = 0.06 kg

    double w[8];
    for (int i = 0; i < 8; ++i)
        w[i] = (i < weights.size()) ? weights[i] : 0.0;

    QSet<int> redSlots;
    for (int i = 0; i < 8; ++i) {
        for (int j = i + 1; j < 8; ++j) {
            if (qAbs(w[i] - w[j]) >= deviationThresholdKg) {
                redSlots.insert(i);
                redSlots.insert(j);
            }
        }
    }

    for (int i = 0; i < 8; ++i) {
        slotLabels[i]->setStyleSheet(redSlots.contains(i) ? QStringLiteral("color: red;") : QString());
    }
}

void MainWindow::setupSlotDoubleClick()
{
    auto addSlot = [this](QLabel *lb, int carIndex, int slotIndex) {
        m_slotMap[lb] = qMakePair(carIndex, slotIndex);
        lb->installEventFilter(this);
    };
    addSlot(ui->slot1_1, 1, 0); addSlot(ui->slot1_2, 1, 1); addSlot(ui->slot1_3, 1, 2); addSlot(ui->slot1_4, 1, 3);
    addSlot(ui->slot1_5, 1, 4); addSlot(ui->slot1_6, 1, 5); addSlot(ui->slot1_7, 1, 6); addSlot(ui->slot1_8, 1, 7);
    addSlot(ui->slot2_1, 2, 0); addSlot(ui->slot2_2, 2, 1); addSlot(ui->slot2_3, 2, 2); addSlot(ui->slot2_4, 2, 3);
    addSlot(ui->slot2_5, 2, 4); addSlot(ui->slot2_6, 2, 5); addSlot(ui->slot2_7, 2, 6); addSlot(ui->slot2_8, 2, 7);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        QLabel *lb = qobject_cast<QLabel *>(watched);
        if (lb && m_slotMap.contains(lb)) {
            QPair<int, int> p = m_slotMap[lb];
            onSlotDoubleClicked(p.first, p.second);
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onSlotDoubleClicked(int carIndex, int slotIndex)
{
    QLabel *slotLabels[8];
    if (carIndex == 1) {
        slotLabels[0] = ui->slot1_1; slotLabels[1] = ui->slot1_2; slotLabels[2] = ui->slot1_3; slotLabels[3] = ui->slot1_4;
        slotLabels[4] = ui->slot1_5; slotLabels[5] = ui->slot1_6; slotLabels[6] = ui->slot1_7; slotLabels[7] = ui->slot1_8;
    } else {
        slotLabels[0] = ui->slot2_1; slotLabels[1] = ui->slot2_2; slotLabels[2] = ui->slot2_3; slotLabels[3] = ui->slot2_4;
        slotLabels[4] = ui->slot2_5; slotLabels[5] = ui->slot2_6; slotLabels[6] = ui->slot2_7; slotLabels[7] = ui->slot2_8;
    }
    if (slotLabels[slotIndex]->text().trimmed().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("该槽位暂无数据"));
        return;
    }

    const PlcProtocol::FirstCarData &car = (carIndex == 1) ? m_car1Data : m_car2Data;
    QString vehicleModel = vehicleTypeToString(car.vehicleType);
    double weight = (slotIndex < car.weights.size()) ? car.weights[slotIndex] : 0.0;
    QString barcode = (slotIndex < car.barcodes.size()) ? car.barcodes[slotIndex] : QString();

    SlotDialog dlg(vehicleModel, weight, barcode, this);
    if (dlg.exec() == QDialog::Accepted && dlg.addToNgRequested()) {
        addNgRecord(vehicleModel, barcode, weight);
        slotLabels[slotIndex]->setText(QString());
        PlcProtocol::FirstCarData &car = (carIndex == 1) ? m_car1Data : m_car2Data;
        while (car.weights.size() < 8) car.weights.append(0.0);
        while (car.barcodes.size() < 8) car.barcodes.append(QString());
        car.weights[slotIndex] = 0.0;
        car.barcodes[slotIndex] = QString();
        applySlotDeviationStyle(carIndex, car.weights);
        ui->statusbar->showMessage(QStringLiteral("已添加到NG品"), 2000);
        Logger::info(QString("槽位 %1-%2 已添加到NG: 车型=%3, 重量=%4, 条码=%5")
                     .arg(carIndex).arg(slotIndex + 1).arg(vehicleModel).arg(weight).arg(barcode));
    }
}

void MainWindow::addNgRecord(const QString &vehicleModel, const QString &barcode, double weight)
{
    qint64 id = DatabaseManager::instance().insertNgRecord(vehicleModel, barcode, weight);
    if (id < 0) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("NG记录保存到数据库失败"));
        return;
    }
    int row = ui->ngTable->rowCount();
    ui->ngTable->insertRow(row);
    ui->ngTable->setItem(row, 0, new QTableWidgetItem(QString::number(id)));
    ui->ngTable->setItem(row, 1, new QTableWidgetItem(vehicleModel));
    ui->ngTable->setItem(row, 2, new QTableWidgetItem(barcode));
    ui->ngTable->setItem(row, 3, new QTableWidgetItem(QString::number(weight * 1000.0, 'f', 1)));  // 显示克
    ui->ngTable->setItem(row, 4, new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
}

void MainWindow::onNgDeleteClicked()
{
    int row = ui->ngTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的NG品"));
        return;
    }
    QTableWidgetItem *idItem = ui->ngTable->item(row, 0);
    QTableWidgetItem *col1 = ui->ngTable->item(row, 1);
    QTableWidgetItem *col2 = ui->ngTable->item(row, 2);
    QTableWidgetItem *col3 = ui->ngTable->item(row, 3);
    if (!idItem || !col1 || !col2 || !col3) return;
    qint64 id = idItem->text().toLongLong();
    QString info = QStringLiteral("%1 | %2 | %3 g").arg(col1->text()).arg(col2->text()).arg(col3->text());

    int ret = QMessageBox::question(this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除该NG品吗？\n\n%1").arg(info),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes && DatabaseManager::instance().deleteNgRecord(id)) {
        ui->ngTable->removeRow(row);
        ui->statusbar->showMessage(QStringLiteral("已删除NG品"), 2000);
        Logger::info(QString("已删除NG记录 id=%1").arg(id));
    }
}

void MainWindow::onNgUseClicked()
{
    int row = ui->ngTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择要使用的NG品"));
        return;
    }
    QString vehicleModel = ui->ngTable->item(row, 1)->text();
    QString barcode = ui->ngTable->item(row, 2)->text();
    double weightG = ui->ngTable->item(row, 3)->text().toDouble();  // 表格显示为克
    double weight = weightG / 1000.0;  // 转为 kg 供协议和槽位数据使用
    qint64 id = ui->ngTable->item(row, 0)->text().toLongLong();

    NgUseDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    int carIndex = dlg.carIndex();
    int slotIndex = dlg.slotIndex();

    QLabel *slotLabels[8];
    if (carIndex == 1) {
        slotLabels[0] = ui->slot1_1; slotLabels[1] = ui->slot1_2; slotLabels[2] = ui->slot1_3; slotLabels[3] = ui->slot1_4;
        slotLabels[4] = ui->slot1_5; slotLabels[5] = ui->slot1_6; slotLabels[6] = ui->slot1_7; slotLabels[7] = ui->slot1_8;
    } else {
        slotLabels[0] = ui->slot2_1; slotLabels[1] = ui->slot2_2; slotLabels[2] = ui->slot2_3; slotLabels[3] = ui->slot2_4;
        slotLabels[4] = ui->slot2_5; slotLabels[5] = ui->slot2_6; slotLabels[6] = ui->slot2_7; slotLabels[7] = ui->slot2_8;
    }

    if (!slotLabels[slotIndex]->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法添加"),
            QStringLiteral("槽位 %1-%2 已有数据，请选择空槽位").arg(carIndex).arg(slotIndex + 1));
        return;
    }

    slotLabels[slotIndex]->setText(formatSlotText(vehicleModel, weight, barcode));

    PlcProtocol::FirstCarData &car = (carIndex == 1) ? m_car1Data : m_car2Data;
    while (car.weights.size() < 8) car.weights.append(0.0);
    while (car.barcodes.size() < 8) car.barcodes.append(QString());
    car.weights[slotIndex] = weight;
    car.barcodes[slotIndex] = barcode;
    car.vehicleType = vehicleModelToType(vehicleModel);

    applySlotDeviationStyle(carIndex, car.weights);

    if (DatabaseManager::instance().deleteNgRecord(id)) {
        ui->ngTable->removeRow(row);
        ui->statusbar->showMessage(QStringLiteral("已使用NG品并放入槽位"), 2000);
        Logger::info(QString("NG品已放入 托%1 槽位%2: %3").arg(carIndex).arg(slotIndex + 1).arg(barcode));
    }
}

int MainWindow::vehicleModelToType(const QString &model)
{
    if (model.contains(QStringLiteral("12V"))) return PlcProtocol::Model12V;
    if (model.contains(QStringLiteral("16V"))) return PlcProtocol::Model16V;
    return PlcProtocol::Model12V;
}

void MainWindow::addWeightData(const WeightData &data, int tableIndex)
{
    DatabaseManager::instance().insertWeightRecord(data, tableIndex);
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
    QList<QString> barcodes;

    for (int i = 0; i < dataList.size(); ++i) {
        const WeightData &data = dataList[i];
        QList<double> weights = data.weights();
        barcodes = data.barcodes();

        int col = 0;
        table->setItem(i, col++, new QTableWidgetItem(data.vehicleModel()));
        for (int j = 0; j < 8; ++j) {
            double weight = (j < weights.size()) ? weights[j] : 0.0;
            QString barcode = (j < barcodes.size()) ? barcodes[j] : QString();
            table->setItem(i, col++, new QTableWidgetItem(QString::number(weight * 1000.0, 'f', 1)));  // 显示克
            table->setItem(i, col++, new QTableWidgetItem(barcode));
        }
        table->setItem(i, col++, new QTableWidgetItem(data.timestamp().toString("yyyy-MM-dd hh:mm:ss")));
    }
}

QString MainWindow::getItemNameByCommand(const QString &command)
{
    // 根据车型代码查找绑定的车型名称
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
        QMessageBox::warning(this, "警告", "请输入车型代码和车型名称！");
        return;
    }
    
    if (m_bindingMap.contains(command)) {
        int ret = QMessageBox::question(this, "确认", 
                                        QString("车型代码 '%1' 已存在，是否覆盖？").arg(command),
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No) {
            return;
        }
    }
    
    if (!DatabaseManager::instance().insertBinding(command, itemName)) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("车型绑定保存到数据库失败"));
        return;
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
        if (DatabaseManager::instance().deleteBinding(command)) {
            m_bindingMap.remove(command);
            delete item;
            ui->statusbar->showMessage("已删除绑定", 2000);
        } else {
            QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("从数据库删除绑定失败"));
        }
    }
}

// ========== 称重数据表格相关槽函数 ==========
void MainWindow::onClearTable1Clicked()
{
    int ret = QMessageBox::question(this, "确认", "确定要清空表格1的所有数据吗？",
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        DatabaseManager::instance().clearTableRecords(1);
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
        DatabaseManager::instance().clearTableRecords(2);
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
