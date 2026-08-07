#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "visualizationslotlabel.h"
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
#include <QItemSelectionModel>
#include <QVector>
#include <algorithm>

// 空槽位阈值，重量<=此值视为空，不参与偏差比较
static const double EmptySlotThreshold = 0.0001;

enum { DeviationStateRole = Qt::UserRole + 1 };

static bool isTrayCompactCurrentTable(QTableWidget *table)
{
    return table && table->columnCount() >= 9 && table->rowCount() >= 3;
}

static QString carDisplayName(int carIndex)
{
    return carIndex == 1 ? QStringLiteral("左车") : QStringLiteral("右车");
}

static void applyTableBarcodeWrapSettings(QTableWidget *t)
{
    if (!t)
        return;
    t->setWordWrap(true);
    t->setTextElideMode(Qt::ElideNone);
}

static void styleBarcodeTableItem(QTableWidgetItem *item)
{
    if (!item)
        return;
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
}

/** 按列宽计算换行后的单元格高度（QTableWidget 需配合 setSizeHint 才能撑开行高） */
static int wrappedBarcodeCellHeight(const QFontMetrics &fm, int columnWidth, const QString &text)
{
    if (text.isEmpty())
        return fm.height() + 8;
    const int innerW = qMax(24, columnWidth - 12);
    const QRect br = fm.boundingRect(QRect(0, 0, innerW, 10000),
                                     Qt::AlignLeft | Qt::TextWordWrap, text);
    return br.height() + 12;
}

static void applyBarcodeCellSizeHint(QTableWidget *table, int row, int col, QTableWidgetItem *item)
{
    if (!table || !item)
        return;
    styleBarcodeTableItem(item);
    const QFontMetrics fm(table->font());
    const int h = wrappedBarcodeCellHeight(fm, table->columnWidth(col), item->text());
    item->setSizeHint(QSize(table->columnWidth(col), h));
}

static void resizeCompactTrayBarcodeRow(QTableWidget *t)
{
    if (!isTrayCompactCurrentTable(t))
        return;
    const QFontMetrics fm(t->font());
    int maxH = fm.height() + 8;
    for (int c = 1; c <= 8; ++c) {
        if (QTableWidgetItem *it = t->item(1, c)) {
            const int h = wrappedBarcodeCellHeight(fm, t->columnWidth(c), it->text());
            it->setSizeHint(QSize(t->columnWidth(c), h));
            maxH = qMax(maxH, h);
        }
    }
    t->verticalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    t->setRowHeight(1, maxH);
}

static void resizeTableRowsForBarcodeColumns(QTableWidget *table, const QList<int> &barcodeCols)
{
    if (!table || barcodeCols.isEmpty())
        return;
    const QFontMetrics fm(table->font());
    for (int row = 0; row < table->rowCount(); ++row) {
        int maxH = fm.height() + 8;
        for (int col : barcodeCols) {
            if (QTableWidgetItem *it = table->item(row, col)) {
                const int h = wrappedBarcodeCellHeight(fm, table->columnWidth(col), it->text());
                it->setSizeHint(QSize(table->columnWidth(col), h));
                maxH = qMax(maxH, h);
            }
        }
        table->setRowHeight(row, maxH);
    }
}

static bool isTrayTableColumnSelected(QTableWidget *table, int column)
{
    if (!table || !table->selectionModel())
        return false;
    for (const QModelIndex &ix : table->selectionModel()->selectedIndexes()) {
        if (ix.column() == column)
            return true;
    }
    return false;
}

/** 偏差色与选中高亮：选中列时清除自定义前景，使用系统默认选中样式 */
static void updateCompactTrayCellAppearance(QTableWidget *table, QTableWidgetItem *item)
{
    if (!table || !item)
        return;
    if (isTrayTableColumnSelected(table, item->column())) {
        item->setData(Qt::ForegroundRole, QVariant());
        item->setData(Qt::BackgroundRole, QVariant());
        return;
    }
    const QString state = item->data(DeviationStateRole).toString();
    if (state == QStringLiteral("alarm")) {
        item->setForeground(QBrush(Qt::red));
        item->setData(Qt::BackgroundRole, QVariant());
    } else if (state == QStringLiteral("ok")) {
        item->setForeground(QBrush(Qt::darkGreen));
        item->setData(Qt::BackgroundRole, QVariant());
    } else if (state == QStringLiteral("ok_blue")) {
        item->setForeground(QBrush(QColor(0, 90, 200)));
        item->setData(Qt::BackgroundRole, QVariant());
    } else {
        item->setData(Qt::ForegroundRole, QVariant());
        item->setData(Qt::BackgroundRole, QVariant());
    }
}

// 当前表列1-8 与协议槽映射不变；仅显示编号：列1-4 标为 5-8，列5-8 标为 1-4（车型仍从右侧槽位 0 起依次填入）
static const int TrayVisualColOrderSlotIndex[8] = { 4, 5, 6, 7, 0, 1, 2, 3 };

static QString trayVisualPosLabel(int trayIndex, int visualCol)
{
    const int labelNum = (visualCol < 4) ? (visualCol + 5) : (visualCol - 3);
    return trayIndex == 1 ? QStringLiteral("左%1").arg(labelNum)
                          : QStringLiteral("右%1").arg(labelNum);
}

static int trayVisualColForSlotIndex(int slotIndex)
{
    for (int c = 0; c < 8; ++c) {
        if (TrayVisualColOrderSlotIndex[c] == slotIndex)
            return c + 1;
    }
    return 1;
}

static bool trayCompactCurrentTableHasPayload(QTableWidget *t)
{
    if (!t || t->rowCount() < 3 || t->columnCount() < 9)
        return false;
    for (int c = 1; c <= 8; ++c) {
        if (QTableWidgetItem *w = t->item(2, c)) {
            if (!w->text().trimmed().isEmpty())
                return true;
        }
        if (QTableWidgetItem *b = t->item(1, c)) {
            if (!b->text().trimmed().isEmpty())
                return true;
        }
    }
    return false;
}

static void setupTrayCompactCurrentTableLayout(QTableWidget *t)
{
    t->clear();
    t->setColumnCount(9);
    t->setRowCount(3);
    t->horizontalHeader()->setVisible(false);
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setStretchLastSection(false);
    applyTableBarcodeWrapSettings(t);
    t->setColumnWidth(0, 52);
    for (int c = 1; c < 9; ++c)
        t->setColumnWidth(c, 132);
    auto makeRowTitle = [](const QString &s) {
        QTableWidgetItem *it = new QTableWidgetItem(s);
        it->setFlags(Qt::ItemIsEnabled);
        QFont f = it->font();
        f.setBold(true);
        it->setFont(f);
        return it;
    };
    t->setItem(0, 0, makeRowTitle(QStringLiteral("车位")));
    t->setItem(1, 0, makeRowTitle(QStringLiteral("条码")));
    t->setItem(2, 0, makeRowTitle(QStringLiteral("重量")));
}

static void fillTrayCompactDataCells(QTableWidget *t, const QList<double> &weights,
                                     const QList<QString> &barcodes, int trayIndex)
{
    for (int c = 0; c < 8; ++c) {
        int slotIdx = TrayVisualColOrderSlotIndex[c];
        double wg = (slotIdx < weights.size()) ? weights[slotIdx] : 0.0;
        QString bc = (slotIdx < barcodes.size()) ? barcodes[slotIdx] : QString();
        QTableWidgetItem *posItem = new QTableWidgetItem(trayVisualPosLabel(trayIndex, c));
        posItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        t->setItem(0, c + 1, posItem);
        const bool has = wg > EmptySlotThreshold;
        auto *bcItem = new QTableWidgetItem(has ? bc : QString());
        bcItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        if (has && !bc.isEmpty())
            bcItem->setToolTip(bc);
        t->setItem(1, c + 1, bcItem);
        applyBarcodeCellSizeHint(t, 1, c + 1, bcItem);
        auto *wItem = new QTableWidgetItem(has ? QString::number(wg, 'f', 1) : QString());
        wItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        t->setItem(2, c + 1, wItem);
    }
    resizeCompactTrayBarcodeRow(t);
}

// 旧对比算法前向声明（已停用）
// static QSet<int> computeRedSlotsOptimalWindow(...);
// static QSet<int> computeRedSlotsCar1(...);
// static QSet<int> computeRedSlotsCar2(...);

static bool plcSlotHasPayload(const PlcProtocol::FirstCarData &car, int i)
{
    const double w = (i < car.weights.size()) ? car.weights[i] : 0.0;
    // 与界面一致：重量为 0（或未称重）视为空槽，不参与「新槽位」判断
    return w > EmptySlotThreshold;
}

/** PLC 按序累计上报（前序数据保留、每次多一个新工件）：取本次新占用的最大槽位下标 */
static int findNewlyFilledSlotIndexMax(const PlcProtocol::FirstCarData &prev, const PlcProtocol::FirstCarData &next)
{
    int best = -1;
    for (int i = 0; i < 8; ++i) {
        if (plcSlotHasPayload(next, i) && !plcSlotHasPayload(prev, i))
            best = i;
    }
    return best;
}

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

    m_newSlotFlashTimer = new QTimer(this);
    m_newSlotFlashTimer->setInterval(280);
    connect(m_newSlotFlashTimer, &QTimer::timeout, this, &MainWindow::onNewSlotFlashTimerFired);

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
            const QString bc = row.value(2).toString();
            auto *bcItem = new QTableWidgetItem(bc);
            if (!bc.isEmpty())
                bcItem->setToolTip(bc);
            ui->ngTable->setItem(i, 2, bcItem);
            applyBarcodeCellSizeHint(ui->ngTable, i, 2, bcItem);
            ui->ngTable->setRowHeight(i, wrappedBarcodeCellHeight(ui->ngTable->fontMetrics(),
                                                                   ui->ngTable->columnWidth(2), bc));
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
    setWindowTitle(QStringLiteral("称重对比系统"));
    setMinimumSize(1100, 720);

    // 隐藏历史记录标签（收到即显示，不再使用历史页）
    const int historyIdx = ui->tabWidget->indexOf(ui->historyTab);
    if (historyIdx >= 0)
        ui->tabWidget->setTabVisible(historyIdx, false);

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
    // 第一托/第二托「当前」表：3×9 紧凑布局（左列车位/条码/重量）
    setupTray1CurrentTable();
    setupTray2CurrentTable();

    auto setupTrayCurrentTableSelection = [this](QTableWidget *table, int carIndex) {
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setSelectionBehavior(QAbstractItemView::SelectColumns);
        table->setFocusPolicy(Qt::StrongFocus);
        connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                [this, table, carIndex](const QItemSelection &, const QItemSelection &) {
                    if (!isTrayCompactCurrentTable(table))
                        return;
                    QList<double> weights;
                    weights.resize(8);
                    for (int c = 0; c < 8; ++c) {
                        const int slotIdx = TrayVisualColOrderSlotIndex[c];
                        QTableWidgetItem *wItem = table->item(2, c + 1);
                        weights[slotIdx] = wItem ? wItem->text().toDouble() : 0.0;
                    }
                    applyCurrentTableDeviationStyle(carIndex, 2, weights);
                });
    };
    setupTrayCurrentTableSelection(ui->extraTable1, 1);
    setupTrayCurrentTableSelection(ui->extraTable2, 2);

    // 称重数据页：仅左右车可视化并排，隐藏右侧 NG/当前表区域
    ui->rightWidget->setVisible(false);
    ui->gridLayout_weightData->setColumnStretch(0, 1);
    ui->gridLayout_weightData->setColumnStretch(1, 1);
    ui->gridLayout_weightData->setRowStretch(0, 1);
    ui->gridLayout_weightData->setRowStretch(1, 0);
    applyTableBarcodeWrapSettings(ui->extraTable1);
    applyTableBarcodeWrapSettings(ui->extraTable2);
    ui->extraTable1->setMaximumHeight(260);
    ui->extraTable2->setMaximumHeight(260);
    
    // 设置历史记录标签页的布局拉伸比例（上下两表各为1）
    ui->verticalLayout_history->setStretchFactor(ui->historyLeftGroup, 1);
    ui->verticalLayout_history->setStretchFactor(ui->historyRightGroup, 1);
    
    // 左车可视化：4 列 — 左备注(0)、槽1-4(1)、槽5-8(2)、右备注(3)；仅中间两列平分宽度
    ui->gridLayout_leftTop->setColumnStretch(0, 0);
    ui->gridLayout_leftTop->setColumnStretch(1, 1);
    ui->gridLayout_leftTop->setColumnStretch(2, 1);
    ui->gridLayout_leftTop->setColumnStretch(3, 0);
    {
        const QList<QLabel *> tray1Notes = {
            ui->slot1_leftNote1, ui->slot1_leftNote2, ui->slot1_leftNote3, ui->slot1_leftNote4,
            ui->slot1_rightNote1, ui->slot1_rightNote2, ui->slot1_rightNote3, ui->slot1_rightNote4
        };
        QFont noteFont = tray1Notes.first()->font();
        noteFont.setPointSize(20);
        noteFont.setBold(true);
        for (QLabel *lb : tray1Notes) {
            lb->setProperty("trayNote", true);
            lb->setFont(noteFont);
            lb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            lb->setMinimumWidth(56);
            lb->setMaximumWidth(72);
        }
    }
    ui->gridLayout_leftTop->setRowStretch(0, 1);
    ui->gridLayout_leftTop->setRowStretch(1, 1);
    ui->gridLayout_leftTop->setRowStretch(2, 1);
    ui->gridLayout_leftTop->setRowStretch(3, 1);
    
    // 右车可视化：与左车相同 4 列布局
    ui->gridLayout_leftBottom->setColumnStretch(0, 0);
    ui->gridLayout_leftBottom->setColumnStretch(1, 1);
    ui->gridLayout_leftBottom->setColumnStretch(2, 1);
    ui->gridLayout_leftBottom->setColumnStretch(3, 0);
    {
        const QList<QLabel *> tray2Notes = {
            ui->slot2_leftNote1, ui->slot2_leftNote2, ui->slot2_leftNote3, ui->slot2_leftNote4,
            ui->slot2_rightNote1, ui->slot2_rightNote2, ui->slot2_rightNote3, ui->slot2_rightNote4
        };
        QFont noteFont = tray2Notes.first()->font();
        noteFont.setPointSize(20);
        noteFont.setBold(true);
        for (QLabel *lb : tray2Notes) {
            lb->setProperty("trayNote", true);
            lb->setFont(noteFont);
            lb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            lb->setMinimumWidth(56);
            lb->setMaximumWidth(72);
        }
    }
    ui->gridLayout_leftBottom->setRowStretch(0, 1);
    ui->gridLayout_leftBottom->setRowStretch(1, 1);
    ui->gridLayout_leftBottom->setRowStretch(2, 1);
    ui->gridLayout_leftBottom->setRowStretch(3, 1);

    // 可视化槽位：自定义绘制，文字块垂直/水平居中；超宽条码按字符折行
    QList<QLabel *> slotLabels;
    slotLabels << ui->slot1_1 << ui->slot1_2 << ui->slot1_3 << ui->slot1_4
               << ui->slot1_5 << ui->slot1_6 << ui->slot1_7 << ui->slot1_8
               << ui->slot2_1 << ui->slot2_2 << ui->slot2_3 << ui->slot2_4
               << ui->slot2_5 << ui->slot2_6 << ui->slot2_7 << ui->slot2_8;
    QFont slotFont;
    slotFont.setPointSize(14);
    slotFont.setBold(true);
    for (QLabel *lb : slotLabels) {
        lb->setTextFormat(Qt::PlainText);
        lb->setAlignment(Qt::AlignCenter);
        lb->setWordWrap(true);
        lb->setFont(slotFont);
        lb->setMinimumWidth(108);
        lb->setContentsMargins(4, 4, 4, 4);
    }

    setupSlotDoubleClick();

    applyTableBarcodeWrapSettings(ui->ngTable);
    ui->ngTable->setColumnWidth(2, 240);

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
    applyTableBarcodeWrapSettings(table);
    // 关闭最后一列的自动拉伸
    table->horizontalHeader()->setStretchLastSection(false);

    // 列顺序：车型名称、重量1、条码1、…、重量8、条码8、时间（共18列）
    table->setColumnWidth(0, 75);   // 车型名称
    for (int i = 0; i < 8; ++i) {
        table->setColumnWidth(1 + i * 2, 55);     // 重量1-8
        table->setColumnWidth(2 + i * 2, 150);    // 条码1-8（20 位可换行显示）
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

void MainWindow::setupTray1CurrentTable()
{
    setupTrayCompactCurrentTableLayout(ui->extraTable1);
}

void MainWindow::setupTray2CurrentTable()
{
    setupTrayCompactCurrentTableLayout(ui->extraTable2);
}

void MainWindow::fillTray1CurrentTable(const QString &vehicleModel, const QList<double> &weights,
                                       const QList<QString> &barcodes, const QDateTime &recordTime)
{
    QTableWidget *t = ui->extraTable1;
    setupTray1CurrentTable();
    t->setProperty("_tray1VehicleModel", vehicleModel);
    t->setProperty("_tray1Time", recordTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    fillTrayCompactDataCells(t, weights, barcodes, 1);
    QList<double> w8;
    for (int i = 0; i < 8; ++i)
        w8.append((i < weights.size()) ? weights[i] : 0.0);
    applyCurrentTableDeviationStyle(1, 2, w8);
}

void MainWindow::fillTray2CurrentTable(const QString &vehicleModel, const QList<double> &weights,
                                       const QList<QString> &barcodes, const QDateTime &recordTime)
{
    QTableWidget *t = ui->extraTable2;
    setupTray2CurrentTable();
    t->setProperty("_tray2VehicleModel", vehicleModel);
    t->setProperty("_tray2Time", recordTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    fillTrayCompactDataCells(t, weights, barcodes, 2);
    QList<double> w8;
    for (int i = 0; i < 8; ++i)
        w8.append((i < weights.size()) ? weights[i] : 0.0);
    applyCurrentTableDeviationStyle(2, 2, w8);
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
        ui->connectionStatusLabel->setStyleSheet(QStringLiteral("color: green; font-weight: bold;"));
    } else {
        ui->connectionStatusLabel->setStyleSheet(QStringLiteral("color: gray;"));
    }
    ui->connectionStatusLabel->setText(connected ? QStringLiteral("已连接") : QStringLiteral("未连接"));
}

#if 0  // 旧路由/对比门控（已停用）
static bool isCarDataEmpty(const PlcProtocol::FirstCarData &car)
{
    if (car.assemblyDone >= PlcProtocol::AssemblyInProgress)
        return false;
    for (double w : car.weights) {
        if (w > EmptySlotThreshold)
            return false;
    }
    return true;
}

static bool isAssemblyComplete(const PlcProtocol::FirstCarData &car)
{
    return car.assemblyDone == PlcProtocol::AssemblyDoneComplete;
}
#endif

void MainWindow::parseReceivedData(const QByteArray &data)
{
    // PLC 每次整帧 432 字节；长度非整帧整数倍则舍弃，避免错位解析
    if (data.size() % PlcProtocol::FullPacketSize != 0) {
        Logger::warning(QString("收到长度%1字节，非%2整数倍，已舍弃")
                            .arg(data.size()).arg(PlcProtocol::FullPacketSize));
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

        // 不再对比、不应答：每次收到整帧先清空再按包内容重写左右车显示
        clearCarDataAndVisualization(1);
        clearCarDataAndVisualization(2);

        updateCarVisualization(1, twoCar.car1);
        appendToCurrentTable(firstCarDataToWeightData(twoCar.car1), 1);

        updateCarVisualization(2, twoCar.car2);
        appendToCurrentTable(firstCarDataToWeightData(twoCar.car2), 2);

        Logger::info(QStringLiteral("已刷新左右车显示（整帧覆盖，无对比/无应答）"));

        /* ===== 旧逻辑：按托路由、补充生产合并、偏差对比后应答（已停用）=====
        bool car1Supplement = (twoCar.car1.productionMode == PlcProtocol::SupplementProduction);
        bool car2Supplement = (twoCar.car2.productionMode == PlcProtocol::SupplementProduction);
        if (car1Supplement || car2Supplement) {
            ...
        }
        if (car1Valid && car2Empty) { update 左车 }
        else if (car2Valid && car1Empty) { update 右车 }
        else if (car1Valid && car2Valid) { ... }
        ===== */
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
    // 重量为 0 不显示（PLC 用 0 填充空位）
    if (weightG <= EmptySlotThreshold)
        return QString();

    const QString bcTrim = barcode.trimmed();
    QString vm = vehicleModel;
    if (vm.isEmpty())
        vm = QStringLiteral("-");
    else if (vm == QStringLiteral("16V机型"))
        vm = QStringLiteral("16v");
    else if (vm == QStringLiteral("12V机型"))
        vm = QStringLiteral("12v");

    QString wPart;
    if (qAbs(weightG - qint64(qRound(weightG))) < 0.05)
        wPart = QString::number(qint64(qRound(weightG))) + QStringLiteral("g");
    else
        wPart = QString::number(weightG, 'f', 1) + QStringLiteral("g");

    const QString bc = bcTrim.isEmpty() ? QStringLiteral("-") : bcTrim;

    return QStringLiteral("机型：%1\n重量：%2\n条码：%3").arg(vm).arg(wPart).arg(bc);
}

void MainWindow::updateCarVisualization(int carIndex, const PlcProtocol::FirstCarData &car)
{
    if (carIndex != 1 && carIndex != 2)
        return;

    const PlcProtocol::FirstCarData prev = (carIndex == 1) ? m_car1Data : m_car2Data;
    const int newFlashSlot = findNewlyFilledSlotIndexMax(prev, car);

    if (carIndex == 1)
        m_car1Data = car;
    else
        m_car2Data = car;

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
        const QString slotText = formatSlotText(vehicleModel, w, bc);
        slotLabels[i]->setText(slotText);
        slotLabels[i]->setToolTip(bc.trimmed().isEmpty() ? QString() : bc.trimmed());
    }
    // 按协议槽位状态着色（0绿/1红），不再做重量对比
    refreshAllVisualizationDeviation();
    if (newFlashSlot >= 0)
        startNewSlotHighlightFlash(carIndex, newFlashSlot);
}

// ===== 旧重量对比算法（已停用，改由 PLC 槽位状态字节 0/1 着色）=====
#if 0
// 协议用 float 传输，转 g 后仍有精度误差，阈值边界需容差
static const double DeviationEpsilon = 0.001;  // 0.001g 容差

static QSet<int> computeRedSlotsOptimalWindow(const double w[8], double thresholdG, const QString &carLabel,
                                              const QSet<int> &excludeSlots, bool assemblyComplete)
{
    QSet<int> redSlots;
    if (excludeSlots.isEmpty() && !assemblyComplete)
        return redSlots;

    struct WeightEntry {
        double weight;
        int slotIndex;
    };
    QVector<WeightEntry> sortData;
    sortData.reserve(8);
    for (int i = 0; i < 8; ++i) {
        if (excludeSlots.contains(i))
            continue;
        if (w[i] <= EmptySlotThreshold)
            continue;
        sortData.append({ w[i], i });
    }

    if (sortData.isEmpty())
        return redSlots;

    std::sort(sortData.begin(), sortData.end(), [](const WeightEntry &a, const WeightEntry &b) {
        if (qAbs(a.weight - b.weight) > DeviationEpsilon)
            return a.weight < b.weight;
        return a.slotIndex < b.slotIndex;
    });

    const int n = sortData.size();
    double total = 0;
    for (int i = 0; i < n; ++i)
        total += sortData[i].weight;

    double maxSum = 0;
    int bestL = 0;
    int bestR = 0;
    for (int i = 0; i < n; ++i) {
        int farthestJ = i;
        for (int j = i; j < n; ++j) {
            const double diff = sortData[j].weight - sortData[i].weight;
            if (diff + DeviationEpsilon >= thresholdG)
                break;
            farthestJ = j;
        }
        double curSum = 0;
        for (int j = i; j <= farthestJ; ++j)
            curSum += sortData[j].weight;
        if (curSum > maxSum + DeviationEpsilon) {
            maxSum = curSum;
            bestL = i;
            bestR = farthestJ;
        }
    }

    QSet<int> keepSlots;
    for (int k = bestL; k <= bestR; ++k)
        keepSlots.insert(sortData[k].slotIndex);
    for (int i = 0; i < n; ++i) {
        if (!keepSlots.contains(sortData[i].slotIndex))
            redSlots.insert(sortData[i].slotIndex);
    }

    const double removedWeight = total - maxSum;
    QStringList keepSlotLabels;
    for (int k = bestL; k <= bestR; ++k)
        keepSlotLabels.append(QString::number(sortData[k].slotIndex + 1));
    if (excludeSlots.isEmpty()) {
        Logger::info(QStringLiteral("%1 %2件对比: 保留%3件(槽%4) 剔除%5件 剔除总重%6g 阈值%7g")
                         .arg(carLabel)
                         .arg(n)
                         .arg(bestR - bestL + 1)
                         .arg(keepSlotLabels.join(QLatin1Char(',')))
                         .arg(redSlots.size())
                         .arg(removedWeight, 0, 'f', 1)
                         .arg(thresholdG, 0, 'f', 1));
    } else {
        Logger::info(QStringLiteral("%1 %2件对比: 已排除%3件(与左车对比已标红) 参与%4件 保留%5件(槽%6) 剔除%7件 剔除总重%8g 阈值%9g")
                         .arg(carLabel)
                         .arg(n)
                         .arg(excludeSlots.size())
                         .arg(n)
                         .arg(bestR - bestL + 1)
                         .arg(keepSlotLabels.join(QLatin1Char(',')))
                         .arg(redSlots.size())
                         .arg(removedWeight, 0, 'f', 1)
                         .arg(thresholdG, 0, 'f', 1));
    }
    return redSlots;
}

static QSet<int> computeRedSlotsCar1(const double w[8], double thresholdG, bool assemblyComplete)
{
    if (!assemblyComplete)
        return QSet<int>();
    return computeRedSlotsOptimalWindow(w, thresholdG, QStringLiteral("左车"), QSet<int>(), true);
}

static QSet<int> computeRedSlotsCar2(const double w1[8], const double w2[8], double thresholdG,
                                       bool car2AssemblyComplete)
{
    QSet<int> redSlots;
    if (!car2AssemblyComplete)
        return redSlots;

    for (int i = 0; i < 8; ++i) {
        if (w2[i] <= EmptySlotThreshold)
            continue;
        for (int j = 0; j < 8; ++j) {
            if (w1[j] <= EmptySlotThreshold)
                continue;
            if (qAbs(w2[i] - w1[j]) + DeviationEpsilon >= thresholdG) {
                redSlots.insert(i);
                break;
            }
        }
    }

    const QSet<int> internalRed = computeRedSlotsOptimalWindow(
        w2, thresholdG, QStringLiteral("右车内部"), redSlots, true);
    for (int idx : internalRed)
        redSlots.insert(idx);

    return redSlots;
}

static QString slotDeviationState(int slotIndex, const double w[8], const QSet<int> &redSlots,
                                  bool assemblyComplete)
{
    if (slotIndex < 0 || slotIndex >= 8 || w[slotIndex] <= EmptySlotThreshold)
        return QString();
    if (redSlots.contains(slotIndex))
        return QStringLiteral("alarm");
    if (!assemblyComplete)
        return QString();
    return QStringLiteral("ok");
}
#endif // 旧重量对比算法

/** 按 PLC 槽位状态着色：正常左车绿(ok)/右车蓝(ok_blue)，异常红(alarm)；空槽无色 */
static QString slotStatusState(int carIndex, int slotIndex, const double w[8], const QList<int> &slotStatuses)
{
    if (slotIndex < 0 || slotIndex >= 8 || w[slotIndex] <= EmptySlotThreshold)
        return QString();
    const int st = (slotIndex < slotStatuses.size()) ? slotStatuses[slotIndex] : PlcProtocol::SlotStatusNormal;
    if (st == PlcProtocol::SlotStatusAlarm)
        return QStringLiteral("alarm");
    return (carIndex == 2) ? QStringLiteral("ok_blue") : QStringLiteral("ok");
}

void MainWindow::applySlotDeviationStyle(int carIndex, const QList<double> &weights,
                                         int flashSlotIndex, bool flashBackgroundOn)
{
    QLabel *slotLabels[8];
    if (carIndex == 1) {
        slotLabels[0] = ui->slot1_1; slotLabels[1] = ui->slot1_2; slotLabels[2] = ui->slot1_3; slotLabels[3] = ui->slot1_4;
        slotLabels[4] = ui->slot1_5; slotLabels[5] = ui->slot1_6; slotLabels[6] = ui->slot1_7; slotLabels[7] = ui->slot1_8;
    } else {
        slotLabels[0] = ui->slot2_1; slotLabels[1] = ui->slot2_2; slotLabels[2] = ui->slot2_3; slotLabels[3] = ui->slot2_4;
        slotLabels[4] = ui->slot2_5; slotLabels[5] = ui->slot2_6; slotLabels[6] = ui->slot2_7; slotLabels[7] = ui->slot2_8;
    }

    double w[8];
    for (int i = 0; i < 8; ++i)
        w[i] = (i < weights.size()) ? weights[i] : 0.0;

    const QList<int> &statuses = (carIndex == 1) ? m_car1Data.slotStatuses : m_car2Data.slotStatuses;

    for (int i = 0; i < 8; ++i) {
        const bool flashHere = (flashSlotIndex == i) && flashBackgroundOn;
        if (auto *vsl = qobject_cast<VisualizationSlotLabel *>(slotLabels[i])) {
            vsl->setFlashHighlight(flashHere);
            vsl->setDeviationState(slotStatusState(carIndex, i, w, statuses));
        }
    }
}

void MainWindow::startNewSlotHighlightFlash(int carIndex, int slotIndex)
{
    if (slotIndex < 0 || slotIndex > 7 || carIndex < 1 || carIndex > 2)
        return;
    if (carIndex == 1)
        m_newSlotFlashSlot1 = slotIndex;
    else
        m_newSlotFlashSlot2 = slotIndex;
    m_newSlotFlashHighlight = true;
    applySlotDeviationStyle(1, m_car1Data.weights,
                            m_newSlotFlashSlot1,
                            m_newSlotFlashHighlight && m_newSlotFlashSlot1 >= 0);
    applySlotDeviationStyle(2, m_car2Data.weights,
                            m_newSlotFlashSlot2,
                            m_newSlotFlashHighlight && m_newSlotFlashSlot2 >= 0);
    if (!m_newSlotFlashTimer->isActive())
        m_newSlotFlashTimer->start();
}

void MainWindow::stopNewSlotHighlightFlash(int carIndex)
{
    if (carIndex == 1)
        m_newSlotFlashSlot1 = -1;
    else if (carIndex == 2)
        m_newSlotFlashSlot2 = -1;
    else
        return;
    if (m_newSlotFlashSlot1 < 0 && m_newSlotFlashSlot2 < 0)
        m_newSlotFlashTimer->stop();
}

void MainWindow::onNewSlotFlashTimerFired()
{
    m_newSlotFlashHighlight = !m_newSlotFlashHighlight;
    applySlotDeviationStyle(1, m_car1Data.weights,
                            m_newSlotFlashSlot1,
                            m_newSlotFlashHighlight && m_newSlotFlashSlot1 >= 0);
    applySlotDeviationStyle(2, m_car2Data.weights,
                            m_newSlotFlashSlot2,
                            m_newSlotFlashHighlight && m_newSlotFlashSlot2 >= 0);
}

void MainWindow::refreshAllVisualizationDeviation()
{
    applySlotDeviationStyle(1, m_car1Data.weights,
                            m_newSlotFlashSlot1,
                            m_newSlotFlashHighlight && m_newSlotFlashSlot1 >= 0);
    applySlotDeviationStyle(2, m_car2Data.weights,
                            m_newSlotFlashSlot2,
                            m_newSlotFlashHighlight && m_newSlotFlashSlot2 >= 0);

    // PLC 持续发送，不再自动应答检测OK
    if (m_detectionOkTimer)
        m_detectionOkTimer->stop();
    /* ===== 旧逻辑：装件完成后无超差则延迟发检测OK（已停用）=====
    QSet<int> red1 = computeRedSlotsCar1(...);
    QSet<int> red2 = computeRedSlotsCar2(...);
    if (car1NoDeviation && connected) m_detectionOkTimer->start(2000);
    ===== */

    if (ui->extraTable1->columnCount() >= 9 && ui->extraTable1->rowCount() >= 3) {
        QList<double> weights;
        weights.resize(8);
        for (int c = 0; c < 8; ++c) {
            const int slotIdx = TrayVisualColOrderSlotIndex[c];
            QTableWidgetItem *wItem = ui->extraTable1->item(2, c + 1);
            weights[slotIdx] = wItem ? wItem->text().toDouble() : 0.0;
        }
        applyCurrentTableDeviationStyle(1, 2, weights);
    } else {
        for (int r = 0; r < ui->extraTable1->rowCount(); ++r) {
            QList<double> weights;
            for (int j = 0; j < 8; ++j) {
                QTableWidgetItem *wItem = ui->extraTable1->item(r, 1 + j * 2);
                weights.append(wItem ? wItem->text().toDouble() : 0.0);
            }
            applyCurrentTableDeviationStyle(1, r, weights);
        }
    }
    if (ui->extraTable2->columnCount() >= 9 && ui->extraTable2->rowCount() >= 3) {
        QList<double> weights;
        weights.resize(8);
        for (int c = 0; c < 8; ++c) {
            const int slotIdx = TrayVisualColOrderSlotIndex[c];
            QTableWidgetItem *wItem = ui->extraTable2->item(2, c + 1);
            weights[slotIdx] = wItem ? wItem->text().toDouble() : 0.0;
        }
        applyCurrentTableDeviationStyle(2, 2, weights);
    } else {
        for (int r = 0; r < ui->extraTable2->rowCount(); ++r) {
            QList<double> weights;
            for (int j = 0; j < 8; ++j) {
                QTableWidgetItem *wItem = ui->extraTable2->item(r, 1 + j * 2);
                weights.append(wItem ? wItem->text().toDouble() : 0.0);
            }
            applyCurrentTableDeviationStyle(2, r, weights);
        }
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
    auto *bcItem = new QTableWidgetItem(barcode);
    if (!barcode.isEmpty())
        bcItem->setToolTip(barcode);
    ui->ngTable->setItem(row, 2, bcItem);
    applyBarcodeCellSizeHint(ui->ngTable, row, 2, bcItem);
    ui->ngTable->setRowHeight(row, wrappedBarcodeCellHeight(ui->ngTable->fontMetrics(),
                                                            ui->ngTable->columnWidth(2), barcode));
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

    ui->statusbar->showMessage(QStringLiteral("已发送生产补充指令: %1, 数量%2")
                                 .arg(carDisplayName(trayIndex)).arg(supplementQty), 3000);
    Logger::info(QString("生产补充: %1 车型%2 数量%3")
                     .arg(carDisplayName(trayIndex)).arg(vehicleType).arg(supplementQty));
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
    slotLabels[slotIndex]->setToolTip(barcode.trimmed());

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
        Logger::info(QString("NG品已放入 %1 槽位%2: %3").arg(carDisplayName(carIndex)).arg(slotIndex + 1).arg(barcode));
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
    if (table == ui->extraTable1 && trayCompactCurrentTableHasPayload(table)) {
        WeightData w;
        w.setVehicleModel(table->property("_tray1VehicleModel").toString());
        const QString tsStr = table->property("_tray1Time").toString();
        w.setTimestamp(QDateTime::fromString(tsStr, QStringLiteral("yyyy-MM-dd hh:mm:ss")));
        QList<double> weights;
        QList<QString> barcodes;
        weights.resize(8);
        barcodes.resize(8);
        for (int c = 0; c < 8; ++c) {
            const int slotIdx = TrayVisualColOrderSlotIndex[c];
            QTableWidgetItem *bIt = table->item(1, c + 1);
            QTableWidgetItem *wIt = table->item(2, c + 1);
            barcodes[slotIdx] = bIt ? bIt->text() : QString();
            weights[slotIdx] = wIt ? wIt->text().toDouble() : 0.0;
        }
        w.setWeights(weights);
        w.setBarcodes(barcodes);
        return w;
    }
    if (table == ui->extraTable2 && trayCompactCurrentTableHasPayload(table)) {
        WeightData w;
        w.setVehicleModel(table->property("_tray2VehicleModel").toString());
        const QString tsStr = table->property("_tray2Time").toString();
        w.setTimestamp(QDateTime::fromString(tsStr, QStringLiteral("yyyy-MM-dd hh:mm:ss")));
        QList<double> weights;
        QList<QString> barcodes;
        weights.resize(8);
        barcodes.resize(8);
        for (int c = 0; c < 8; ++c) {
            const int slotIdx = TrayVisualColOrderSlotIndex[c];
            QTableWidgetItem *bIt = table->item(1, c + 1);
            QTableWidgetItem *wIt = table->item(2, c + 1);
            barcodes[slotIdx] = bIt ? bIt->text() : QString();
            weights[slotIdx] = wIt ? wIt->text().toDouble() : 0.0;
        }
        w.setWeights(weights);
        w.setBarcodes(barcodes);
        return w;
    }
    if (table == ui->extraTable1)
        return WeightData();
    if (table == ui->extraTable2)
        return WeightData();

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
    // PLC 持续发送且不需要应答，检测OK发送已停用
    return;
    /* ===== 旧逻辑：无超差时向 PLC 发送检测OK并保存清空（已停用）=====
    if (!m_tcpClient->isConnected()) return;
    ...
    ===== */
}

void MainWindow::onLongPressTimerFired()
{
    if (m_longPressCarIndex == 1 || m_longPressCarIndex == 2) {
        clearCarDataAndVisualization(m_longPressCarIndex);
        ui->statusbar->showMessage(
            QStringLiteral("已清空%1").arg(carDisplayName(m_longPressCarIndex)), 2000);
        Logger::info(QString("长按1号槽位5秒，已清空%1").arg(carDisplayName(m_longPressCarIndex)));
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
    stopNewSlotHighlightFlash(1);
    refreshAllVisualizationDeviation();
    // 第一托完成按钮不做任何操作，等第二托完成
    return;
}

void MainWindow::onCompleteCurrent2Clicked()
{
    stopNewSlotHighlightFlash(2);

    int totalRows = (trayCompactCurrentTableHasPayload(ui->extraTable1) ? 1 : 0)
        + (trayCompactCurrentTableHasPayload(ui->extraTable2) ? 1 : 0);
    if (totalRows == 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("当前表格无数据"));
        refreshAllVisualizationDeviation();
        return;
    }
    // 不再向 PLC 应答、不再写历史库，仅清空当前显示
    /* if (m_tcpClient->isConnected()) {
        sendDataWithLog(PlcProtocol::buildDetectionOkPacketBoth(...));
    } */
    doCompleteAndClearBothTrays();
    ui->statusbar->showMessage(QStringLiteral("已清空左右车当前显示"), 2000);
}

void MainWindow::doCompleteAndClearBothTrays()
{
    m_newSlotFlashSlot1 = -1;
    m_newSlotFlashSlot2 = -1;
    m_newSlotFlashTimer->stop();

    int totalRows = (trayCompactCurrentTableHasPayload(ui->extraTable1) ? 1 : 0)
        + (trayCompactCurrentTableHasPayload(ui->extraTable2) ? 1 : 0);
    // 不再写入历史数据库
    /* if (trayCompactCurrentTableHasPayload(ui->extraTable1)) {
        addWeightData(currentTableRowToWeightData(ui->extraTable1, 0), 1);
    }
    if (trayCompactCurrentTableHasPayload(ui->extraTable2)) {
        addWeightData(currentTableRowToWeightData(ui->extraTable2, 0), 2);
    } */
    ui->extraTable1->setProperty("_tray1VehicleModel", QVariant());
    ui->extraTable1->setProperty("_tray1Time", QVariant());
    ui->extraTable2->setProperty("_tray2VehicleModel", QVariant());
    ui->extraTable2->setProperty("_tray2Time", QVariant());
    ui->extraTable1->setRowCount(0);
    ui->extraTable2->setRowCount(0);
    QLabel *slots1[] = { ui->slot1_1, ui->slot1_2, ui->slot1_3, ui->slot1_4, ui->slot1_5, ui->slot1_6, ui->slot1_7, ui->slot1_8 };
    QLabel *slots2[] = { ui->slot2_1, ui->slot2_2, ui->slot2_3, ui->slot2_4, ui->slot2_5, ui->slot2_6, ui->slot2_7, ui->slot2_8 };
    for (int i = 0; i < 8; ++i) {
        slots1[i]->setText(QString());
        slots2[i]->setText(QString());
        if (auto *v1 = qobject_cast<VisualizationSlotLabel *>(slots1[i])) {
            v1->setFlashHighlight(false);
            v1->setDeviationState(QString());
        }
        if (auto *v2 = qobject_cast<VisualizationSlotLabel *>(slots2[i])) {
            v2->setFlashHighlight(false);
            v2->setDeviationState(QString());
        }
    }
    m_detectionOkTimer->stop();
    m_firstTrayOkSent = false;  // 清空后重置，下一轮可再发第一托OK
    m_car1Data = PlcProtocol::FirstCarData();
    m_car2Data = PlcProtocol::FirstCarData();
    m_displayMaxWeight = -1;
    m_displayMinWeight = -1;
    updateWeightRangeDisplay();
    refreshAllVisualizationDeviation();
    Logger::info(QString("已清空当前表格和可视化（未写历史库），原行数合计 %1").arg(totalRows));
}

void MainWindow::completeCurrentTable(QTableWidget *table, int tableIndex)
{
    if (table == ui->extraTable1 || table == ui->extraTable2) {
        if (!trayCompactCurrentTableHasPayload(table)) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("当前表格无数据"));
            return;
        }
        WeightData w = currentTableRowToWeightData(table, 0);
        addWeightData(w, tableIndex);
        if (table == ui->extraTable1) {
            table->setProperty("_tray1VehicleModel", QVariant());
            table->setProperty("_tray1Time", QVariant());
        } else {
            table->setProperty("_tray2VehicleModel", QVariant());
            table->setProperty("_tray2Time", QVariant());
        }
        table->setRowCount(0);
        ui->statusbar->showMessage(QStringLiteral("已发送到历史表格并保存"), 2000);
        Logger::info(QString("当前表格%1 完成: 1 条记录已写入历史").arg(tableIndex));
        return;
    }

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
    QList<double> weights = data.weights();
    QList<QString> barcodes = data.barcodes();

    if (carIndex == 1) {
        fillTray1CurrentTable(data.vehicleModel(), weights, barcodes, data.timestamp());
        return;
    }
    if (carIndex == 2) {
        fillTray2CurrentTable(data.vehicleModel(), weights, barcodes, data.timestamp());
        return;
    }
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
        slotLabels[i]->setToolTip(bc.trimmed().isEmpty() ? QString() : bc.trimmed());
    }

    // 更新当前表格
    if (carIndex == 1) {
        QDateTime ts = QDateTime::fromString(ui->extraTable1->property("_tray1Time").toString(),
                                              QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        if (!ts.isValid())
            ts = QDateTime::currentDateTime();
        fillTray1CurrentTable(vehicleModel, target.weights, target.barcodes, ts);
    } else {
        QDateTime ts = QDateTime::fromString(ui->extraTable2->property("_tray2Time").toString(),
                                              QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        if (!ts.isValid())
            ts = QDateTime::currentDateTime();
        fillTray2CurrentTable(vehicleModel, target.weights, target.barcodes, ts);
    }
    refreshAllVisualizationDeviation();
}

void MainWindow::clearCarDataAndVisualization(int carIndex)
{
    stopNewSlotHighlightFlash(carIndex);

    QLabel *slotLabels[8];
    if (carIndex == 1) {
        slotLabels[0] = ui->slot1_1; slotLabels[1] = ui->slot1_2; slotLabels[2] = ui->slot1_3; slotLabels[3] = ui->slot1_4;
        slotLabels[4] = ui->slot1_5; slotLabels[5] = ui->slot1_6; slotLabels[6] = ui->slot1_7; slotLabels[7] = ui->slot1_8;
    } else {
        slotLabels[0] = ui->slot2_1; slotLabels[1] = ui->slot2_2; slotLabels[2] = ui->slot2_3; slotLabels[3] = ui->slot2_4;
        slotLabels[4] = ui->slot2_5; slotLabels[5] = ui->slot2_6; slotLabels[6] = ui->slot2_7; slotLabels[7] = ui->slot2_8;
    }
    for (int i = 0; i < 8; ++i) {
        slotLabels[i]->setText(QString());
        if (auto *vsl = qobject_cast<VisualizationSlotLabel *>(slotLabels[i])) {
            vsl->setFlashHighlight(false);
            vsl->setDeviationState(QString());
        }
    }

    PlcProtocol::FirstCarData &car = (carIndex == 1) ? m_car1Data : m_car2Data;
    car.weights.clear();
    car.barcodes.clear();
    car.slotStatuses.clear();
    for (int i = 0; i < 8; ++i) {
        car.weights.append(0.0);
        car.barcodes.append(QString());
        car.slotStatuses.append(PlcProtocol::SlotStatusNormal);
    }

    QTableWidget *table = (carIndex == 1) ? ui->extraTable1 : ui->extraTable2;
    if (carIndex == 1) {
        table->setProperty("_tray1VehicleModel", QVariant());
        table->setProperty("_tray1Time", QVariant());
    } else {
        table->setProperty("_tray2VehicleModel", QVariant());
        table->setProperty("_tray2Time", QVariant());
    }
    table->setRowCount(0);

    refreshAllVisualizationDeviation();
}

void MainWindow::clearCurrentTableSlot(int carIndex, int slotIndex)
{
    QTableWidget *table = (carIndex == 1) ? ui->extraTable1 : ui->extraTable2;
    if (!table) return;
    if (carIndex == 1) {
        if (table->columnCount() < 9 || table->rowCount() < 3)
            return;
        const int col = trayVisualColForSlotIndex(slotIndex);
        QTableWidgetItem *wItem = table->item(2, col);
        QTableWidgetItem *bItem = table->item(1, col);
        if (wItem) {
            wItem->setText(QString());
            wItem->setData(Qt::ForegroundRole, QVariant());
        }
        if (bItem) {
            bItem->setText(QString());
            bItem->setData(Qt::ForegroundRole, QVariant());
        }
        QList<double> w8;
        for (int i = 0; i < 8; ++i)
            w8.append((i < m_car1Data.weights.size()) ? m_car1Data.weights[i] : 0.0);
        applyCurrentTableDeviationStyle(1, 2, w8);
        return;
    }
    if (carIndex == 2) {
        if (table->columnCount() < 9 || table->rowCount() < 3)
            return;
        const int col = trayVisualColForSlotIndex(slotIndex);
        QTableWidgetItem *wItem = table->item(2, col);
        QTableWidgetItem *bItem = table->item(1, col);
        if (wItem) {
            wItem->setText(QString());
            wItem->setData(Qt::ForegroundRole, QVariant());
        }
        if (bItem) {
            bItem->setText(QString());
            bItem->setData(Qt::ForegroundRole, QVariant());
        }
        QList<double> w8;
        for (int i = 0; i < 8; ++i)
            w8.append((i < m_car2Data.weights.size()) ? m_car2Data.weights[i] : 0.0);
        applyCurrentTableDeviationStyle(2, 2, w8);
        return;
    }
}

void MainWindow::updateCurrentTableSlot(int carIndex, int slotIndex, const QString &vehicleModel, double weightG, const QString &barcode)
{
    QTableWidget *table = (carIndex == 1) ? ui->extraTable1 : ui->extraTable2;
    if (!table) return;

    if (carIndex == 1) {
        PlcProtocol::FirstCarData &car = m_car1Data;
        QList<double> wts;
        QList<QString> bcs;
        for (int i = 0; i < 8; ++i) {
            wts.append((i < car.weights.size()) ? car.weights[i] : 0.0);
            bcs.append((i < car.barcodes.size()) ? car.barcodes[i] : QString());
        }
        QString vm = vehicleModel;
        if (vm.isEmpty()) {
            vm = getItemNameByCommand(QString::number(car.vehicleType));
            if (vm.isEmpty())
                vm = vehicleTypeToString(car.vehicleType);
        }
        QDateTime ts = QDateTime::fromString(table->property("_tray1Time").toString(),
                                              QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        if (!ts.isValid())
            ts = QDateTime::currentDateTime();
        fillTray1CurrentTable(vm, wts, bcs, ts);
        return;
    }

    if (carIndex == 2) {
        PlcProtocol::FirstCarData &car = m_car2Data;
        QList<double> wts;
        QList<QString> bcs;
        for (int i = 0; i < 8; ++i) {
            wts.append((i < car.weights.size()) ? car.weights[i] : 0.0);
            bcs.append((i < car.barcodes.size()) ? car.barcodes[i] : QString());
        }
        QString vm = vehicleModel;
        if (vm.isEmpty()) {
            vm = getItemNameByCommand(QString::number(car.vehicleType));
            if (vm.isEmpty())
                vm = vehicleTypeToString(car.vehicleType);
        }
        QDateTime ts = QDateTime::fromString(table->property("_tray2Time").toString(),
                                              QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        if (!ts.isValid())
            ts = QDateTime::currentDateTime();
        fillTray2CurrentTable(vm, wts, bcs, ts);
        return;
    }
}

void MainWindow::applyCurrentTableDeviationStyle(int carIndex, int row, const QList<double> &weights)
{
    QTableWidget *table = (carIndex == 1) ? ui->extraTable1 : ui->extraTable2;
    if (!table) return;
    const bool compactTray1 = (carIndex == 1 && table == ui->extraTable1
                               && table->columnCount() >= 9 && table->rowCount() >= 3);
    const bool compactTray2 = (carIndex == 2 && table == ui->extraTable2
                               && table->columnCount() >= 9 && table->rowCount() >= 3);
    const QList<int> &statuses = (carIndex == 1) ? m_car1Data.slotStatuses : m_car2Data.slotStatuses;
    double w[8];
    for (int i = 0; i < 8; ++i)
        w[i] = (i < weights.size()) ? weights[i] : 0.0;

    if (compactTray1 || compactTray2) {
        for (int i = 0; i < 8; ++i) {
            const int col = trayVisualColForSlotIndex(i);
            QTableWidgetItem *wItem = table->item(2, col);
            const bool hasData = wItem && !wItem->text().trimmed().isEmpty();
            const QString state = hasData ? slotStatusState(carIndex, i, w, statuses) : QString();
            for (int r = 0; r < 3; ++r) {
                QTableWidgetItem *cell = table->item(r, col);
                if (!cell)
                    continue;
                cell->setData(DeviationStateRole, state);
                updateCompactTrayCellAppearance(table, cell);
            }
        }
        resizeCompactTrayBarcodeRow(table);
        return;
    }

    if (row < 0 || row >= table->rowCount()) return;
    const QBrush alarmBrush(QColor(QStringLiteral("#ff5c5c")));
    const QBrush okBrush(carIndex == 2 ? QColor(0, 90, 200) : QColor(Qt::darkGreen));
    for (int i = 0; i < 8; ++i) {
        QTableWidgetItem *wItem = table->item(row, 1 + i * 2);
        QTableWidgetItem *bItem = table->item(row, 2 + i * 2);
        const QString state = slotStatusState(carIndex, i, w, statuses);
        if (state == QStringLiteral("alarm")) {
            if (wItem) wItem->setForeground(alarmBrush);
            if (bItem) bItem->setForeground(alarmBrush);
        } else if (state == QStringLiteral("ok") || state == QStringLiteral("ok_blue")) {
            if (wItem) wItem->setForeground(okBrush);
            if (bItem) bItem->setForeground(okBrush);
        } else {
            if (wItem) wItem->setData(Qt::ForegroundRole, QVariant());
            if (bItem) bItem->setData(Qt::ForegroundRole, QVariant());
        }
    }
}

void MainWindow::addWeightData(const WeightData &data, int tableIndex)
{
    // 不再保存称重记录到数据库，仅刷新内存历史表显示（若手动完成仍调用）
    // DatabaseManager::instance().insertWeightRecord(data, tableIndex);
    Q_UNUSED(data);
    Q_UNUSED(tableIndex);
    Logger::info(QStringLiteral("跳过写入历史数据库（收到即显示模式）"));
    /* 旧逻辑：
    if (tableIndex == 1) {
        m_weightDataList1.append(data);
        updateWeightTable(ui->weightTable1, m_weightDataList1);
    } else if (tableIndex == 2) {
        m_weightDataList2.append(data);
        updateWeightTable(ui->weightTable2, m_weightDataList2);
    }
    */
}

void MainWindow::updateWeightTable(QTableWidget *table, const QList<WeightData> &dataList)
{
    applyTableBarcodeWrapSettings(table);
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
            auto *bcItem = new QTableWidgetItem(barcode);
            if (!barcode.isEmpty())
                bcItem->setToolTip(barcode);
            const int bcCol = col;
            table->setItem(i, col++, bcItem);
            applyBarcodeCellSizeHint(table, i, bcCol, bcItem);
        }
        table->setItem(i, col++, new QTableWidgetItem(data.timestamp().toString("yyyy-MM-dd hh:mm:ss")));
    }
    QList<int> barcodeCols;
    for (int j = 0; j < 8; ++j)
        barcodeCols.append(2 + j * 2);
    resizeTableRowsForBarcodeColumns(table, barcodeCols);
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
