#ifndef NGADDDIALOG_H
#define NGADDDIALOG_H

#include <QDialog>
#include <QString>

class NgAddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NgAddDialog(QWidget *parent = nullptr);

    QString vehicleModel() const;
    QString barcode() const;
    double weightGrams() const;  // 重量(克)

private:
    class QLineEdit *m_vehicleModelEdit;
    class QLineEdit *m_barcodeEdit;
    class QLineEdit *m_weightEdit;
};

#endif // NGADDDIALOG_H
