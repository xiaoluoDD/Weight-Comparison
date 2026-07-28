#ifndef SLOTDIALOG_H
#define SLOTDIALOG_H

#include <QDialog>
#include <QString>

class SlotDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SlotDialog(const QString &vehicleModel, double weightG, const QString &barcode,
                       QWidget *parent = nullptr);

    bool addToNgRequested() const { return m_addToNgRequested; }

private slots:
    void onAddToNgClicked();

private:
    bool m_addToNgRequested = false;
};

#endif // SLOTDIALOG_H
