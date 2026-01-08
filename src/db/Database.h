#pragma once

#include <QSqlDatabase>
#include <QString>

namespace codex::db {

class Database {
public:
    static Database& instance();

    bool initialize(const QString& dbPath = QString());
    bool isInitialized() const;

    QSqlDatabase& connection();

    bool runMigrations();
    int getCurrentVersion();

private:
    Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QString defaultDbPath() const;
    bool createTables();

    QSqlDatabase m_db;
    bool m_initialized = false;
    QString m_dbPath;
};

} // namespace codex::db
