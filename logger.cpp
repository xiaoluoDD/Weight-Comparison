#include "logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStringConverter>
#include <QTextStream>
#include <QDateTime>

QString Logger::s_logDir;
QMutex Logger::s_mutex;

void Logger::init()
{
    QString exeDir = QCoreApplication::applicationDirPath();
    s_logDir = exeDir + QStringLiteral("/log");
    QDir dir;
    if (!dir.exists(s_logDir)) {
        dir.mkpath(s_logDir);
    }
}

QString Logger::logDir()
{
    return s_logDir;
}

QString Logger::currentLogFilePath()
{
    QString dateStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd"));
    return s_logDir + QLatin1Char('/') + dateStr + QStringLiteral(".log");
}

void Logger::log(const QString &level, const QString &message)
{
    writeToFile(level, message);
}

void Logger::info(const QString &message)
{
    writeToFile(QStringLiteral("INFO"), message);
}

void Logger::warning(const QString &message)
{
    writeToFile(QStringLiteral("WARN"), message);
}

void Logger::error(const QString &message)
{
    writeToFile(QStringLiteral("ERROR"), message);
}

void Logger::writeToFile(const QString &level, const QString &message)
{
    QMutexLocker locker(&s_mutex);
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    QString line = QStringLiteral("[%1] [%2] %3\n").arg(timeStr, level, message);

    QString path = currentLogFilePath();
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        out << line;
        file.close();
    }
}
