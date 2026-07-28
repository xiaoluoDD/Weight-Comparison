#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QtGlobal>
#include "weightdata.h"

/**
 * SQLite 数据库管理：数据库文件放在 exe 同级目录
 * 默认文件名：weight_comparison.db
 */
class DatabaseManager
{
public:
    static DatabaseManager &instance();

    // 初始化并打开数据库（exe 同级目录）
    bool init();

    // 插入称重记录，tableIndex 为 1 或 2
    bool insertWeightRecord(const WeightData &data, int tableIndex);

    // 加载指定表格的历史记录
    QList<WeightData> loadWeightRecords(int tableIndex);

    // 清空指定表格的数据库记录
    bool clearTableRecords(int tableIndex);

    // NG品记录
    qint64 insertNgRecord(const QString &vehicleModel, const QString &barcode, double weightG);  // 返回插入的id，失败返回-1，weightG为克
    bool deleteNgRecord(qint64 id);
    QList<QList<QVariant>> loadNgRecords();  // 每行: [id, vehicleModel, barcode, weight, created_at]

    // 车型绑定
    bool insertBinding(const QString &command, const QString &itemName);  // 插入或覆盖
    bool deleteBinding(const QString &command);
    QMap<QString, QString> loadBindings();  // command -> itemName

    // 获取数据库文件路径
    QString databasePath() const;

    // 系统设置（key-value）
    void setSetting(const QString &key, const QVariant &value);
    QVariant getSetting(const QString &key, const QVariant &defaultValue = QVariant()) const;

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    bool createTables();
    QString m_dbPath;
};

#endif // DATABASE_H
