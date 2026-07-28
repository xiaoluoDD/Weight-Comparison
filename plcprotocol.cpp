#include "plcprotocol.h"
#include <cstring>

namespace PlcProtocol {

// 大端写 Int16
static void writeInt16BE(QByteArray &data, int offset, qint16 value) {
    if (data.size() < offset + 2) data.resize(offset + 2);
    data[offset] = static_cast<char>((value >> 8) & 0xFF);
    data[offset + 1] = static_cast<char>(value & 0xFF);
}

// 大端写 Float
static void writeFloat32BE(QByteArray &data, int offset, float value) {
    if (data.size() < offset + 4) data.resize(offset + 4);
    quint32 u;
    memcpy(&u, &value, 4);
    data[offset] = static_cast<char>((u >> 24) & 0xFF);
    data[offset + 1] = static_cast<char>((u >> 16) & 0xFF);
    data[offset + 2] = static_cast<char>((u >> 8) & 0xFF);
    data[offset + 3] = static_cast<char>(u & 0xFF);
}

// 写 20 字节二维码字段（无头格式，最多 20 字节 ASCII，不足填 0）
static void writeBarcode22(QByteArray &data, int offset, const QString &barcode) {
    if (data.size() < offset + BarcodeFieldSize) data.resize(offset + BarcodeFieldSize);
    QByteArray raw = barcode.left(BarcodeEffectiveBytes).toUtf8();
    int usedLen = qMin(raw.size(), BarcodeEffectiveBytes);
    for (int i = 0; i < BarcodeFieldSize; ++i)
        data[offset + i] = (i < usedLen) ? raw[i] : '\0';
}

QString readBarcode22(const QByteArray &data, int offset)
{
    if (data.size() < offset + BarcodeFieldSize)
        return QString();
    QByteArray raw = data.mid(offset, BarcodeFieldSize);
    int end = raw.size();
    while (end > 0) {
        const unsigned char ch = static_cast<unsigned char>(raw.at(end - 1));
        if (ch == 0 || ch == ' ')
            --end;
        else
            break;
    }
    int start = 0;
    while (start < end && raw.at(start) == '\0')
        ++start;
    if (start >= end)
        return QString();
    QByteArray slice = raw.mid(start, end - start);
    const int nil = slice.indexOf('\0');
    if (nil >= 0)
        slice = slice.left(nil);
    return QString::fromLatin1(slice).trimmed();
}

bool parseFirstCarBlock(const QByteArray &block, FirstCarData &out)
{
    if (block.size() < CarBlockSize) return false;

    out.productionMode = readInt16BE(block, Offset_ProductionMode);
    out.vehicleType    = readInt16BE(block, Offset_VehicleType);
    out.assemblyDone   = readInt16BE(block, Offset_AssemblyDone);

    out.weights.clear();
    out.barcodes.clear();
    out.slotStatuses.clear();
    for (int i = 0; i < 8; ++i) {
        int wo = absoluteWeightOffset(i);
        int bo = absoluteBarcodeOffset(i);
        if (wo < 0 || bo < 0 || block.size() < bo + BarcodeFieldSize) return false;
        double wKg = static_cast<double>(readFloat32BE(block, wo));
        double wG = wKg * 1000.0;
        out.weights.append(qRound(wG * 10.0) / 10.0);  // 协议kg转g，四舍五入1位小数消除float精度误差
        out.barcodes.append(readBarcode22(block, bo));
        out.slotStatuses.append(SlotStatusNormal);  // 状态由整帧末尾16个状态字填充
    }
    return true;
}

bool parseTwoCarPacket(const QByteArray &packet, TwoCarPacket &out)
{
    if (packet.size() < FullPacketSize) return false;

    if (!parseFirstCarBlock(packet.mid(0, CarBlockSize), out.car1))
        return false;
    if (!parseFirstCarBlock(packet.mid(SecondCarBlockOffset, CarBlockSize), out.car2))
        return false;

    // 整帧末尾：2 空字跳过；其后 16 个状态字（大端 Int16）= 左车8 + 右车8
    out.car1.slotStatuses.clear();
    out.car2.slotStatuses.clear();
    for (int i = 0; i < SlotStatusPerCar; ++i) {
        const int o1 = absolutePacketSlotStatusOffset(1, i);
        const int o2 = absolutePacketSlotStatusOffset(2, i);
        if (o1 < 0 || o2 < 0 || packet.size() < o2 + BytesPerWord)
            return false;
        out.car1.slotStatuses.append(static_cast<int>(readInt16BE(packet, o1)));
        out.car2.slotStatuses.append(static_cast<int>(readInt16BE(packet, o2)));
    }
    return true;
}

QByteArray buildSupplementPacket(int trayIndex, int vehicleType, int supplementQty,
    const QList<double> &weights, const QList<QString> &barcodes)
{
    QByteArray packet(FullPacketSize, '\0');

    auto fillCarBlock = [&](int blockOffset, int prodMode, int vType, int asmDone,
                           const QList<double> &w, const QList<QString> &b) {
        writeInt16BE(packet, blockOffset + Offset_ProductionMode, prodMode);
        writeInt16BE(packet, blockOffset + Offset_VehicleType, vType);
        writeInt16BE(packet, blockOffset + Offset_AssemblyDone, asmDone);
        for (int i = 0; i < 8; ++i) {
            double wG = (i < w.size()) ? w[i] : 0.0;
            float wKg = static_cast<float>(wG / 1000.0);
            writeFloat32BE(packet, blockOffset + absoluteWeightOffset(i), wKg);
            QString bc = (i < b.size()) ? b[i] : QString();
            writeBarcode22(packet, blockOffset + absoluteBarcodeOffset(i), bc);
        }
    };

    // 补充生产=1，装件完成=补充数量
    const int prodMode = 1;
    if (trayIndex == 1) {
        fillCarBlock(0, prodMode, vehicleType, supplementQty, weights, barcodes);
        fillCarBlock(SecondCarBlockOffset, 0, 0, 0, QList<double>(), QList<QString>());
    } else {
        fillCarBlock(0, 0, 0, 0, QList<double>(), QList<QString>());
        fillCarBlock(SecondCarBlockOffset, prodMode, vehicleType, supplementQty, weights, barcodes);
    }
    return packet;
}

QByteArray buildDetectionOkPacketTray1(const QList<double> &weights1, const QList<QString> &barcodes1, int vehicleType1)
{
    QByteArray packet(FullPacketSize, '\0');
    // car1: productionMode=2(检测ok)，携带第一托物品数据
    writeInt16BE(packet, 0 + Offset_ProductionMode, 2);
    writeInt16BE(packet, 0 + Offset_VehicleType, vehicleType1);
    writeInt16BE(packet, 0 + Offset_AssemblyDone, 1);
    for (int i = 0; i < 8; ++i) {
        double wG = (i < weights1.size()) ? weights1[i] : 0.0;
        float wKg = static_cast<float>(wG / 1000.0);
        writeFloat32BE(packet, 0 + absoluteWeightOffset(i), wKg);
        QString bc = (i < barcodes1.size()) ? barcodes1[i] : QString();
        writeBarcode22(packet, 0 + absoluteBarcodeOffset(i), bc);
    }
    // car2: 全0（packet已初始化为\0）
    return packet;
}

QByteArray buildDetectionOkPacketBoth(const QList<double> &weights1, const QList<QString> &barcodes1, int vehicleType1,
    const QList<double> &weights2, const QList<QString> &barcodes2, int vehicleType2)
{
    QByteArray packet(FullPacketSize, '\0');
    // car1
    writeInt16BE(packet, 0 + Offset_ProductionMode, 2);
    writeInt16BE(packet, 0 + Offset_VehicleType, vehicleType1);
    writeInt16BE(packet, 0 + Offset_AssemblyDone, 1);
    for (int i = 0; i < 8; ++i) {
        double wG = (i < weights1.size()) ? weights1[i] : 0.0;
        float wKg = static_cast<float>(wG / 1000.0);
        writeFloat32BE(packet, 0 + absoluteWeightOffset(i), wKg);
        QString bc = (i < barcodes1.size()) ? barcodes1[i] : QString();
        writeBarcode22(packet, 0 + absoluteBarcodeOffset(i), bc);
    }
    // car2
    writeInt16BE(packet, SecondCarBlockOffset + Offset_ProductionMode, 2);
    writeInt16BE(packet, SecondCarBlockOffset + Offset_VehicleType, vehicleType2);
    writeInt16BE(packet, SecondCarBlockOffset + Offset_AssemblyDone, 1);
    for (int i = 0; i < 8; ++i) {
        double wG = (i < weights2.size()) ? weights2[i] : 0.0;
        float wKg = static_cast<float>(wG / 1000.0);
        writeFloat32BE(packet, SecondCarBlockOffset + absoluteWeightOffset(i), wKg);
        QString bc = (i < barcodes2.size()) ? barcodes2[i] : QString();
        writeBarcode22(packet, SecondCarBlockOffset + absoluteBarcodeOffset(i), bc);
    }
    return packet;
}

} // namespace PlcProtocol
