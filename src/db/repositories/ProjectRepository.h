#pragma once

#include <QVector>
#include <QString>
#include <QDateTime>
#include <optional>

namespace codex::db {

struct Project {
    int id = -1;
    QString name;
    QString treatiseCode;
    QString category;
    QDateTime createdAt;
    QDateTime updatedAt;
};

class ProjectRepository {
public:
    ProjectRepository();

    int create(const Project& project);
    std::optional<Project> findById(int id);
    QVector<Project> findAll();
    bool update(const Project& project);
    bool remove(int id);

    QVector<Project> findRecent(int limit = 10);
    std::optional<Project> findByTreatiseCode(const QString& code);
};

} // namespace codex::db
