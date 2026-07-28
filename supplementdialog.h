#ifndef SUPPLEMENTDIALOG_H
#define SUPPLEMENTDIALOG_H

#include <QDialog>

class SupplementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SupplementDialog(const QMap<QString, QString> &bindingMap, QWidget *parent = nullptr);

    // 获取用户选择
    int trayIndex() const;      // 1 或 2
    QString vehicleCommand() const;  // 车型代码，对应绑定表 command
    int supplementQuantity() const;

private:
    class QComboBox *m_trayCombo;
    class QComboBox *m_vehicleCombo;
    class QSpinBox *m_quantitySpin;
};

#endif // SUPPLEMENTDIALOG_H
