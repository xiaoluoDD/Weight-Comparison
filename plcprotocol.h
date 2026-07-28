#ifndef PLCPROTOCOL_H
#define PLCPROTOCOL_H

#include <QtGlobal>
#include <QByteArray>
#include <QString>
#include <QList>
#include <cstring>

/**
 * PLC 活连称重数据协议解析
 *
 * 一帧完整报文：432 字节 = 第一车 198 + 第二车 198 + 末尾 18 字（36 字节），无包头
 * - 0~197:     第一车数据（198 字节）
 * - 198~395:   第二车数据（198 字节）
 * - 396~399:   2 个空字（预留，4 字节）
 * - 400~431:   16 个状态字（32 字节，大端 Int16）：左车工件1~8 + 右车工件1~8
 *
 * 单车数据块布局（198 字节），两车格式一致：
 *
 * 【车头 6 字节】
 * - 0~1:   Int16  正常生产/补充生产  1=正常生产 2=补充生产
 * - 2~3:   Int16  车型              1=12V机型  2=16V机型
 * - 4~5:   Int16  装件完成标志      0=未完成 1=装件中 2=装件完成
 *
 * 【接上 8 组工件】每组：4 字节重量（Float 大端）+ 20 字节二维码
 * - 重量：4 字节，IEEE754 大端（协议单位 kg，软件内转 g）
 * - 二维码：20 字节，无头格式，有效 ASCII 最多 20 字节
 *
 * 单车道 = 6 + 8×(4+20) = 6 + 192 = 198 字节
 *
 * 【整帧末尾 18 字 = 36 字节】
 * - 2 空字 + 16 状态字
 * - 状态字：0 = 正常（界面绿色），1 = 异常（界面红色）
 *
 * 多字节类型为大端序（高字节在前）。1 字 = 2 字节。
 */

namespace PlcProtocol {

// 一帧完整报文长度（2 车 + 末尾18字，无包头）
constexpr int FullPacketSize = 432;

// 无包头
constexpr int PacketHeaderSize = 0;

// 单车道数据块长度（字节）
constexpr int CarBlockSize = 198;

// 第二车在报文中的起始偏移
constexpr int SecondCarBlockOffset = CarBlockSize;

// 整帧末尾附加区：2 空字 + 16 状态字 = 18 字 = 36 字节
constexpr int BytesPerWord = 2;
constexpr int Offset_TailReserved = 396;   // 2 个空字起始
constexpr int TailReservedWords = 2;
constexpr int Offset_SlotStatus = 400;     // 16 个状态字起始（跳过 2 空字）
constexpr int SlotStatusPerCar = 8;
constexpr int SlotStatusTotalWords = 16;   // 左车8 + 右车8
constexpr int TailExtraBytes = (TailReservedWords + SlotStatusTotalWords) * BytesPerWord; // 36

static_assert(FullPacketSize == CarBlockSize * 2 + TailExtraBytes, "packet size mismatch");
static_assert(Offset_SlotStatus == Offset_TailReserved + TailReservedWords * BytesPerWord, "status offset");

// 单车块内偏移
constexpr int Offset_ProductionMode = 0;   // Int16 正常生产/补充生产
constexpr int Offset_VehicleType     = 2;   // Int16 车型
constexpr int Offset_AssemblyDone   = 4;   // Int16 装件完成标志
constexpr int Offset_Weight1        = 6;   // Float 工件1重量
constexpr int Offset_Barcode1       = 10;  // 工件1二维码（20 字节）

// 每组工件：4 重量 + 20 二维码
constexpr int BytesPerWorkpiece = 24;

// 二维码字段长度（协议中 20 字节，无头格式）
constexpr int BarcodeFieldSize = 20;

// 二维码有效字节数（20 字节字段全部作为有效 ASCII）
constexpr int BarcodeEffectiveBytes = 20;

// 从单车块内取工件重量/二维码的绝对偏移
inline int absoluteWeightOffset(int workpieceIndex) {
    if (workpieceIndex < 0 || workpieceIndex >= 8) return -1;
    return Offset_Weight1 + workpieceIndex * BytesPerWorkpiece;
}
inline int absoluteBarcodeOffset(int workpieceIndex) {
    if (workpieceIndex < 0 || workpieceIndex >= 8) return -1;
    return Offset_Barcode1 + workpieceIndex * BytesPerWorkpiece;
}
// 整帧中槽位状态字偏移（字节）：carIndex 1=左车 2=右车，workpieceIndex 0~7
inline int absolutePacketSlotStatusOffset(int carIndex, int workpieceIndex) {
    if (carIndex < 1 || carIndex > 2 || workpieceIndex < 0 || workpieceIndex >= SlotStatusPerCar)
        return -1;
    return Offset_SlotStatus
           + ((carIndex - 1) * SlotStatusPerCar + workpieceIndex) * BytesPerWord;
}

// 装件完成标志
constexpr int AssemblyNotStarted = 0;
constexpr int AssemblyInProgress = 1;
constexpr int AssemblyDoneComplete = 2;

// 槽位状态
constexpr int SlotStatusNormal = 0;
constexpr int SlotStatusAlarm = 1;

// 生产模式
enum ProductionMode { NormalProduction = 1, SupplementProduction = 2 };

// 车型
enum VehicleType { Model12V = 1, Model16V = 2 };

// 解析结果：单车道一帧数据
struct FirstCarData {
    int productionMode = 0;   // 1=正常生产 2=补充生产
    int vehicleType    = 0;   // 1=12V 2=16V
    int assemblyDone   = 0;   // 0=未完成 1=装件中 2=装件完成
    QList<double> weights;    // 8 个重量
    QList<QString> barcodes;  // 8 个条码
    QList<int> slotStatuses;  // 8 个状态：0正常 1异常
};

// 解析结果：一帧 432 字节（2 车 + 末尾18字）
struct TwoCarPacket {
    FirstCarData car1;
    FirstCarData car2;
};

// 大端读 Int16
inline qint16 readInt16BE(const QByteArray &data, int offset) {
    if (data.size() < offset + 2) return 0;
    return (quint8(data[offset]) << 8) | quint8(data[offset + 1]);
}

// 大端读 Float（IEEE 754）
inline float readFloat32BE(const QByteArray &data, int offset) {
    if (data.size() < offset + 4) return 0.0f;
    quint32 u = (quint8(data[offset]) << 24) | (quint8(data[offset + 1]) << 16)
                | (quint8(data[offset + 2]) << 8) | quint8(data[offset + 3]);
    float f;
    memcpy(&f, &u, 4);
    return f;
}

/**
 * 读二维码字段：20 字节无头格式，有效 ASCII 最多 20 字节转字符串
 */
QString readBarcode22(const QByteArray &data, int offset);

// 解析单车道 198 字节数据块；若长度不足返回 false
bool parseFirstCarBlock(const QByteArray &block, FirstCarData &out);

// 解析一帧 432 字节（第一车 198 + 第二车 198 + 末尾 2空字 + 16状态字）；若长度不足返回 false
bool parseTwoCarPacket(const QByteArray &packet, TwoCarPacket &out);

/**
 * 构建生产补充指令包（432 字节）
 * 包头新含义：补充生产=1 检测生产=2；装件完成=补充数量
 * trayIndex: 1=第一托 2=第二托；选中托有数据，另一托全0
 * vehicleType: 1=12V 2=16V（对应绑定表 command）
 * supplementQty: 补充数量
 * weights/barcodes: 选中托的当前数据（8个），内部克(g)转kg
 */
QByteArray buildSupplementPacket(int trayIndex, int vehicleType, int supplementQty,
    const QList<double> &weights, const QList<QString> &barcodes);

/**
 * 构建第一托检测OK指令包（432 字节）
 * car1: productionMode=2(检测ok)，携带第一托物品数据；car2全0
 */
QByteArray buildDetectionOkPacketTray1(const QList<double> &weights1, const QList<QString> &barcodes1, int vehicleType1);

/**
 * 构建两托完成检测OK指令包（432 字节）
 * car1、car2均 productionMode=2(检测ok)，携带各自物品数据
 */
QByteArray buildDetectionOkPacketBoth(const QList<double> &weights1, const QList<QString> &barcodes1, int vehicleType1,
    const QList<double> &weights2, const QList<QString> &barcodes2, int vehicleType2);

} // namespace PlcProtocol

#endif // PLCPROTOCOL_H
