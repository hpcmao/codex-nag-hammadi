#include "ProjectRepository.h"
#include "../Database.h"
#include "utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

namespace codex::db {

ProjectRepository::ProjectRepository() {
}

int ProjectRepository::create(const Project& project) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        INSERT INTO projects (name, treatise_code, category)
        VALUES (:name, :treatise_code, :category)
    )");
    query.bindValue(":name", project.name);
    query.bindValue(":treatise_code", project.treatiseCode);
    query.bindValue(":category", project.category);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to create project: %1").arg(query.lastError().text()));
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<Project> ProjectRepository::findById(int id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM projects WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Project project;
    project.id = query.value("id").toInt();
    project.name = query.value("name").toString();
    project.treatiseCode = query.value("treatise_code").toString();
    project.category = query.value("category").toString();
    project.createdAt = query.value("created_at").toDateTime();
    project.updatedAt = query.value("updated_at").toDateTime();

    return project;
}

QVector<Project> ProjectRepository::findAll() {
    QVector<Project> projects;
    QSqlQuery query(Database::instance().connection());

    if (!query.exec("SELECT * FROM projects ORDER BY updated_at DESC")) {
        LOG_ERROR(QString("Failed to fetch projects: %1").arg(query.lastError().text()));
        return projects;
    }

    while (query.next()) {
        Project project;
        project.id = query.value("id").toInt();
        project.name = query.value("name").toString();
        project.treatiseCode = query.value("treatise_code").toString();
        project.category = query.value("category").toString();
        project.createdAt = query.value("created_at").toDateTime();
        project.updatedAt = query.value("updated_at").toDateTime();
        projects.append(project);
    }

    return projects;
}

bool ProjectRepository::update(const Project& project) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        UPDATE projects SET
            name = :name,
            treatise_code = :treatise_code,
            category = :category,
            updated_at = CURRENT_TIMESTAMP
        WHERE id = :id
    )");
    query.bindValue(":id", project.id);
    query.bindValue(":name", project.name);
    query.bindValue(":treatise_code", project.treatiseCode);
    query.bindValue(":category", project.category);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to update project: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool ProjectRepository::remove(int id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("DELETE FROM projects WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to delete project: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

QVector<Project> ProjectRepository::findRecent(int limit) {
    QVector<Project> projects;
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM projects ORDER BY updated_at DESC LIMIT :limit");
    query.bindValue(":limit", limit);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to fetch recent projects: %1").arg(query.lastError().text()));
        return projects;
    }

    while (query.next()) {
        Project project;
        project.id = query.value("id").toInt();
        project.name = query.value("name").toString();
        project.treatiseCode = query.value("treatise_code").toString();
        project.category = query.value("category").toString();
        project.createdAt = query.value("created_at").toDateTime();
        project.updatedAt = query.value("updated_at").toDateTime();
        projects.append(project);
    }

    return projects;
}

std::optional<Project> ProjectRepository::findByTreatiseCode(const QString& code) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM projects WHERE treatise_code = :code ORDER BY updated_at DESC LIMIT 1");
    query.bindValue(":code", code);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Project project;
    project.id = query.value("id").toInt();
    project.name = query.value("name").toString();
    project.treatiseCode = query.value("treatise_code").toString();
    project.category = query.value("category").toString();
    project.createdAt = query.value("created_at").toDateTime();
    project.updatedAt = query.value("updated_at").toDateTime();

    return project;
}

} // namespace codex::db
