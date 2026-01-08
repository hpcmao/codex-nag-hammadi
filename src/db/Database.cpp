#include "Database.h"
#include "utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>

namespace codex::db {

Database& Database::instance() {
    static Database instance;
    return instance;
}

QString Database::defaultDbPath() const {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    return appData + "/codex.db";
}

bool Database::initialize(const QString& dbPath) {
    if (m_initialized) {
        return true;
    }

    m_dbPath = dbPath.isEmpty() ? defaultDbPath() : dbPath;

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        LOG_ERROR(QString("Failed to open database: %1").arg(m_db.lastError().text()));
        return false;
    }

    LOG_INFO(QString("Database opened: %1").arg(m_dbPath));

    if (!createTables()) {
        return false;
    }

    m_initialized = true;
    return true;
}

bool Database::isInitialized() const {
    return m_initialized;
}

QSqlDatabase& Database::connection() {
    return m_db;
}

bool Database::createTables() {
    QSqlQuery query(m_db);

    // Projects table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS projects (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            treatise_code TEXT,
            category TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        LOG_ERROR(QString("Failed to create projects table: %1").arg(query.lastError().text()));
        return false;
    }

    // Passages table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS passages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            project_id INTEGER REFERENCES projects(id) ON DELETE CASCADE,
            text_content TEXT NOT NULL,
            start_position INTEGER,
            end_position INTEGER,
            order_index INTEGER,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        LOG_ERROR(QString("Failed to create passages table: %1").arg(query.lastError().text()));
        return false;
    }

    // Images table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS images (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            passage_id INTEGER REFERENCES passages(id) ON DELETE CASCADE,
            prompt_used TEXT,
            file_path TEXT,
            generation_params TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        LOG_ERROR(QString("Failed to create images table: %1").arg(query.lastError().text()));
        return false;
    }

    // Audio files table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS audio_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            passage_id INTEGER REFERENCES passages(id) ON DELETE CASCADE,
            file_path TEXT,
            duration_ms INTEGER,
            voice_id TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        LOG_ERROR(QString("Failed to create audio_files table: %1").arg(query.lastError().text()));
        return false;
    }

    // Config table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS config (
            key TEXT PRIMARY KEY,
            value TEXT
        )
    )")) {
        LOG_ERROR(QString("Failed to create config table: %1").arg(query.lastError().text()));
        return false;
    }

    // Favorites table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS favorites (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            treatise_code TEXT NOT NULL,
            passage_excerpt TEXT,
            start_position INTEGER,
            end_position INTEGER,
            favorite_type TEXT NOT NULL DEFAULT 'star',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(treatise_code, start_position, end_position)
        )
    )")) {
        LOG_ERROR(QString("Failed to create favorites table: %1").arg(query.lastError().text()));
        return false;
    }

    // Schema version
    if (!query.exec(R"(
        INSERT OR IGNORE INTO config (key, value) VALUES ('schema_version', '2')
    )")) {
        LOG_ERROR(QString("Failed to set schema version: %1").arg(query.lastError().text()));
        return false;
    }

    LOG_INFO("Database tables created successfully");
    return true;
}

bool Database::runMigrations() {
    // For now, just ensure tables exist
    return createTables();
}

int Database::getCurrentVersion() {
    QSqlQuery query(m_db);
    query.exec("SELECT value FROM config WHERE key = 'schema_version'");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

} // namespace codex::db
