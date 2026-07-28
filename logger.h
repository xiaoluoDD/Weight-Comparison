#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QMutex>

/**
 * 日志系统：按日期保存到 exe 同级目录的 log 文件夹
 * 文件名格式：yyyy-MM-dd.log
 */
class Logger
{
public:
    // 初始化日志目录（exe 同级目录/log），应在程序启动时调用
    static void init();

    // 获取当前日志目录
    static QString logDir();

    // 写入日志（自动带时间戳）
    static void log(const QString &level, const QString &message);
    static void info(const QString &message);
    static void warning(const QString &message);
    static void error(const QString &message);

private:
    static QString s_logDir;
    static QMutex s_mutex;

    static QString currentLogFilePath();
    static void writeToFile(const QString &level, const QString &message);
};

#endif // LOGGER_H
