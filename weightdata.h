#ifndef WEIGHTDATA_H
#define WEIGHTDATA_H

#include <QDateTime>
#include <QString>
#include <QList>

class WeightData
{
public:
    WeightData();
    WeightData(const QDateTime &timestamp, double value, const QString &unit, const QString &status);
    
    // Getter方法
    QDateTime timestamp() const;
    double value() const;
    QString unit() const;
    QString status() const;
    QString itemName() const;
    QString vehicleModel() const;  // 车型名称
    QString barcode() const;        // 条码
    QList<double> weights() const; // 8个重量值列表
    
    // Setter方法
    void setTimestamp(const QDateTime &timestamp);
    void setValue(double value);
    void setUnit(const QString &unit);
    void setStatus(const QString &status);
    void setItemName(const QString &itemName);
    void setVehicleModel(const QString &vehicleModel);
    void setBarcode(const QString &barcode);
    void setWeights(const QList<double> &weights);
    void setWeight(int slotIndex, double weight);  // 设置指定槽位的重量（1-8）
    
    // 比较操作符（用于对比功能）
    bool operator==(const WeightData &other) const;
    bool operator!=(const WeightData &other) const;
    
    // 转换为字符串
    QString toString() const;

private:
    QDateTime m_timestamp;  // 时间戳
    double m_value;         // 重量值（保留用于兼容）
    QString m_unit;         // 单位（kg, g, t等）
    QString m_status;       // 状态（正常、异常等）
    QString m_itemName;     // 物品名称
    QString m_vehicleModel; // 车型名称
    QString m_barcode;      // 条码
    QList<double> m_weights; // 8个槽位的重量值列表
};

#endif // WEIGHTDATA_H
