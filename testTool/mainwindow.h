#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QDoubleSpinBox>
#include <QLineEdit>

#include <QComboBox>

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
    void onStartServerClicked();
    void onStopServerClicked();
    void onSendClicked();
    void onNewConnection();
    void onClientDisconnected();

private:
    Ui::MainWindow *ui;
    QTcpServer *m_server;
    QList<QTcpSocket *> m_clients;

    QList<QDoubleSpinBox *> m_car1WeightSpinBox;
    QList<QLineEdit *> m_car1BarcodeEdit;
    QList<QComboBox *> m_car1StatusCombo;
    QList<QDoubleSpinBox *> m_car2WeightSpinBox;
    QList<QLineEdit *> m_car2BarcodeEdit;
    QList<QComboBox *> m_car2StatusCombo;

    void setupCarWidgets();
    void updateServerStatus();
    QByteArray buildPacket();  // 生成 432 字节协议报文
    void appendLog(const QString &msg);
};

#endif // MAINWINDOW_H
