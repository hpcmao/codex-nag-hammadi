#include "PassageRepository.h"
#include "../Database.h"
#include "utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

namespace codex::db {

PassageRepository::PassageRepository() {
}

int PassageRepository::create(const Passage& passage) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        INSERT INTO passages (project_id, text_content, start_position, end_position, order_index)
        VALUES (:project_id, :text_content, :start_position, :end_position, :order_index)
    )");
    query.bindValue(":project_id", passage.projectId);
    query.bindValue(":text_content", passage.textContent);
    query.bindValue(":start_position", passage.startPosition);
    query.bindValue(":end_position", passage.endPosition);
    query.bindValue(":order_index", passage.orderIndex);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to create passage: %1").arg(query.lastError().text()));
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<Passage> PassageRepository::findById(int id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM passages WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Passage passage;
    passage.id = query.value("id").toInt();
    passage.projectId = query.value("project_id").toInt();
    passage.textContent = query.value("text_content").toString();
    passage.startPosition = query.value("start_position").toInt();
    passage.endPosition = query.value("end_position").toInt();
    passage.orderIndex = query.value("order_index").toInt();
    passage.createdAt = query.value("created_at").toDateTime();

    return passage;
}

QVector<Passage> PassageRepository::findByProjectId(int projectId) {
    QVector<Passage> passages;
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM passages WHERE project_id = :project_id ORDER BY order_index");
    query.bindValue(":project_id", projectId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to fetch passages: %1").arg(query.lastError().text()));
        return passages;
    }

    while (query.next()) {
        Passage passage;
        passage.id = query.value("id").toInt();
        passage.projectId = query.value("project_id").toInt();
        passage.textContent = query.value("text_content").toString();
        passage.startPosition = query.value("start_position").toInt();
        passage.endPosition = query.value("end_position").toInt();
        passage.orderIndex = query.value("order_index").toInt();
        passage.createdAt = query.value("created_at").toDateTime();
        passages.append(passage);
    }

    return passages;
}

bool PassageRepository::update(const Passage& passage) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        UPDATE passages SET
            text_content = :text_content,
            start_position = :start_position,
            end_position = :end_position,
            order_index = :order_index
        WHERE id = :id
    )");
    query.bindValue(":id", passage.id);
    query.bindValue(":text_content", passage.textContent);
    query.bindValue(":start_position", passage.startPosition);
    query.bindValue(":end_position", passage.endPosition);
    query.bindValue(":order_index", passage.orderIndex);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to update passage: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool PassageRepository::remove(int id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("DELETE FROM passages WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to delete passage: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool PassageRepository::removeByProjectId(int projectId) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("DELETE FROM passages WHERE project_id = :project_id");
    query.bindValue(":project_id", projectId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to delete passages: %1").arg(query.lastError().text()));
        return false;
    }

    return true;
}

bool PassageRepository::updateOrder(int passageId, int newOrderIndex) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("UPDATE passages SET order_index = :order_index WHERE id = :id");
    query.bindValue(":id", passageId);
    query.bindValue(":order_index", newOrderIndex);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to update passage order: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

int PassageRepository::getMaxOrderIndex(int projectId) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT MAX(order_index) FROM passages WHERE project_id = :project_id");
    query.bindValue(":project_id", projectId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return -1;
}

} // namespace codex::db
