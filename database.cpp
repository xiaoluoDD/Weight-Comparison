#include "database.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include "logger.h"

DatabaseManager::DatabaseManager()
{
    QString exeDir = QCoreApplication::applicationDirPath();
    m_dbPath = exeDir + QStringLiteral("/weight_comparison.db");
}

DatabaseManager::~DatabaseManager()
{
    if (QSqlDatabase::contains(QStringLiteral("weight_connection"))) {
        QSqlDatabase::database(QStringLiteral("weight_connection")).close();
        QSqlDatabase::removeDatabase(QStringLiteral("weight_connection"));
    }
}

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::init()
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("weight_connection"));
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        Logger::error(QString("数据库打开失败: %1").arg(db.lastError().text()));
        return false;
    }
    Logger::info(QString("数据库已打开: %1").arg(m_dbPath));
    return createTables();
}

QString DatabaseManager::databasePath() const
{
    return m_dbPath;
}

bool DatabaseManager::createTables()
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    QSqlQuery query(db);

    QString sql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS weight_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "table_index INTEGER NOT NULL,"
        "vehicle_model TEXT,"
        "weight1 REAL, weight2 REAL, weight3 REAL, weight4 REAL,"
        "weight5 REAL, weight6 REAL, weight7 REAL, weight8 REAL,"
        "barcode1 TEXT, barcode2 TEXT, barcode3 TEXT, barcode4 TEXT,"
        "barcode5 TEXT, barcode6 TEXT, barcode7 TEXT, barcode8 TEXT,"
        "created_at TEXT NOT NULL"
        ")"
    );
    if (!query.exec(sql)) {
        Logger::error(QString("创建表失败: %1").arg(query.lastError().text()));
        return false;
    }

    // NG品记录表
    QString ngSql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS ng_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "vehicle_model TEXT,"
        "barcode TEXT,"
        "weight REAL,"
        "created_at TEXT NOT NULL"
        ")"
    );
    if (!query.exec(ngSql)) {
        Logger::error(QString("创建NG表失败: %1").arg(query.lastError().text()));
        return false;
    }

    // 车型绑定表
    QString bindSql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS vehicle_bindings ("
        "command TEXT PRIMARY KEY,"
        "item_name TEXT NOT NULL"
        ")"
    );
    if (!query.exec(bindSql)) {
        Logger::error(QString("创建车型绑定表失败: %1").arg(query.lastError().text()));
        return false;
    }

    // 系统设置表（key-value）
    QString settingsSql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS system_settings ("
        "key TEXT PRIMARY KEY,"
        "value TEXT"
        ")"
    );
    if (!query.exec(settingsSql)) {
        Logger::error(QString("创建系统设置表失败: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

void DatabaseManager::setSetting(const QString &key, const QVariant &value)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO system_settings (key, value) VALUES (?, ?)"));
    query.addBindValue(key);
    query.addBindValue(value.toString());
    if (!query.exec())
        Logger::error(QString("保存系统设置失败: %1").arg(query.lastError().text()));
}

QVariant DatabaseManager::getSetting(const QString &key, const QVariant &defaultValue) const
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) return defaultValue;

    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT value FROM system_settings WHERE key = ?"));
    query.addBindValue(key);
    if (!query.exec() || !query.next())
        return defaultValue;
    QString val = query.value(0).toString();
    if (val.isEmpty()) return defaultValue;
    return QVariant(val);
}

bool DatabaseManager::insertWeightRecord(const WeightData &data, int tableIndex)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO weight_records (table_index, vehicle_model,"
        "weight1, weight2, weight3, weight4, weight5, weight6, weight7, weight8,"
        "barcode1, barcode2, barcode3, barcode4, barcode5, barcode6, barcode7, barcode8,"
        "created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
    ));
    query.addBindValue(tableIndex);
    query.addBindValue(data.vehicleModel());

    QList<double> weights = data.weights();
    for (int i = 0; i < 8; ++i)
        query.addBindValue(i < weights.size() ? weights[i] : 0.0);

    QList<QString> barcodes = data.barcodes();
    for (int i = 0; i < 8; ++i)
        query.addBindValue(i < barcodes.size() ? barcodes[i] : QString());

    query.addBindValue(data.timestamp().toString(Qt::ISODate));

    if (!query.exec()) {
        Logger::error(QString("插入记录失败: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

QList<WeightData> DatabaseManager::loadWeightRecords(int tableIndex)
{
    QList<WeightData> list;
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) return list;

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT vehicle_model, weight1, weight2, weight3, weight4, weight5, weight6, weight7, weight8,"
        "barcode1, barcode2, barcode3, barcode4, barcode5, barcode6, barcode7, barcode8, created_at "
        "FROM weight_records WHERE table_index = ? ORDER BY id ASC"
    ));
    query.addBindValue(tableIndex);

    if (!query.exec()) {
        Logger::error(QString("加载记录失败: %1").arg(query.lastError().text()));
        return list;
    }

    while (query.next()) {
        WeightData w;
        w.setVehicleModel(query.value(0).toString());
        QList<double> weights;
        for (int i = 1; i <= 8; ++i)
            weights.append(query.value(i).toDouble());
        w.setWeights(weights);
        QList<QString> barcodes;
        for (int i = 9; i <= 16; ++i)
            barcodes.append(query.value(i).toString());
        w.setBarcodes(barcodes);
        w.setTimestamp(QDateTime::fromString(query.value(17).toString(), Qt::ISODate));
        list.append(w);
    }
    return list;
}

qint64 DatabaseManager::insertNgRecord(const QString &vehicleModel, const QString &barcode, double weightG)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) {
        Logger::error(QString("插入NG记录失败: 数据库未打开，请确认程序已正确初始化。路径: %1").arg(m_dbPath));
        return -1;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO ng_records (vehicle_model, barcode, weight, created_at) VALUES (?, ?, ?, ?)"
    ));
    query.addBindValue(vehicleModel);
    query.addBindValue(barcode);
    query.addBindValue(weightG);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        Logger::error(QString("插入NG记录失败: %1").arg(query.lastError().text()));
        return -1;
    }
    return query.lastInsertId().toLongLong();
}

bool DatabaseManager::deleteNgRecord(qint64 id)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM ng_records WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        Logger::error(QString("删除NG记录失败: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

QList<QList<QVariant>> DatabaseManager::loadNgRecords()
{
    QList<QList<QVariant>> list;
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) return list;

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
        "SELECT id, vehicle_model, barcode, weight, created_at FROM ng_records ORDER BY id ASC"
    ))) {
        Logger::error(QString("加载NG记录失败: %1").arg(query.lastError().text()));
        return list;
    }

    while (query.next()) {
        QList<QVariant> row;
        row << query.value(0) << query.value(1) << query.value(2) << query.value(3) << query.value(4);
        list.append(row);
    }
    return list;
}

bool DatabaseManager::insertBinding(const QString &command, const QString &itemName)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO vehicle_bindings (command, item_name) VALUES (?, ?)"
    ));
    query.addBindValue(command);
    query.addBindValue(itemName);

    if (!query.exec()) {
        Logger::error(QString("插入车型绑定失败: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool DatabaseManager::deleteBinding(const QString &command)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM vehicle_bindings WHERE command = ?"));
    query.addBindValue(command);
    if (!query.exec()) {
        Logger::error(QString("删除车型绑定失败: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

QMap<QString, QString> DatabaseManager::loadBindings()
{
    QMap<QString, QString> map;
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) return map;

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT command, item_name FROM vehicle_bindings ORDER BY command"))) {
        Logger::error(QString("加载车型绑定失败: %1").arg(query.lastError().text()));
        return map;
    }

    while (query.next()) {
        map.insert(query.value(0).toString(), query.value(1).toString());
    }
    return map;
}

bool DatabaseManager::clearTableRecords(int tableIndex)
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("weight_connection"));
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM weight_records WHERE table_index = ?"));
    query.addBindValue(tableIndex);
    if (!query.exec()) {
        Logger::error(QString("清空表记录失败: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}
