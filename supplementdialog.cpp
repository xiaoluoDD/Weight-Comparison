#include "supplementdialog.h"
#include <QComboBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>

SupplementDialog::SupplementDialog(const QMap<QString, QString> &bindingMap, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("生产补充"));
    setMinimumWidth(300);

    m_trayCombo = new QComboBox(this);
    m_trayCombo->addItem(QStringLiteral("左车"), 1);
    m_trayCombo->addItem(QStringLiteral("右车"), 2);

    m_vehicleCombo = new QComboBox(this);
    for (auto it = bindingMap.constBegin(); it != bindingMap.constEnd(); ++it) {
        m_vehicleCombo->addItem(it.value(), it.key());  // 显示车型名称，data 为 command
    }
    if (m_vehicleCombo->count() == 0) {
        m_vehicleCombo->addItem(QStringLiteral("12V机型"), QString("1"));
        m_vehicleCombo->addItem(QStringLiteral("16V机型"), QString("2"));
    }

    m_quantitySpin = new QSpinBox(this);
    m_quantitySpin->setMinimum(1);
    m_quantitySpin->setMaximum(999);
    m_quantitySpin->setValue(1);

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(QStringLiteral("车:"), m_trayCombo);
    layout->addRow(QStringLiteral("车型:"), m_vehicleCombo);
    layout->addRow(QStringLiteral("补充数量:"), m_quantitySpin);

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    buttons->addButton(QDialogButtonBox::Ok);
    buttons->addButton(QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(buttons);
}

int SupplementDialog::trayIndex() const
{
    return m_trayCombo->currentData().toInt();
}

QString SupplementDialog::vehicleCommand() const
{
    return m_vehicleCombo->currentData().toString();
}

int SupplementDialog::supplementQuantity() const
{
    return m_quantitySpin->value();
}
