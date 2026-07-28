#ifndef NGUSEDIALOG_H
#define NGUSEDIALOG_H

#include <QDialog>

class NgUseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NgUseDialog(QWidget *parent = nullptr);

    // 获取用户选择：carIndex 1或2，slotIndex 0-7
    int carIndex() const;
    int slotIndex() const;

private:
    class QComboBox *m_carCombo;
    class QComboBox *m_slotCombo;
};

#endif // NGUSEDIALOG_H
