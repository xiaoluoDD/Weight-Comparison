#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <cstring>

// 协议常量（与称重软件 plcprotocol.h 一致，432 字节 = 2×198 + 末尾18字）
namespace {
constexpr int CarBlockSize = 198;
constexpr int FullPacketSize = 432;
constexpr int BarcodeFieldSize = 20;
constexpr int BarcodeEffectiveBytes = 20;
constexpr int SlotStatusPerCar = 8;

void writeInt16BE(QByteArray &out, qint16 val) {
    out.append(static_cast<char>((val >> 8) & 0xFF));
    out.append(static_cast<char>(val & 0xFF));
}

void writeFloat32BE(QByteArray &out, float val) {
    quint32 u;
    memcpy(&u, &val, 4);
    out.append(static_cast<char>((u >> 24) & 0xFF));
    out.append(static_cast<char>((u >> 16) & 0xFF));
    out.append(static_cast<char>((u >> 8) & 0xFF));
    out.append(static_cast<char>(u & 0xFF));
}

void writeBarcode22(QByteArray &out, const QString &barcode) {
    QByteArray content = barcode.left(BarcodeEffectiveBytes).toUtf8();
    int usedLen = qMin(content.size(), BarcodeEffectiveBytes);
    for (int i = 0; i < BarcodeFieldSize; ++i)
        out.append((i < usedLen) ? content[i] : '\0');
}

QString defaultBarcode20(const QString &prefix, int workpieceIndex1Based)
{
    QString core = QStringLiteral("%1%2").arg(prefix).arg(workpieceIndex1Based, 3, 10, QChar('0'));
    if (core.size() > BarcodeEffectiveBytes)
        return core.left(BarcodeEffectiveBytes);
    return core.leftJustified(BarcodeEffectiveBytes, QLatin1Char('0'));
}
} // anonymous namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_server(new QTcpServer(this))
{
    ui->setupUi(this);

    setupCarWidgets();

    connect(m_server, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
    connect(ui->startServerBtn, &QPushButton::clicked, this, &MainWindow::onStartServerClicked);
    connect(ui->stopServerBtn, &QPushButton::clicked, this, &MainWindow::onStopServerClicked);
    connect(ui->sendBtn, &QPushButton::clicked, this, &MainWindow::onSendClicked);

    ui->stopServerBtn->setEnabled(false);
    ui->sendBtn->setEnabled(false);
    updateServerStatus();
}

void MainWindow::setupCarWidgets()
{
    auto setupCombo = [](QComboBox *combo, const QVector<QPair<QString, int>> &items) {
        combo->clear();
        for (const auto &p : items)
            combo->addItem(p.first, p.second);
    };

    setupCombo(ui->car1ProdModeCombo, {{QStringLiteral("1-正常生产"), 1}, {QStringLiteral("2-补充生产"), 2}});
    setupCombo(ui->car1VehicleCombo, {{QStringLiteral("1-12V机型"), 1}, {QStringLiteral("2-16V机型"), 2}});
    setupCombo(ui->car1AssemblyCombo, {{QStringLiteral("0-未完成"), 0}, {QStringLiteral("1-装件中"), 1}, {QStringLiteral("2-装件完成"), 2}});
    setupCombo(ui->car2ProdModeCombo, {{QStringLiteral("1-正常生产"), 1}, {QStringLiteral("2-补充生产"), 2}});
    setupCombo(ui->car2VehicleCombo, {{QStringLiteral("1-12V机型"), 1}, {QStringLiteral("2-16V机型"), 2}});
    setupCombo(ui->car2AssemblyCombo, {{QStringLiteral("0-未完成"), 0}, {QStringLiteral("1-装件中"), 1}, {QStringLiteral("2-装件完成"), 2}});
    ui->car1AssemblyCombo->setCurrentIndex(2);
    ui->car2AssemblyCombo->setCurrentIndex(2);

    QWidget *car1Widget = ui->car1ScrollArea->widget();
    QWidget *car2Widget = ui->car2ScrollArea->widget();
    if (!car1Widget) {
        car1Widget = new QWidget();
        ui->car1ScrollArea->setWidget(car1Widget);
    }
    if (!car2Widget) {
        car2Widget = new QWidget();
        ui->car2ScrollArea->setWidget(car2Widget);
    }
    QGridLayout *grid1 = car1Widget->findChild<QGridLayout *>("gridLayout_car1Workpieces");
    QGridLayout *grid2 = car2Widget->findChild<QGridLayout *>("gridLayout_car2Workpieces");
    if (!grid1) grid1 = new QGridLayout(car1Widget);
    if (!grid2) grid2 = new QGridLayout(car2Widget);

    for (int i = 0; i < 8; ++i) {
        auto *w1 = new QDoubleSpinBox(this);
        w1->setRange(0, 999999.99);
        w1->setDecimals(2);
        w1->setValue((i + 1) * 10.0);  // 10g … 80g
        w1->setSuffix(QStringLiteral(" g"));
        m_car1WeightSpinBox.append(w1);

        auto *b1 = new QLineEdit(this);
        b1->setMaxLength(BarcodeEffectiveBytes);
        b1->setPlaceholderText(QStringLiteral("20位条码 %1").arg(i + 1));
        b1->setText(defaultBarcode20(QStringLiteral("TEST"), i + 1));
        m_car1BarcodeEdit.append(b1);

        auto *s1 = new QComboBox(this);
        s1->addItem(QStringLiteral("0-正常"), 0);
        s1->addItem(QStringLiteral("1-异常"), 1);
        s1->setCurrentIndex(0);
        m_car1StatusCombo.append(s1);

        grid1->addWidget(new QLabel(QStringLiteral("工件%1:").arg(i + 1)), i, 0);
        grid1->addWidget(w1, i, 1);
        grid1->addWidget(b1, i, 2);
        grid1->addWidget(s1, i, 3);

        auto *w2 = new QDoubleSpinBox(this);
        w2->setRange(0, 999999.99);
        w2->setDecimals(2);
        w2->setValue((i + 1) * 10.0);
        w2->setSuffix(QStringLiteral(" g"));
        m_car2WeightSpinBox.append(w2);

        auto *b2 = new QLineEdit(this);
        b2->setMaxLength(BarcodeEffectiveBytes);
        b2->setPlaceholderText(QStringLiteral("20位条码 %1").arg(i + 1));
        b2->setText(defaultBarcode20(QStringLiteral("BC002"), i + 1));
        m_car2BarcodeEdit.append(b2);

        auto *s2 = new QComboBox(this);
        s2->addItem(QStringLiteral("0-正常"), 0);
        s2->addItem(QStringLiteral("1-异常"), 1);
        s2->setCurrentIndex(0);
        m_car2StatusCombo.append(s2);

        grid2->addWidget(new QLabel(QStringLiteral("工件%1:").arg(i + 1)), i, 0);
        grid2->addWidget(w2, i, 1);
        grid2->addWidget(b2, i, 2);
        grid2->addWidget(s2, i, 3);
    }
}

MainWindow::~MainWindow()
{
    if (m_server->isListening())
        m_server->close();
    for (QTcpSocket *s : m_clients)
        s->disconnectFromHost();
    delete ui;
}

void MainWindow::onStartServerClicked()
{
    int port = ui->portSpinBox->value();
    if (m_server->listen(QHostAddress::Any, port)) {
        appendLog(QString("服务器已启动，监听端口 %1").arg(port));
        ui->startServerBtn->setEnabled(false);
        ui->stopServerBtn->setEnabled(true);
        ui->sendBtn->setEnabled(true);
        ui->portSpinBox->setEnabled(false);
    } else {
        QMessageBox::critical(this, QStringLiteral("错误"),
            QString("启动失败: %1").arg(m_server->errorString()));
    }
    updateServerStatus();
}

void MainWindow::onStopServerClicked()
{
    m_server->close();
    for (QTcpSocket *s : m_clients) {
        s->disconnectFromHost();
        s->deleteLater();
    }
    m_clients.clear();
    appendLog(QStringLiteral("服务器已停止"));
    ui->startServerBtn->setEnabled(true);
    ui->stopServerBtn->setEnabled(false);
    ui->sendBtn->setEnabled(false);
    ui->portSpinBox->setEnabled(true);
    updateServerStatus();
}

void MainWindow::onSendClicked()
{
    QByteArray packet = buildPacket();
    if (packet.size() != FullPacketSize) {
        QMessageBox::warning(this, QStringLiteral("警告"),
            QString("生成报文长度异常: %1 字节，应为 %2 字节").arg(packet.size()).arg(FullPacketSize));
        return;
    }

    int sent = 0;
    for (QTcpSocket *client : m_clients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            qint64 n = client->write(packet);
            if (n == packet.size())
                ++sent;
        }
    }
    appendLog(QString("已发送 %1 字节 到 %2 个客户端").arg(packet.size()).arg(sent));
    if (m_clients.isEmpty())
        appendLog(QStringLiteral("提示: 当前无客户端连接，请先在称重软件中连接本服务器"));
}

void MainWindow::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        connect(client, &QTcpSocket::disconnected, this, &MainWindow::onClientDisconnected);
        m_clients.append(client);
        appendLog(QString("客户端已连接: %1:%2").arg(client->peerAddress().toString()).arg(client->peerPort()));
    }
    updateServerStatus();
}

void MainWindow::onClientDisconnected()
{
    QTcpSocket *s = qobject_cast<QTcpSocket *>(sender());
    if (s) {
        m_clients.removeAll(s);
        s->deleteLater();
        appendLog(QStringLiteral("客户端已断开"));
    }
    updateServerStatus();
}

void MainWindow::updateServerStatus()
{
    bool listening = m_server->isListening();
    QString status = listening
        ? QString("运行中 | 端口 %1 | 客户端 %2 个").arg(ui->portSpinBox->value()).arg(m_clients.size())
        : QStringLiteral("已停止");
    ui->statusLabel->setText(status);
}

QByteArray MainWindow::buildPacket()
{
    QByteArray packet;

    auto buildCarBlock = [&](int prodMode, int vehicleType, int assemblyDone,
                            const QList<QDoubleSpinBox *> &weights,
                            const QList<QLineEdit *> &barcodes) {
        QByteArray block;
        writeInt16BE(block, static_cast<qint16>(prodMode));
        writeInt16BE(block, static_cast<qint16>(vehicleType));
        writeInt16BE(block, static_cast<qint16>(assemblyDone));
        for (int i = 0; i < 8; ++i) {
            float wG = (i < weights.size()) ? static_cast<float>(weights[i]->value()) : 0.0f;
            float w = wG / 1000.0f;
            QString bc = (i < barcodes.size()) ? barcodes[i]->text().trimmed() : QString();
            writeFloat32BE(block, w);
            writeBarcode22(block, bc);
        }
        return block;
    };

    packet.append(buildCarBlock(
        ui->car1ProdModeCombo->currentData().toInt(),
        ui->car1VehicleCombo->currentData().toInt(),
        ui->car1AssemblyCombo->currentData().toInt(),
        m_car1WeightSpinBox, m_car1BarcodeEdit));
    packet.append(buildCarBlock(
        ui->car2ProdModeCombo->currentData().toInt(),
        ui->car2VehicleCombo->currentData().toInt(),
        ui->car2AssemblyCombo->currentData().toInt(),
        m_car2WeightSpinBox, m_car2BarcodeEdit));

    // 整帧末尾：2 个空字（4 字节）+ 16 个状态字（左车8 + 右车8，大端 Int16）
    writeInt16BE(packet, 0);
    writeInt16BE(packet, 0);
    for (int i = 0; i < SlotStatusPerCar; ++i) {
        const qint16 st = static_cast<qint16>(
            (i < m_car1StatusCombo.size()) ? m_car1StatusCombo[i]->currentData().toInt() : 0);
        writeInt16BE(packet, st);
    }
    for (int i = 0; i < SlotStatusPerCar; ++i) {
        const qint16 st = static_cast<qint16>(
            (i < m_car2StatusCombo.size()) ? m_car2StatusCombo[i]->currentData().toInt() : 0);
        writeInt16BE(packet, st);
    }

    return packet;
}

void MainWindow::appendLog(const QString &msg)
{
    QString line = QString("[%1] %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), msg);
    ui->logEdit->appendPlainText(line);
}
