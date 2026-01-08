#include "Logger.h"

#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <iostream>

namespace codex::utils {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    if (m_logToFile) {
        QString logPath = logFilePath();
        QDir().mkpath(QFileInfo(logPath).absolutePath());
        m_logFile.setFileName(logPath);
        m_logFile.open(QIODevice::WriteOnly | QIODevice::Append);
    }
}

Logger::~Logger() {
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

QString Logger::logFilePath() const {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData + "/logs/codex.log";
}

void Logger::setLogLevel(LogLevel level) {
    m_level = level;
}

void Logger::setLogToFile(bool enabled) {
    m_logToFile = enabled;
}

void Logger::debug(const QString& message) {
    log(LogLevel::Debug, message);
}

void Logger::info(const QString& message) {
    log(LogLevel::Info, message);
}

void Logger::warning(const QString& message) {
    log(LogLevel::Warning, message);
}

void Logger::error(const QString& message) {
    log(LogLevel::Error, message);
}

void Logger::log(LogLevel level, const QString& message) {
    if (level < m_level) {
        return;
    }

    QMutexLocker locker(&m_mutex);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString logLine = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);

    // Console output
    std::cout << logLine.toStdString() << std::endl;

    // File output
    if (m_logToFile && m_logFile.isOpen()) {
        QTextStream stream(&m_logFile);
        stream << logLine << "\n";
        stream.flush();
    }
}

QString Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        default:                return "?????";
    }
}

} // namespace codex::utils
