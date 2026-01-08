#include "ImageRepository.h"
#include "../Database.h"
#include "utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

namespace codex::db {

ImageRepository::ImageRepository() {
}

int ImageRepository::create(const GeneratedImage& image) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        INSERT INTO images (passage_id, prompt_used, file_path, generation_params)
        VALUES (:passage_id, :prompt_used, :file_path, :generation_params)
    )");
    query.bindValue(":passage_id", image.passageId);
    query.bindValue(":prompt_used", image.promptUsed);
    query.bindValue(":file_path", image.filePath);
    query.bindValue(":generation_params", image.generationParams);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to create image: %1").arg(query.lastError().text()));
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<GeneratedImage> ImageRepository::findById(int id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM images WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    GeneratedImage image;
    image.id = query.value("id").toInt();
    image.passageId = query.value("passage_id").toInt();
    image.promptUsed = query.value("prompt_used").toString();
    image.filePath = query.value("file_path").toString();
    image.generationParams = query.value("generation_params").toString();
    image.createdAt = query.value("created_at").toDateTime();

    return image;
}

QVector<GeneratedImage> ImageRepository::findByPassageId(int passageId) {
    QVector<GeneratedImage> images;
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM images WHERE passage_id = :passage_id ORDER BY created_at DESC");
    query.bindValue(":passage_id", passageId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to fetch images: %1").arg(query.lastError().text()));
        return images;
    }

    while (query.next()) {
        GeneratedImage image;
        image.id = query.value("id").toInt();
        image.passageId = query.value("passage_id").toInt();
        image.promptUsed = query.value("prompt_used").toString();
        image.filePath = query.value("file_path").toString();
        image.generationParams = query.value("generation_params").toString();
        image.createdAt = query.value("created_at").toDateTime();
        images.append(image);
    }

    return images;
}

std::optional<GeneratedImage> ImageRepository::findLatestByPassageId(int passageId) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM images WHERE passage_id = :passage_id ORDER BY created_at DESC LIMIT 1");
    query.bindValue(":passage_id", passageId);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    GeneratedImage image;
    image.id = query.value("id").toInt();
    image.passageId = query.value("passage_id").toInt();
    image.promptUsed = query.value("prompt_used").toString();
    image.filePath = query.value("file_path").toString();
    image.generationParams = query.value("generation_params").toString();
    image.createdAt = query.value("created_at").toDateTime();

    return image;
}

bool ImageRepository::update(const GeneratedImage& image) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        UPDATE images SET
            prompt_used = :prompt_used,
            file_path = :file_path,
            generation_params = :generation_params
        WHERE id = :id
    )");
    query.bindValue(":id", image.id);
    query.bindValue(":prompt_used", image.promptUsed);
    query.bindValue(":file_path", image.filePath);
    query.bindValue(":generation_params", image.generationParams);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to update image: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool ImageRepository::remove(int id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("DELETE FROM images WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to delete image: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool ImageRepository::removeByPassageId(int passageId) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("DELETE FROM images WHERE passage_id = :passage_id");
    query.bindValue(":passage_id", passageId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to delete images: %1").arg(query.lastError().text()));
        return false;
    }

    return true;
}

QVector<GeneratedImage> ImageRepository::findByProjectId(int projectId) {
    QVector<GeneratedImage> images;
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        SELECT i.* FROM images i
        JOIN passages p ON i.passage_id = p.id
        WHERE p.project_id = :project_id
        ORDER BY p.order_index, i.created_at DESC
    )");
    query.bindValue(":project_id", projectId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to fetch images by project: %1").arg(query.lastError().text()));
        return images;
    }

    while (query.next()) {
        GeneratedImage image;
        image.id = query.value("id").toInt();
        image.passageId = query.value("passage_id").toInt();
        image.promptUsed = query.value("prompt_used").toString();
        image.filePath = query.value("file_path").toString();
        image.generationParams = query.value("generation_params").toString();
        image.createdAt = query.value("created_at").toDateTime();
        images.append(image);
    }

    return images;
}

} // namespace codex::db
