#include "ngadddialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>

NgAddDialog::NgAddDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("添加NG品"));
    setMinimumWidth(300);

    m_vehicleModelEdit = new QLineEdit(this);
    m_vehicleModelEdit->setPlaceholderText(QStringLiteral("输入车型名称"));

    m_barcodeEdit = new QLineEdit(this);
    m_barcodeEdit->setPlaceholderText(QStringLiteral("输入条码"));

    m_weightEdit = new QLineEdit(this);
    m_weightEdit->setPlaceholderText(QStringLiteral("输入重量(g)，如 123.5"));

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(QStringLiteral("车型名称:"), m_vehicleModelEdit);
    layout->addRow(QStringLiteral("条码:"), m_barcodeEdit);
    layout->addRow(QStringLiteral("重量(g):"), m_weightEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    buttons->addButton(QDialogButtonBox::Ok);
    buttons->addButton(QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(buttons);
}

QString NgAddDialog::vehicleModel() const
{
    return m_vehicleModelEdit->text().trimmed();
}

QString NgAddDialog::barcode() const
{
    return m_barcodeEdit->text().trimmed();
}

double NgAddDialog::weightGrams() const
{
    return m_weightEdit->text().trimmed().toDouble();
}
