#pragma once

#include <QString>
#include <QFile>
#include <QMutex>

namespace codex::utils {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& instance();

    void setLogLevel(LogLevel level);
    void setLogToFile(bool enabled);

    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const QString& message);
    QString levelToString(LogLevel level) const;
    QString logFilePath() const;

    LogLevel m_level = LogLevel::Info;
    bool m_logToFile = true;
    QFile m_logFile;
    QMutex m_mutex;
};

// Convenience macros
#define LOG_DEBUG(msg) codex::utils::Logger::instance().debug(msg)
#define LOG_INFO(msg) codex::utils::Logger::instance().info(msg)
#define LOG_WARN(msg) codex::utils::Logger::instance().warning(msg)
#define LOG_ERROR(msg) codex::utils::Logger::instance().error(msg)

} // namespace codex::utils
