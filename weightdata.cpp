#include "weightdata.h"

WeightData::WeightData()
    : m_value(0.0)
{
    m_timestamp = QDateTime::currentDateTime();
    m_unit = "kg";
    m_status = "正常";
    m_itemName = "";
    m_vehicleModel = "";
    m_barcode = "";
    m_barcodes = QList<QString>(8, QString());
    m_weights = QList<double>(8, 0.0);  // 初始化8个重量值为0
}

WeightData::WeightData(const QDateTime &timestamp, double value, const QString &unit, const QString &status)
    : m_timestamp(timestamp)
    , m_value(value)
    , m_unit(unit)
    , m_status(status)
    , m_itemName("")
    , m_vehicleModel("")
    , m_barcode("")
    , m_barcodes(QList<QString>(8, QString()))
    , m_weights(QList<double>(8, 0.0))
{
}

QDateTime WeightData::timestamp() const
{
    return m_timestamp;
}

double WeightData::value() const
{
    return m_value;
}

QString WeightData::unit() const
{
    return m_unit;
}

QString WeightData::status() const
{
    return m_status;
}

QString WeightData::itemName() const
{
    return m_itemName;
}

void WeightData::setTimestamp(const QDateTime &timestamp)
{
    m_timestamp = timestamp;
}

void WeightData::setValue(double value)
{
    m_value = value;
}

void WeightData::setUnit(const QString &unit)
{
    m_unit = unit;
}

void WeightData::setStatus(const QString &status)
{
    m_status = status;
}

void WeightData::setItemName(const QString &itemName)
{
    m_itemName = itemName;
}

QString WeightData::vehicleModel() const
{
    return m_vehicleModel;
}

QString WeightData::barcode() const
{
    return m_barcodes.isEmpty() ? m_barcode : m_barcodes.first();
}

QList<QString> WeightData::barcodes() const
{
    return m_barcodes;
}

QList<double> WeightData::weights() const
{
    return m_weights;
}

void WeightData::setVehicleModel(const QString &vehicleModel)
{
    m_vehicleModel = vehicleModel;
}

void WeightData::setBarcode(const QString &barcode)
{
    m_barcode = barcode;
}

void WeightData::setBarcodes(const QList<QString> &barcodes)
{
    m_barcodes = barcodes;
    while (m_barcodes.size() < 8) m_barcodes.append(QString());
    if (m_barcodes.size() > 8) m_barcodes = m_barcodes.mid(0, 8);
}

void WeightData::setWeights(const QList<double> &weights)
{
    m_weights = weights;
    // 确保有8个值
    while (m_weights.size() < 8) {
        m_weights.append(0.0);
    }
    if (m_weights.size() > 8) {
        m_weights = m_weights.mid(0, 8);
    }
}

void WeightData::setWeight(int slotIndex, double weight)
{
    // slotIndex范围是1-8，转换为0-7的索引
    if (slotIndex >= 1 && slotIndex <= 8) {
        int index = slotIndex - 1;
        // 确保列表有足够的大小
        while (m_weights.size() <= index) {
            m_weights.append(0.0);
        }
        m_weights[index] = weight;
    }
}

bool WeightData::operator==(const WeightData &other) const
{
    // 可以根据需要调整比较逻辑
    // 这里简单比较重量值（可以添加容差）
    return qAbs(m_value - other.m_value) < 0.001 && m_unit == other.m_unit;
}

bool WeightData::operator!=(const WeightData &other) const
{
    return !(*this == other);
}

QString WeightData::toString() const
{
    QString itemInfo = m_itemName.isEmpty() ? "" : QString(", 物品: %1").arg(m_itemName);
    return QString("时间: %1, 重量: %2 %3, 状态: %4%5")
            .arg(m_timestamp.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(m_value, 0, 'f', 3)
            .arg(m_unit)
            .arg(m_status)
            .arg(itemInfo);
}
