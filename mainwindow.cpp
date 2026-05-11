#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "slotdialog.h"
#include "ngadddialog.h"
#include "ngusedialog.h"
#include "supplementdialog.h"
#include "logger.h"
#include "database.h"
#include <QApplication>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QDateTime>
#include <QFormLayout>
#include <QSplitter>
#include <QFont>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QSet>
#include <QBrush>
#include <QVector>
#include <algorithm>

// 空槽位阈值，重量<=此值视为空，不参与偏差比较
static const double EmptySlotThreshold = 0.0001;

static QSet<int> computeRedSlotsCar2(const double w1[8], const double w2[8], double thresholdG);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_tcpClient(nullptr)
{
    ui->setupUi(this);
    
    // 创建TCP客户端
    m_tcpClient = new TcpClient(this);

    // 检测OK延迟发送定时器（2秒稳定后再发，避免连续放入NG时过早发送）
    m_detectionOkTimer = new QTimer(this);
    m_detectionOkTimer->setSingleShot(true);
    connect(m_detectionOkTimer, &QTimer::timeout, this, &MainWindow::onDetectionOkTimerFired);

    // 长按1号槽位5秒清空
    m_longPressTimer = new QTimer(this);
    m_longPressTimer->setSingleShot(true);
    connect(m_longPressTimer, &QTimer::timeout, this, &MainWindow::onLongPressTimerFired);

    // 初始化UI设置
    initializeUI();
    
    setupConnections();

    // 初始化数据库并加载历史记录、NG记录、车型绑定
    if (!DatabaseManager::instance().init()) {
        QMessageBox::critical(this, QStringLiteral("数据库错误"),
            QStringLiteral("数据库初始化失败，NG品添加、历史记录等功能将不可用。\n请检查程序目录是否有写权限。"));
        Logger::error("数据库初始化失败，部分功能受限");
    } else {
        m_bindingMap = DatabaseManager::instance().loadBindings();
        ui->bindingTable->setRowCount(m_bindingMap.size());
        int row = 0;
        for (auto it = m_bindingMap.constBegin(); it != m_bindingMap.constEnd(); ++it, ++row) {
            ui->bindingTable->setItem(row, 0, new QTableWidgetItem(it.key()));
            ui->bindingTable->setItem(row, 1, new QTableWidgetItem(it.value()));
        }
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
            ui->ngTable->setItem(i, 3, new QTableWidgetItem(QString::number(row.value(3).toDouble(), 'f', 1)));  // 数据库存克(g)
            QDateTime dt = QDateTime::fromString(row.value(4).toString(), Qt::ISODate);
            ui->ngTable->setItem(i, 4, new QTableWidgetItem(dt.toString("yyyy-MM-dd hh:mm:ss")));
        }
        Logger::info(QString("已从数据库加载NG记录: %1 条").arg(ngList.size()));

        // 加载系统设置
        ui->serverAddressEdit->setText(DatabaseManager::instance().getSetting("server_address", "127.0.0.1").toString());
        ui->serverPortEdit->setText(DatabaseManager::instance().getSetting("server_port", "8080").toString());
        ui->deviationThresholdSpinBox->setValue(DatabaseManager::instance().getSetting("deviation_threshold_g", 60.0).toDouble());
        Logger::info("已从数据库加载系统设置");
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
    // 系统设置区域：TCP连接和偏差参数不拉伸，保持正常大小
    ui->verticalLayout_settings->setStretchFactor(ui->connectionGroup, 0);
    ui->verticalLayout_settings->setStretchFactor(ui->deviationGroup, 0);

    // 设置NG品表格列标题和属性
    ui->ngTable->horizontalHeader()->setStretchLastSection(true);

    // 设置车型绑定表格列宽
    ui->bindingTable->horizontalHeader()->setStretchLastSection(true);
    
    // 设置历史记录表格列宽（不拉伸最后一列，设置固定列宽以便一次性显示所有列）
    setupHistoryTableColumns(ui->weightTable1);
    setupHistoryTableColumns(ui->weightTable2);
    // 第一托/第二托当前表格：与历史表格相同的列结构
    setupCurrentTableColumns(ui->extraTable1);
    setupCurrentTableColumns(ui->extraTable2);
    
    // 设置称重数据标签页的布局拉伸比例
    // 列拉伸：左列(0)为1，右列(1)为2（左边窄，右边宽）
    ui->gridLayout_weightData->setColumnStretch(0, 1);
    ui->gridLayout_weightData->setColumnStretch(1, 2);
    // 行拉伸：上下两行各为1
    ui->gridLayout_weightData->setRowStretch(0, 1);
    ui->gridLayout_weightData->setRowStretch(1, 1);
    // 右侧区域：NG品表格占更多空间，当前表格区域降低高度（基本只有1条数据）
    ui->verticalLayout_right->setStretchFactor(ui->ngGroupBox, 2);
    ui->verticalLayout_right->setStretchFactor(ui->extraTable1Group, 1);
    ui->verticalLayout_right->setStretchFactor(ui->extraTable2Group, 1);
    // 当前表格最大高度限制，便于显示1-2行
    ui->extraTable1->setMaximumHeight(140);
    ui->extraTable2->setMaximumHeight(140);
    
    // 设置历史记录标签页的布局拉伸比例（上下两表各为1）
    ui->verticalLayout_history->setStretchFactor(ui->historyLeftGroup, 1);
    ui->verticalLayout_history->setStretchFactor(ui->historyRightGroup, 1);
    
    // 第一托可视化：4 列 — 左备注(0)、左槽(1)、右槽(2)、右备注(3)；仅中间两列平分宽度
    ui->gridLayout_leftTop->setColumnStretch(0, 0);
    ui->gridLayout_leftTop->setColumnStretch(1, 1);
    ui->gridLayout_leftTop->setColumnStretch(2, 1);
    ui->gridLayout_leftTop->setColumnStretch(3, 0);
    {
        const QList<QLabel *> tray1Notes = {
            ui->slot1_leftNote1, ui->slot1_leftNote2, ui->slot1_leftNote3, ui->slot1_leftNote4,
            ui->slot1_rightNote1, ui->slot1_rightNote2, ui->slot1_rightNote3, ui->slot1_rightNote4
        };
        for (QLabel *lb : tray1Notes) {
            lb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            lb->setMinimumWidth(36);
            lb->setMaximumWidth(44);
        }
    }
    ui->gridLayout_leftTop->setRowStretch(0, 1);
    ui->gridLayout_leftTop->setRowStretch(1, 1);
    ui->gridLayout_leftTop->setRowStretch(2, 1);
    ui->gridLayout_leftTop->setRowStretch(3, 1);
    
    // 第二托可视化：与第一托相同 4 列布局
    ui->gridLayout_leftBottom->setColumnStretch(0, 0);
    ui->gridLayout_leftBottom->setColumnStretch(1, 1);
    ui->gridLayout_leftBottom->setColumnStretch(2, 1);
    ui->gridLayout_leftBottom->setColumnStretch(3, 0);
    {
        const QList<QLabel *> tray2Notes = {
            ui->slot2_leftNote1, ui->slot2_leftNote2, ui->slot2_leftNote3, ui->slot2_leftNote4,
            ui->slot2_rightNote1, ui->slot2_rightNote2, ui->slot2_rightNote3, ui->slot2_rightNote4
        };
        for (QLabel *lb : tray2Notes) {
            lb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            lb->setMinimumWidth(36);
            lb->setMaximumWidth(44);
        }
    }
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
    connect(ui->ngAddBtn, &QPushButton::clicked, this, &MainWindow::onNgAddClicked);
    connect(ui->productionSupplementBtn, &QPushButton::clicked, this, &MainWindow::onProductionSupplementClicked);
    connect(ui->ngDeleteBtn, &QPushButton::clicked, this, &MainWindow::onNgDeleteClicked);
    connect(ui->ngUseBtn, &QPushButton::clicked, this, &MainWindow::onNgUseClicked);
    connect(ui->completeCurrent1Btn, &QPushButton::clicked, this, &MainWindow::onCompleteCurrent1Clicked);
    connect(ui->completeCurrent2Btn, &QPushButton::clicked, this, &MainWindow::onCompleteCurrent2Clicked);
    connect(ui->deviationThresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { refreshAllVisualizationDeviation(); });
    connect(ui->saveDeviationBtn, &QPushButton::clicked, this, &MainWindow::onSaveDeviationClicked);
    connect(ui->serverAddressEdit, &QLineEdit::editingFinished,
            this, [this]() { DatabaseManager::instance().setSetting("server_address", ui->serverAddressEdit->text()); });
    connect(ui->serverPortEdit, &QLineEdit::editingFinished,
            this, [this]() { DatabaseManager::instance().setSetting("server_port", ui->serverPortEdit->text()); });
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

void MainWindow::setupCurrentTableColumns(QTableWidget *table)
{
    if (table->columnCount() < 18) {
        table->setColumnCount(18);
        table->setHorizontalHeaderItem(0, new QTableWidgetItem(QStringLiteral("车型名称")));
        for (int i = 0; i < 8; ++i) {
            table->setHorizontalHeaderItem(1 + i * 2, new QTableWidgetItem(QString("重量%1").arg(i + 1)));
            table->setHorizontalHeaderItem(2 + i * 2, new QTableWidgetItem(QString("条码%1").arg(i + 1)));
        }
        table->setHorizontalHeaderItem(17, new QTableWidgetItem(QStringLiteral("时间")));
    }
    table->horizontalHeader()->setStretchLastSection(false);
    table->setColumnWidth(0, 75);
    for (int i = 0; i < 8; ++i) {
        table->setColumnWidth(1 + i * 2, 55);
        table->setColumnWidth(2 + i * 2, 75);
    }
    table->setColumnWidth(17, 130);
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
    m_receiveBuffer.clear();  // 新连接清空缓冲，避免粘包错位
    updateConnectionStatus(true);
    ui->statusbar->showMessage("已连接到服务器", 3000);
    Logger::info("已连接到服务器");
}

void MainWindow::onDisconnected()
{
    m_receiveBuffer.clear();
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

void MainWindow::sendDataWithLog(const QByteArray &data)
{
    QString hexStr = QString::fromLatin1(data.toHex(' ').toUpper());
    Logger::info(QString("发送指令 [%1 字节]: %2").arg(data.size()).arg(hexStr));
    m_tcpClient->sendData(data);
}

void MainWindow::onDataReceived(const QByteArray &data)
{
    // 打印接收指令到日志（十六进制，便于排查）
    QString hexStr = QString::fromLatin1(data.toHex(' ').toUpper());
    Logger::info(QString("接收指令 [%1 字节]: %2").arg(data.size()).arg(hexStr));
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

static bool isCarDataEmpty(const PlcProtocol::FirstCarData &car)
{
    if (car.assemblyDone != 1) return true;
    for (double w : car.weights) {
        if (w > 0.0001) return false;
    }
    return true;  // 完成标志有但重量全0，视为空
}

void MainWindow::parseReceivedData(const QByteArray &data)
{
    // 长度不是396整数倍的包直接舍弃，避免影响后续解析
    if (data.size() % PlcProtocol::FullPacketSize != 0) {
        Logger::warning(QString("收到长度%1字节，非396整数倍，已舍弃").arg(data.size()));
        return;
    }
    m_receiveBuffer.append(data);

    while (m_receiveBuffer.size() >= PlcProtocol::FullPacketSize) {
        QByteArray packet = m_receiveBuffer.left(PlcProtocol::FullPacketSize);
        m_receiveBuffer.remove(0, PlcProtocol::FullPacketSize);

        PlcProtocol::TwoCarPacket twoCar;
        if (!PlcProtocol::parseTwoCarPacket(packet, twoCar)) {
            Logger::warning(QString("解析%1字节失败，丢弃首包尝试重同步。首6字节(车1头): %2 %3 %4 %5 %6 %7")
                .arg(PlcProtocol::FullPacketSize)
                .arg(quint8(packet[0]), 2, 16, QChar('0'))
                .arg(quint8(packet[1]), 2, 16, QChar('0'))
                .arg(quint8(packet[2]), 2, 16, QChar('0'))
                .arg(quint8(packet[3]), 2, 16, QChar('0'))
                .arg(quint8(packet[4]), 2, 16, QChar('0'))
                .arg(quint8(packet[5]), 2, 16, QChar('0')));
            continue;
        }

        // 补充生产：识别 productionMode==2，读取对应托的前N个工件并合并
        bool car1Supplement = (twoCar.car1.productionMode == PlcProtocol::SupplementProduction);
        bool car2Supplement = (twoCar.car2.productionMode == PlcProtocol::SupplementProduction);
        if (car1Supplement || car2Supplement) {
            int trayIndex = car1Supplement ? 1 : 2;
            const PlcProtocol::FirstCarData &car = car1Supplement ? twoCar.car1 : twoCar.car2;
            int count = m_lastSupplementQuantity;
            if (count <= 0) count = 8;  // 未记录时取前8个
            mergeSupplementIntoCar(trayIndex, car, count);
            Logger::info(QString("解析到补充生产数据: 第%1托 前%2个工件").arg(trayIndex).arg(count));
            continue;
        }

        // 正常生产：通过完成标志(assemblyDone)识别是哪一托
        bool car1Valid = (twoCar.car1.assemblyDone == 1) && (twoCar.car1.productionMode == PlcProtocol::NormalProduction || twoCar.car1.productionMode == 0);
        bool car2Valid = (twoCar.car2.assemblyDone == 1) && (twoCar.car2.productionMode == PlcProtocol::NormalProduction || twoCar.car2.productionMode == 0);
        bool car2Empty = isCarDataEmpty(twoCar.car2);

        if (car1Valid && car2Empty) {
            // 第一托数据：第二托为空，只更新第一托
            updateCarVisualization(1, twoCar.car1);
            WeightData w1 = firstCarDataToWeightData(twoCar.car1);
            appendToCurrentTable(w1, 1);
            Logger::info("解析到第一托数据并已写入当前表格");
        } else if (car2Valid) {
            // 第二托数据：第一托已有不再变，只更新第二托
            updateCarVisualization(2, twoCar.car2);
            WeightData w2 = firstCarDataToWeightData(twoCar.car2);
            appendToCurrentTable(w2, 2);
            Logger::info("解析到第二托数据并已写入当前表格");
            // 只把正常数据（非红）与最大最小比较，异常值不参与
            if (m_displayMaxWeight >= 0 || m_displayMinWeight >= 0) {
                double w1[8], w2arr[8];
                for (int i = 0; i < 8; ++i) {
                    w1[i] = (i < m_car1Data.weights.size()) ? m_car1Data.weights[i] : 0.0;
                    w2arr[i] = (i < twoCar.car2.weights.size()) ? twoCar.car2.weights[i] : 0.0;
                }
                QSet<int> red2 = computeRedSlotsCar2(w1, w2arr, ui->deviationThresholdSpinBox->value());
                bool updated = false;
                for (int i = 0; i < 8; ++i) {
                    if (red2.contains(i) || w2arr[i] <= EmptySlotThreshold) continue;  // 跳过异常和空槽
                    double w = w2arr[i];
                    if (m_displayMaxWeight >= 0 && w > m_displayMaxWeight) {
                        m_displayMaxWeight = w;
                        updated = true;
                    }
                    if (m_displayMinWeight >= 0 && w < m_displayMinWeight) {
                        m_displayMinWeight = w;
                        updated = true;
                    }
                }
                if (updated) updateWeightRangeDisplay();
            }
        } else {
            // 不满足 car1Valid&&car2Empty 也不满足 car2Valid：记录忽略原因便于排查
            // 调试：打印实际解析用的前6字节，排查粘包错位
            Logger::info(QString("指令已解析但未处理: 车1 assemblyDone=%1 productionMode=%2, 车2 assemblyDone=%3 productionMode=%4, car2Empty=%5")
                .arg(twoCar.car1.assemblyDone).arg(twoCar.car1.productionMode)
                .arg(twoCar.car2.assemblyDone).arg(twoCar.car2.productionMode)
                .arg(car2Empty ? "是" : "否"));
            Logger::info(QString("  解析用包前6字节(车1头): %1 %2 %3 %4 %5 %6")
                .arg(quint8(packet[0]), 2, 16, QChar('0')).arg(quint8(packet[1]), 2, 16, QChar('0'))
                .arg(quint8(packet[2]), 2, 16, QChar('0')).arg(quint8(packet[3]), 2, 16, QChar('0'))
                .arg(quint8(packet[4]), 2, 16, QChar('0')).arg(quint8(packet[5]), 2, 16, QChar('0')));
        }
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
    QString vehicleModel = getItemNameByCommand(QString::number(car.vehicleType));
    if (vehicleModel.isEmpty())
        vehicleModel = vehicleTypeToString(car.vehicleType);
    w.setVehicleModel(vehicleModel);
    w.setWeights(car.weights);
    w.setBarcodes(car.barcodes);
    return w;
}

QString MainWindow::formatSlotText(const QString &vehicleModel, double weightG, const QString &barcode) const
{
    QString vm = vehicleModel.isEmpty() ? QStringLiteral("-") : vehicleModel;
    QString w = QString::number(weightG, 'f', 1);  // 内部统一克(g)
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
    refreshAllVisualizationDeviation();  // 每次物品移动后判断任意2个是否大于60g
}

// 协议用 float 传输，转 g 后仍有精度误差，60g 边界需容差
static const double DeviationEpsilon = 0.001;  // 0.001g 容差

// 第一托偏差逻辑：从最大重量依次比较每一个，有>=阈值则变红；空槽位不参与；直到某重量无超差则停止
static QSet<int> computeRedSlotsCar1(const double w[8], double thresholdG)
{
    QSet<int> redSlots;
    QVector<int> indices(8);
    for (int i = 0; i < 8; ++i) indices[i] = i;
    std::sort(indices.begin(), indices.end(), [&w](int a, int b) { return w[a] > w[b]; });

    for (int k = 0; k < 8; ++k) {
        int i = indices[k];
        if (w[i] <= EmptySlotThreshold) continue;  // 空槽位不参与比较
        bool hasDeviation = false;
        for (int j = 0; j < 8; ++j) {
            if (i == j) continue;
            if (w[j] <= EmptySlotThreshold) continue;  // 空槽位不参与比较
            if (qAbs(w[i] - w[j]) >= thresholdG - DeviationEpsilon) {
                redSlots.insert(i);
                redSlots.insert(j);
                hasDeviation = true;
            }
        }
        if (!hasDeviation) break;  // 直到没有超过阈值的，顺延停止
    }
    return redSlots;
}

/**
 * 第二托偏差逻辑（第一托为固定标准）：
 * 1. 每个物品与第一托比较，超差则标红
 * 2. 黑色物品内部比较，超差的进入第三步
 * 3. 若仅2个超差：任一个红一个黑；若3个及以上：取平均，小于平均的标红，大于等于平均的保持黑
 */
static QSet<int> computeRedSlotsCar2(const double w1[8], const double w2[8], double thresholdG)
{
    QSet<int> redSlots;

    // 第一步：与第一托比较，超差则标红
    for (int i = 0; i < 8; ++i) {
        if (w2[i] <= EmptySlotThreshold) continue;
        for (int j = 0; j < 8; ++j) {
            if (w1[j] <= EmptySlotThreshold) continue;
            if (qAbs(w2[i] - w1[j]) >= thresholdG - DeviationEpsilon) {
                redSlots.insert(i);
                break;
            }
        }
    }

    // 第二步：黑色物品内部比较，找出超差的集合（不含已红的）
    QSet<int> internalDeviationSet;
    for (int i = 0; i < 8; ++i) {
        if (w2[i] <= EmptySlotThreshold || redSlots.contains(i)) continue;
        for (int j = i + 1; j < 8; ++j) {
            if (w2[j] <= EmptySlotThreshold || redSlots.contains(j)) continue;
            if (qAbs(w2[i] - w2[j]) >= thresholdG - DeviationEpsilon) {
                internalDeviationSet.insert(i);
                internalDeviationSet.insert(j);
            }
        }
    }

    // 第三步：对内部超差集合处理
    if (internalDeviationSet.size() == 2) {
        // 仅2个：任一个红一个黑，取索引小的标红
        QList<int> list = internalDeviationSet.values();
        redSlots.insert(list[0]);
    } else if (internalDeviationSet.size() >= 3) {
        // 3个及以上：取重量平均，按平均分为2部分（小于平均 / 大于等于平均），数量少的那部分标红
        double sum = 0;
        for (int idx : internalDeviationSet)
            sum += w2[idx];
        double avg = sum / internalDeviationSet.size();
        QVector<int> belowAvg, aboveAvg;
        for (int idx : internalDeviationSet) {
            if (w2[idx] < avg - DeviationEpsilon)
                belowAvg.append(idx);
            else
                aboveAvg.append(idx);
        }
        const QVector<int> &toRed = (belowAvg.size() <= aboveAvg.size()) ? belowAvg : aboveAvg;
        for (int idx : toRed)
            redSlots.insert(idx);
    }
    return redSlots;
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
    const double deviationThresholdG = ui->deviationThresholdSpinBox->value();

    double w[8];
    for (int i = 0; i < 8; ++i)
        w[i] = (i < weights.size()) ? weights[i] : 0.0;

    QSet<int> redSlots;
    if (carIndex == 1) {
        redSlots = computeRedSlotsCar1(w, deviationThresholdG);
    } else {
        double w1[8];
        for (int i = 0; i < 8; ++i)
            w1[i] = (i < m_car1Data.weights.size()) ? m_car1Data.weights[i] : 0.0;
        redSlots = computeRedSlotsCar2(w1, w, deviationThresholdG);
    }

    // 使用应用级默认颜色，避免被之前设置的 styleSheet 影响
    QColor defaultTextColor = QApplication::palette().color(QPalette::WindowText);
    for (int i = 0; i < 8; ++i) {
        if (redSlots.contains(i)) {
            slotLabels[i]->setStyleSheet(QStringLiteral("color: red;"));
        } else {
            slotLabels[i]->setStyleSheet(QString("color: %1;").arg(defaultTextColor.name()));
        }
    }
}

void MainWindow::refreshAllVisualizationDeviation()
{
    applySlotDeviationStyle(1, m_car1Data.weights);
    applySlotDeviationStyle(2, m_car2Data.weights);

    // 第一托填满且无超差时：未发过第一托OK则启动定时器；已发过则等第二托也填满无超差才启动
    double w1[8], w2[8];
    for (int i = 0; i < 8; ++i) {
        w1[i] = (i < m_car1Data.weights.size()) ? m_car1Data.weights[i] : 0.0;
        w2[i] = (i < m_car2Data.weights.size()) ? m_car2Data.weights[i] : 0.0;
    }
    QSet<int> red1 = computeRedSlotsCar1(w1, ui->deviationThresholdSpinBox->value());
    QSet<int> red2 = computeRedSlotsCar2(w1, w2, ui->deviationThresholdSpinBox->value());
    bool car1IsFull = true, car2IsFull = true;
    for (int i = 0; i < 8; ++i) {
        if (w1[i] <= EmptySlotThreshold) car1IsFull = false;
        if (w2[i] <= EmptySlotThreshold) car2IsFull = false;
    }
    bool car1NoDeviation = red1.isEmpty() && car1IsFull;
    bool car2NoDeviation = red2.isEmpty() && car2IsFull;
    bool shouldStartTimer = false;
    if (car1NoDeviation && m_tcpClient->isConnected()) {
        if (!m_firstTrayOkSent) {
            shouldStartTimer = true;  // 未发过第一托OK，2秒后发
        } else if (car2NoDeviation) {
            shouldStartTimer = true;  // 已发过第一托OK，等第二托也OK，2秒后发全部
        }
    }
    if (shouldStartTimer) {
        m_detectionOkTimer->start(2000);
    } else {
        m_detectionOkTimer->stop();
    }
    m_lastCar1HadDeviation = !car1NoDeviation;

    for (int r = 0; r < ui->extraTable1->rowCount(); ++r) {
        QList<double> weights;
        for (int j = 0; j < 8; ++j) {
            QTableWidgetItem *wItem = ui->extraTable1->item(r, 1 + j * 2);
            weights.append(wItem ? wItem->text().toDouble() : 0.0);
        }
        applyCurrentTableDeviationStyle(1, r, weights);
    }
    for (int r = 0; r < ui->extraTable2->rowCount(); ++r) {
        QList<double> weights;
        for (int j = 0; j < 8; ++j) {
            QTableWidgetItem *wItem = ui->extraTable2->item(r, 1 + j * 2);
            weights.append(wItem ? wItem->text().toDouble() : 0.0);
        }
        applyCurrentTableDeviationStyle(2, r, weights);
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
    QLabel *lb = qobject_cast<QLabel *>(watched);
    if (lb && m_slotMap.contains(lb)) {
        QPair<int, int> p = m_slotMap[lb];
        if (event->type() == QEvent::MouseButtonDblClick) {
            onSlotDoubleClicked(p.first, p.second);
            return true;
        }
        // 长按1号槽位(slotIndex==0)5秒清空当前托
        if (p.second == 0) {
            QMouseEvent *me = nullptr;
            if (event->type() == QEvent::MouseButtonPress) {
                me = static_cast<QMouseEvent *>(event);
                if (me->button() == Qt::LeftButton) {
                    m_longPressCarIndex = p.first;
                    m_longPressTimer->start(5000);
                }
            } else if (event->type() == QEvent::MouseButtonRelease) {
                me = static_cast<QMouseEvent *>(event);
                if (me->button() == Qt::LeftButton) {
                    m_longPressTimer->stop();
                    m_longPressCarIndex = 0;
                }
            }
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
    QString vehicleModel = getItemNameByCommand(QString::number(car.vehicleType));
    if (vehicleModel.isEmpty())
        vehicleModel = vehicleTypeToString(car.vehicleType);
    double weightG = (slotIndex < car.weights.size()) ? car.weights[slotIndex] : 0.0;
    QString barcode = (slotIndex < car.barcodes.size()) ? car.barcodes[slotIndex] : QString();

    SlotDialog dlg(vehicleModel, weightG, barcode, this);
    if (dlg.exec() == QDialog::Accepted && dlg.addToNgRequested()) {
        addNgRecord(vehicleModel, barcode, weightG);
        slotLabels[slotIndex]->setText(QString());
        PlcProtocol::FirstCarData &car = (carIndex == 1) ? m_car1Data : m_car2Data;
        while (car.weights.size() < 8) car.weights.append(0.0);
        while (car.barcodes.size() < 8) car.barcodes.append(QString());
        car.weights[slotIndex] = 0.0;  // 克(g)
        car.barcodes[slotIndex] = QString();
        clearCurrentTableSlot(carIndex, slotIndex);  // 先同步清空当前表格，再刷新偏差
        refreshAllVisualizationDeviation();  // 物品移出，重新比较剩余物品，红/黑正确更新
        ui->statusbar->showMessage(QStringLiteral("已添加到NG品"), 2000);
        Logger::info(QString("槽位 %1-%2 已添加到NG: 车型=%3, 重量=%4 g, 条码=%5")
                     .arg(carIndex).arg(slotIndex + 1).arg(vehicleModel).arg(weightG).arg(barcode));
    }
}

void MainWindow::onNgAddClicked()
{
    NgAddDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    QString vehicleModel = dlg.vehicleModel();
    QString barcode = dlg.barcode();
    double weightG = dlg.weightGrams();
    if (vehicleModel.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请输入车型名称"));
        return;
    }
    if (barcode.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请输入条码"));
        return;
    }
    if (weightG <= 0) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请输入有效的重量(克)"));
        return;
    }
    addNgRecord(vehicleModel, barcode, weightG);
    ui->statusbar->showMessage(QStringLiteral("已添加NG品"), 2000);
    Logger::info(QString("手动添加NG: 车型=%1, 条码=%2, 重量=%3 g").arg(vehicleModel).arg(barcode).arg(weightG));
}

void MainWindow::addNgRecord(const QString &vehicleModel, const QString &barcode, double weightG)
{
    qint64 id = DatabaseManager::instance().insertNgRecord(vehicleModel, barcode, weightG);
    if (id < 0) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("NG记录保存到数据库失败"));
        return;
    }
    int row = ui->ngTable->rowCount();
    ui->ngTable->insertRow(row);
    ui->ngTable->setItem(row, 0, new QTableWidgetItem(QString::number(id)));
    ui->ngTable->setItem(row, 1, new QTableWidgetItem(vehicleModel));
    ui->ngTable->setItem(row, 2, new QTableWidgetItem(barcode));
    ui->ngTable->setItem(row, 3, new QTableWidgetItem(QString::number(weightG, 'f', 1)));  // 克(g)
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

void MainWindow::onProductionSupplementClicked()
{
    if (!m_tcpClient->isConnected()) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请先连接PLC"));
        return;
    }

    SupplementDialog dlg(m_bindingMap, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    int trayIndex = dlg.trayIndex();
    QString vehicleCommand = dlg.vehicleCommand();
    int supplementQty = dlg.supplementQuantity();

    int vehicleType = vehicleCommand.toInt();
    if (vehicleType <= 0) vehicleType = PlcProtocol::Model12V;

    const PlcProtocol::FirstCarData &selectedCar = (trayIndex == 1) ? m_car1Data : m_car2Data;
    QList<double> weights = selectedCar.weights;
    QList<QString> barcodes = selectedCar.barcodes;
    while (weights.size() < 8) weights.append(0.0);
    while (barcodes.size() < 8) barcodes.append(QString());

    QByteArray packet = PlcProtocol::buildSupplementPacket(trayIndex, vehicleType, supplementQty, weights, barcodes);
    sendDataWithLog(packet);

    m_lastSupplementTrayIndex = trayIndex;
    m_lastSupplementQuantity = supplementQty;

    // 仅第一托补充时清空第二托；第二托补充时保留第一托（第一托为比较标准，需参与第二托偏差判断）
    if (trayIndex == 1) {
        clearCarDataAndVisualization(2);
    }
    // trayIndex==2 时不清空第一托

    ui->statusbar->showMessage(QStringLiteral("已发送生产补充指令: 第%1托, 数量%2").arg(trayIndex).arg(supplementQty), 3000);
    Logger::info(QString("生产补充: 托%1 车型%2 数量%3").arg(trayIndex).arg(vehicleType).arg(supplementQty));
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

    slotLabels[slotIndex]->setText(formatSlotText(vehicleModel, weightG, barcode));

    PlcProtocol::FirstCarData &car = (carIndex == 1) ? m_car1Data : m_car2Data;
    while (car.weights.size() < 8) car.weights.append(0.0);
    while (car.barcodes.size() < 8) car.barcodes.append(QString());
    car.weights[slotIndex] = weightG;  // 内部克(g)
    car.barcodes[slotIndex] = barcode;
    car.vehicleType = vehicleModelToType(vehicleModel);

    updateCurrentTableSlot(carIndex, slotIndex, vehicleModel, weightG, barcode);  // 先同步当前表格
    refreshAllVisualizationDeviation();  // 再重新判断偏差（可视化+当前表格）

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

WeightData MainWindow::currentTableRowToWeightData(QTableWidget *table, int row)
{
    WeightData w;
    QTableWidgetItem *vmItem = table->item(row, 0);
    QTableWidgetItem *timeItem = table->item(row, 17);
    w.setVehicleModel(vmItem ? vmItem->text() : QString());
    w.setTimestamp(timeItem ? QDateTime::fromString(timeItem->text(), "yyyy-MM-dd hh:mm:ss") : QDateTime::currentDateTime());
    QList<double> weights;
    QList<QString> barcodes;
    for (int j = 0; j < 8; ++j) {
        QTableWidgetItem *wItem = table->item(row, 1 + j * 2);
        QTableWidgetItem *bItem = table->item(row, 2 + j * 2);
        double weightG = wItem ? wItem->text().toDouble() : 0.0;
        weights.append(weightG);  // 内部克(g)
        barcodes.append(bItem ? bItem->text() : QString());
    }
    w.setWeights(weights);
    w.setBarcodes(barcodes);
    return w;
}

void MainWindow::onDetectionOkTimerFired()
{
    if (!m_tcpClient->isConnected()) return;
    // 再次检查第二托是否也填满且无超差
    double w2[8];
    for (int i = 0; i < 8; ++i)
        w2[i] = (i < m_car2Data.weights.size()) ? m_car2Data.weights[i] : 0.0;
    double w1[8];
    for (int i = 0; i < 8; ++i)
        w1[i] = (i < m_car1Data.weights.size()) ? m_car1Data.weights[i] : 0.0;
    QSet<int> red2 = computeRedSlotsCar2(w1, w2, ui->deviationThresholdSpinBox->value());
    bool car2IsFull = true;
    for (int i = 0; i < 8; ++i) {
        if (w2[i] <= EmptySlotThreshold) { car2IsFull = false; break; }
    }
    bool car2NoDeviation = red2.isEmpty() && car2IsFull;

    if (car2NoDeviation) {
        // 两托都填满且无超差：发送两托数据，保存并清空
        QList<double> w1List, w2List;
        QList<QString> b1List, b2List;
        for (int i = 0; i < 8; ++i) {
            w1List.append(w1[i]);
            b1List.append((i < m_car1Data.barcodes.size()) ? m_car1Data.barcodes[i] : QString());
            w2List.append(w2[i]);
            b2List.append((i < m_car2Data.barcodes.size()) ? m_car2Data.barcodes[i] : QString());
        }
        sendDataWithLog(PlcProtocol::buildDetectionOkPacketBoth(w1List, b1List, m_car1Data.vehicleType, w2List, b2List, m_car2Data.vehicleType));
        ui->statusbar->showMessage(QStringLiteral("2托检测OK，已发送信号并清空"), 2000);
        Logger::info("2托无超差，已发送检测OK指令（含两托数据）");
        doCompleteAndClearBothTrays();
    } else {
        // 仅第一托填满无超差且尚未发过第一托OK：发送第一托数据（只发一次）
        if (!m_firstTrayOkSent) {
            QList<double> w1List;
            QList<QString> b1List;
            for (int i = 0; i < 8; ++i) {
                w1List.append(w1[i]);
                b1List.append((i < m_car1Data.barcodes.size()) ? m_car1Data.barcodes[i] : QString());
            }
            sendDataWithLog(PlcProtocol::buildDetectionOkPacketTray1(w1List, b1List, m_car1Data.vehicleType));
            m_firstTrayOkSent = true;  // 标记已发，不再重复发送
            ui->statusbar->showMessage(QStringLiteral("第一托检测OK，已发送信号"), 2000);
            Logger::info("第一托无超差，已发送检测OK指令");
            // 写入第一托最大最小重量到显示
            double maxW = -1, minW = -1;
            for (int i = 0; i < 8; ++i) {
                if (w1[i] > EmptySlotThreshold) {
                    if (maxW < 0 || w1[i] > maxW) maxW = w1[i];
                    if (minW < 0 || w1[i] < minW) minW = w1[i];
                }
            }
            m_displayMaxWeight = maxW;
            m_displayMinWeight = minW;
            updateWeightRangeDisplay();
        }
        // 已发过第一托OK则不做任何事，等待第二托也OK后发送全部
    }
}

void MainWindow::onLongPressTimerFired()
{
    if (m_longPressCarIndex == 1 || m_longPressCarIndex == 2) {
        clearCarDataAndVisualization(m_longPressCarIndex);
        ui->statusbar->showMessage(
            (m_longPressCarIndex == 1) ? QStringLiteral("已清空第一托") : QStringLiteral("已清空第二托"), 2000);
        Logger::info(QString("长按1号槽位5秒，已清空第%1托").arg(m_longPressCarIndex));
        m_longPressCarIndex = 0;
    }
}

void MainWindow::updateWeightRangeDisplay()
{
    QString maxStr = (m_displayMaxWeight >= 0) ? QString::number(m_displayMaxWeight, 'f', 1) : QStringLiteral("--");
    QString minStr = (m_displayMinWeight >= 0) ? QString::number(m_displayMinWeight, 'f', 1) : QStringLiteral("--");
    ui->weightRangeLabel->setText(QStringLiteral("最大重量: %1 g    最小重量: %2 g").arg(maxStr).arg(minStr));
}

void MainWindow::onSaveDeviationClicked()
{
    double v = ui->deviationThresholdSpinBox->value();
    DatabaseManager::instance().setSetting("deviation_threshold_g", v);
    ui->statusbar->showMessage(QStringLiteral("偏差参数已保存"), 2000);
    Logger::info(QString("偏差阈值已保存: %1 g").arg(v));
}

void MainWindow::onCompleteCurrent1Clicked()
{
    // 第一托完成按钮不做任何操作，等第二托完成
    return;
}

void MainWindow::onCompleteCurrent2Clicked()
{
    int totalRows = ui->extraTable1->rowCount() + ui->extraTable2->rowCount();
    if (totalRows == 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("当前表格无数据"));
        return;
    }
    // 发送第二托完成OK指令（携带两托所有物品数据）
    if (m_tcpClient->isConnected()) {
        QList<double> w1, w2;
        QList<QString> b1, b2;
        for (int i = 0; i < 8; ++i) {
            w1.append((i < m_car1Data.weights.size()) ? m_car1Data.weights[i] : 0.0);
            b1.append((i < m_car1Data.barcodes.size()) ? m_car1Data.barcodes[i] : QString());
            w2.append((i < m_car2Data.weights.size()) ? m_car2Data.weights[i] : 0.0);
            b2.append((i < m_car2Data.barcodes.size()) ? m_car2Data.barcodes[i] : QString());
        }
        sendDataWithLog(PlcProtocol::buildDetectionOkPacketBoth(w1, b1, m_car1Data.vehicleType, w2, b2, m_car2Data.vehicleType));
        Logger::info("2托完成，已发送检测OK指令（含两托数据）");
    }
    doCompleteAndClearBothTrays();
    ui->statusbar->showMessage(QStringLiteral("2托已完成，已发送OK指令、保存到历史并清空"), 2000);
}

void MainWindow::doCompleteAndClearBothTrays()
{
    int totalRows = ui->extraTable1->rowCount() + ui->extraTable2->rowCount();
    for (int r = 0; r < ui->extraTable1->rowCount(); ++r) {
        WeightData w = currentTableRowToWeightData(ui->extraTable1, r);
        addWeightData(w, 1);
    }
    for (int r = 0; r < ui->extraTable2->rowCount(); ++r) {
        WeightData w = currentTableRowToWeightData(ui->extraTable2, r);
        addWeightData(w, 2);
    }
    ui->extraTable1->setRowCount(0);
    ui->extraTable2->setRowCount(0);
    QLabel *slots1[] = { ui->slot1_1, ui->slot1_2, ui->slot1_3, ui->slot1_4, ui->slot1_5, ui->slot1_6, ui->slot1_7, ui->slot1_8 };
    QLabel *slots2[] = { ui->slot2_1, ui->slot2_2, ui->slot2_3, ui->slot2_4, ui->slot2_5, ui->slot2_6, ui->slot2_7, ui->slot2_8 };
    for (int i = 0; i < 8; ++i) {
        slots1[i]->setText(QString());
        slots1[i]->setStyleSheet(QString());
        slots2[i]->setText(QString());
        slots2[i]->setStyleSheet(QString());
    }
    m_detectionOkTimer->stop();
    m_firstTrayOkSent = false;  // 清空后重置，下一轮可再发第一托OK
    m_car1Data = PlcProtocol::FirstCarData();
    m_car2Data = PlcProtocol::FirstCarData();
    m_displayMaxWeight = -1;
    m_displayMinWeight = -1;
    updateWeightRangeDisplay();
    refreshAllVisualizationDeviation();
    Logger::info(QString("2托完成: %1 条记录已写入历史，已清空当前表格和可视化").arg(totalRows));
}

void MainWindow::completeCurrentTable(QTableWidget *table, int tableIndex)
{
    int rowCount = table->rowCount();
    if (rowCount == 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("当前表格无数据"));
        return;
    }
    for (int r = 0; r < rowCount; ++r) {
        WeightData w = currentTableRowToWeightData(table, r);
        addWeightData(w, tableIndex);
    }
    table->setRowCount(0);
    ui->statusbar->showMessage(QStringLiteral("已发送到历史表格并保存"), 2000);
    Logger::info(QString("当前表格%1 完成: %2 条记录已写入历史").arg(tableIndex).arg(rowCount));
}

void MainWindow::appendToCurrentTable(const WeightData &data, int carIndex)
{
    QTableWidget *table = (carIndex == 1) ? ui->extraTable1 : ui->extraTable2;
    if (!table) return;
    QList<double> weights = data.weights();
    QList<QString> barcodes = data.barcodes();
    int row;
    if (table->rowCount() > 0) {
        // 原来可视化/表格已有数据：直接更新最后一行
        row = table->rowCount() - 1;
    } else {
        // 无数据：新增一行
        row = table->rowCount();
        table->insertRow(row);
    }
    int col = 0;
    table->setItem(row, col++, new QTableWidgetItem(data.vehicleModel()));
    for (int j = 0; j < 8; ++j) {
        double weightG = (j < weights.size()) ? weights[j] : 0.0;
        QString barcode = (j < barcodes.size()) ? barcodes[j] : QString();
        table->setItem(row, col++, new QTableWidgetItem(QString::number(weightG, 'f', 1)));  // 内部克(g)
        table->setItem(row, col++, new QTableWidgetItem(barcode));
    }
    table->setItem(row, col++, new QTableWidgetItem(data.timestamp().toString("yyyy-MM-dd hh:mm:ss")));
    applyCurrentTableDeviationStyle(carIndex, row, weights);  // 表格偏差格显示红色
}

void MainWindow::mergeSupplementIntoCar(int carIndex, const PlcProtocol::FirstCarData &car, int count)
{
    static const double EmptyThreshold = 0.0001;
    PlcProtocol::FirstCarData &target = (carIndex == 1) ? m_car1Data : m_car2Data;
    while (target.weights.size() < 8) target.weights.append(0.0);
    while (target.barcodes.size() < 8) target.barcodes.append(QString());

    auto compactUpward = [&](int startIdx, int count) {
        QList<double> wList;
        QList<QString> bList;
        for (int i = 0; i < count; ++i) {
            int idx = startIdx + i;
            double w = target.weights[idx];
            QString bc = target.barcodes[idx];
            if (w > EmptyThreshold) {
                wList.append(w);
                bList.append(bc);
            }
        }
        for (int i = 0; i < count; ++i) {
            int idx = startIdx + i;
            if (i < wList.size()) {
                target.weights[idx] = wList[i];
                target.barcodes[idx] = bList[i];
            } else {
                target.weights[idx] = 0.0;
                target.barcodes[idx] = QString();
            }
        }
    };

    auto doMoveOperation = [&]() {
        double w8 = target.weights[7];
        QString b8 = target.barcodes[7];
        if (w8 <= EmptyThreshold) return;  // 8槽位空则无需移动

        bool hasEmptyIn1234 = false;
        for (int i = 0; i < 4; ++i) {
            if (target.weights[i] <= EmptyThreshold) hasEmptyIn1234 = true;
        }
        if (hasEmptyIn1234) {
            // 1234有空位：紧凑1234，8往右移入1234
            compactUpward(0, 4);
            for (int i = 0; i < 4; ++i) {
                if (target.weights[i] <= EmptyThreshold) {
                    target.weights[i] = w8;
                    target.barcodes[i] = b8;
                    target.weights[7] = 0.0;
                    target.barcodes[7] = QString();
                    return;
                }
            }
        }

        // 1234已满，检查567
        bool hasEmptyIn567 = false;
        for (int i = 4; i < 7; ++i) {
            if (target.weights[i] <= EmptyThreshold) hasEmptyIn567 = true;
        }
        if (hasEmptyIn567) {
            // 567有空位：紧凑567（空位下方往上移），8往上移入567
            compactUpward(4, 3);
            for (int i = 4; i < 7; ++i) {
                if (target.weights[i] <= EmptyThreshold) {
                    target.weights[i] = w8;
                    target.barcodes[i] = b8;
                    target.weights[7] = 0.0;
                    target.barcodes[7] = QString();
                    return;
                }
            }
        }
    };

    // 按顺序推入：工件1 -> 工件2 -> ... 每个先执行移动再放入8槽位
    int n = qMin(count, 8);
    for (int i = 0; i < n; ++i) {
        doMoveOperation();  // 为新工件腾出8槽位
        double w = (i < car.weights.size()) ? car.weights[i] : 0.0;
        QString bc = (i < car.barcodes.size()) ? car.barcodes[i] : QString();
        target.weights[7] = w;   // 从8槽位(入口)进入
        target.barcodes[7] = bc;
    }
    if (target.vehicleType == 0 && car.vehicleType != 0)
        target.vehicleType = car.vehicleType;

    QString vehicleModel = getItemNameByCommand(QString::number(target.vehicleType));
    if (vehicleModel.isEmpty())
        vehicleModel = vehicleTypeToString(target.vehicleType);

    // 更新可视化
    QLabel *slotLabels[8];
    if (carIndex == 1) {
        slotLabels[0] = ui->slot1_1; slotLabels[1] = ui->slot1_2; slotLabels[2] = ui->slot1_3; slotLabels[3] = ui->slot1_4;
        slotLabels[4] = ui->slot1_5; slotLabels[5] = ui->slot1_6; slotLabels[6] = ui->slot1_7; slotLabels[7] = ui->slot1_8;
    } else {
        slotLabels[0] = ui->slot2_1; slotLabels[1] = ui->slot2_2; slotLabels[2] = ui->slot2_3; slotLabels[3] = ui->slot2_4;
        slotLabels[4] = ui->slot2_5; slotLabels[5] = ui->slot2_6; slotLabels[6] = ui->slot2_7; slotLabels[7] = ui->slot2_8;
    }
    for (int i = 0; i < 8; ++i) {
        double w = (i < target.weights.size()) ? target.weights[i] : 0.0;
        QString bc = (i < target.barcodes.size()) ? target.barcodes[i] : QString();
        slotLabels[i]->setText(formatSlotText(vehicleModel, w, bc));
    }

    // 更新当前表格：合并到最后一行
    QTableWidget *table = (carIndex == 1) ? ui->extraTable1 : ui->extraTable2;
    if (table->rowCount() == 0) {
        table->insertRow(0);
        table->setItem(0, 0, new QTableWidgetItem(vehicleModel));
        for (int j = 0; j < 8; ++j) {
            table->setItem(0, 1 + j * 2, new QTableWidgetItem(QString()));
            table->setItem(0, 2 + j * 2, new QTableWidgetItem(QString()));
        }
        table->setItem(0, 17, new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    }
    int row = table->rowCount() - 1;
    QTableWidgetItem *vmItem = table->item(row, 0);
    if (vmItem && vmItem->text().isEmpty())
        vmItem->setText(vehicleModel);
    for (int j = 0; j < 8; ++j) {
        double w = (j < target.weights.size()) ? target.weights[j] : 0.0;
        QString bc = (j < target.barcodes.size()) ? target.barcodes[j] : QString();
        table->setItem(row, 1 + j * 2, new QTableWidgetItem(QString::number(w, 'f', 1)));
        table->setItem(row, 2 + j * 2, new QTableWidgetItem(bc));
    }
    applyCurrentTableDeviationStyle(carIndex, row, target.weights);
    refreshAllVisualizationDeviation();
}

void MainWindow::clearCarDataAndVisualization(int carIndex)
{
    QLabel *slotLabels[8];
    if (carIndex == 1) {
        slotLabels[0] = ui->slot1_1; slotLabels[1] = ui->slot1_2; slotLabels[2] = ui->slot1_3; slotLabels[3] = ui->slot1_4;
        slotLabels[4] = ui->slot1_5; slotLabels[5] = ui->slot1_6; slotLabels[6] = ui->slot1_7; slotLabels[7] = ui->slot1_8;
    } else {
        slotLabels[0] = ui->slot2_1; slotLabels[1] = ui->slot2_2; slotLabels[2] = ui->slot2_3; slotLabels[3] = ui->slot2_4;
        slotLabels[4] = ui->slot2_5; slotLabels[5] = ui->slot2_6; slotLabels[6] = ui->slot2_7; slotLabels[7] = ui->slot2_8;
    }
    for (int i = 0; i < 8; ++i)
        slotLabels[i]->setText(QString());

    PlcProtocol::FirstCarData &car = (carIndex == 1) ? m_car1Data : m_car2Data;
    car.weights.clear();
    car.barcodes.clear();
    for (int i = 0; i < 8; ++i) {
        car.weights.append(0.0);
        car.barcodes.append(QString());
    }

    QTableWidget *table = (carIndex == 1) ? ui->extraTable1 : ui->extraTable2;
    table->setRowCount(0);

    refreshAllVisualizationDeviation();
}

void MainWindow::clearCurrentTableSlot(int carIndex, int slotIndex)
{
    QTableWidget *table = (carIndex == 1) ? ui->extraTable1 : ui->extraTable2;
    if (!table || table->rowCount() == 0) return;
    int row = table->rowCount() - 1;
    int weightCol = 1 + slotIndex * 2;
    int barcodeCol = 2 + slotIndex * 2;
    QTableWidgetItem *wItem = table->item(row, weightCol);
    QTableWidgetItem *bItem = table->item(row, barcodeCol);
    if (wItem) { wItem->setText(QString()); wItem->setData(Qt::ForegroundRole, QVariant()); }
    if (bItem) { bItem->setText(QString()); bItem->setData(Qt::ForegroundRole, QVariant()); }
}

void MainWindow::updateCurrentTableSlot(int carIndex, int slotIndex, const QString &vehicleModel, double weightG, const QString &barcode)
{
    QTableWidget *table = (carIndex == 1) ? ui->extraTable1 : ui->extraTable2;
    if (!table) return;
    int row;
    if (table->rowCount() == 0) {
        table->insertRow(0);
        row = 0;
        table->setItem(row, 0, new QTableWidgetItem(vehicleModel));
        for (int j = 0; j < 8; ++j) {
            table->setItem(row, 1 + j * 2, new QTableWidgetItem(QString()));
            table->setItem(row, 2 + j * 2, new QTableWidgetItem(QString()));
        }
        table->setItem(row, 17, new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    } else {
        row = table->rowCount() - 1;
        QTableWidgetItem *vmItem = table->item(row, 0);
        if (vmItem && vmItem->text().isEmpty())
            vmItem->setText(vehicleModel);
    }
    int weightCol = 1 + slotIndex * 2;
    int barcodeCol = 2 + slotIndex * 2;
    table->setItem(row, weightCol, new QTableWidgetItem(QString::number(weightG, 'f', 1)));  // 内部克(g)
    table->setItem(row, barcodeCol, new QTableWidgetItem(barcode));
}

void MainWindow::applyCurrentTableDeviationStyle(int carIndex, int row, const QList<double> &weights)
{
    QTableWidget *table = (carIndex == 1) ? ui->extraTable1 : ui->extraTable2;
    if (!table || row < 0 || row >= table->rowCount()) return;
    const double deviationThresholdG = ui->deviationThresholdSpinBox->value();
    double w[8];
    for (int i = 0; i < 8; ++i)
        w[i] = (i < weights.size()) ? weights[i] : 0.0;
    QSet<int> redSlots;
    if (carIndex == 1) {
        redSlots = computeRedSlotsCar1(w, deviationThresholdG);
    } else {
        double w1[8];
        for (int i = 0; i < 8; ++i)
            w1[i] = (i < m_car1Data.weights.size()) ? m_car1Data.weights[i] : 0.0;
        redSlots = computeRedSlotsCar2(w1, w, deviationThresholdG);
    }
    for (int i = 0; i < 8; ++i) {
        QTableWidgetItem *wItem = table->item(row, 1 + i * 2);
        QTableWidgetItem *bItem = table->item(row, 2 + i * 2);
        QBrush redBrush(Qt::red);
        if (redSlots.contains(i)) {
            if (wItem) wItem->setForeground(redBrush);
            if (bItem) bItem->setForeground(redBrush);
        } else {
            if (wItem) wItem->setData(Qt::ForegroundRole, QVariant());
            if (bItem) bItem->setData(Qt::ForegroundRole, QVariant());
        }
    }
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
            double weightG = (j < weights.size()) ? weights[j] : 0.0;
            QString barcode = (j < barcodes.size()) ? barcodes[j] : QString();
            table->setItem(i, col++, new QTableWidgetItem(QString::number(weightG, 'f', 1)));  // 内部克(g)
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
    
    // 更新表格显示
    ui->bindingTable->setRowCount(m_bindingMap.size());
    int row = 0;
    for (auto it = m_bindingMap.constBegin(); it != m_bindingMap.constEnd(); ++it, ++row) {
        ui->bindingTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        ui->bindingTable->setItem(row, 1, new QTableWidgetItem(it.value()));
    }
    
    ui->commandEdit->clear();
    ui->itemNameEdit->clear();
    ui->statusbar->showMessage(QString("已添加绑定: %1 -> %2").arg(command, itemName), 2000);
}

void MainWindow::onRemoveBindingClicked()
{
    int row = ui->bindingTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的绑定项！"));
        return;
    }
    QTableWidgetItem *cmdItem = ui->bindingTable->item(row, 0);
    QTableWidgetItem *nameItem = ui->bindingTable->item(row, 1);
    if (!cmdItem || !nameItem) return;
    QString command = cmdItem->text();
    QString text = QStringLiteral("%1 -> %2").arg(command).arg(nameItem->text());
    
    int ret = QMessageBox::question(this, "确认", 
                                    QString("确定要删除绑定 '%1' 吗？").arg(text),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (DatabaseManager::instance().deleteBinding(command)) {
            m_bindingMap.remove(command);
            ui->bindingTable->removeRow(row);
            ui->statusbar->showMessage(QStringLiteral("已删除绑定"), 2000);
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
