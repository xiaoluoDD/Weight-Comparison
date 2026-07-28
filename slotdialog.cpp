#include "slotdialog.h"
#include <QFormLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>

SlotDialog::SlotDialog(const QString &vehicleModel, double weightG, const QString &barcode,
                       QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("槽位详情"));
    setMinimumWidth(320);

    QFormLayout *layout = new QFormLayout(this);

    layout->addRow(QStringLiteral("车型:"), new QLabel(vehicleModel.isEmpty() ? QStringLiteral("-") : vehicleModel));
    layout->addRow(QStringLiteral("重量 (g):"), new QLabel(QString::number(weightG, 'f', 1)));  // 内部克(g)
    layout->addRow(QStringLiteral("条码:"), new QLabel(barcode.isEmpty() ? QStringLiteral("-") : barcode));

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    QPushButton *addNgBtn = buttons->addButton(QStringLiteral("添加到NG"), QDialogButtonBox::ActionRole);
    QPushButton *closeBtn = buttons->addButton(QStringLiteral("关闭"), QDialogButtonBox::RejectRole);

    connect(addNgBtn, &QPushButton::clicked, this, &SlotDialog::onAddToNgClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    layout->addRow(buttons);
}

void SlotDialog::onAddToNgClicked()
{
    m_addToNgRequested = true;
    accept();
}
