#include "ngusedialog.h"
#include <QComboBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>

NgUseDialog::NgUseDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("选择放置位置"));
    setMinimumWidth(280);

    m_carCombo = new QComboBox(this);
    m_carCombo->addItem(QStringLiteral("左车"), 1);
    m_carCombo->addItem(QStringLiteral("右车"), 2);

    m_slotCombo = new QComboBox(this);
    for (int i = 1; i <= 8; ++i)
        m_slotCombo->addItem(QStringLiteral("槽位 %1").arg(i), i - 1);

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(QStringLiteral("车:"), m_carCombo);
    layout->addRow(QStringLiteral("槽位:"), m_slotCombo);

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    buttons->addButton(QDialogButtonBox::Ok);
    buttons->addButton(QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(buttons);
}

int NgUseDialog::carIndex() const
{
    return m_carCombo->currentData().toInt();
}

int NgUseDialog::slotIndex() const
{
    return m_slotCombo->currentData().toInt();
}
