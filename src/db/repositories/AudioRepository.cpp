#include "AudioRepository.h"
#include "../Database.h"
#include "utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

namespace codex::db {

AudioRepository::AudioRepository() {
}

int AudioRepository::create(const AudioFile& audio) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        INSERT INTO audio_files (passage_id, file_path, duration_ms, voice_id)
        VALUES (:passage_id, :file_path, :duration_ms, :voice_id)
    )");
    query.bindValue(":passage_id", audio.passageId);
    query.bindValue(":file_path", audio.filePath);
    query.bindValue(":duration_ms", audio.durationMs);
    query.bindValue(":voice_id", audio.voiceId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to create audio file: %1").arg(query.lastError().text()));
        return -1;
    }

    return query.lastInsertId().toInt();
}

std::optional<AudioFile> AudioRepository::findById(int id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM audio_files WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    AudioFile audio;
    audio.id = query.value("id").toInt();
    audio.passageId = query.value("passage_id").toInt();
    audio.filePath = query.value("file_path").toString();
    audio.durationMs = query.value("duration_ms").toInt();
    audio.voiceId = query.value("voice_id").toString();
    audio.createdAt = query.value("created_at").toDateTime();

    return audio;
}

QVector<AudioFile> AudioRepository::findByPassageId(int passageId) {
    QVector<AudioFile> audioFiles;
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM audio_files WHERE passage_id = :passage_id ORDER BY created_at DESC");
    query.bindValue(":passage_id", passageId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to fetch audio files: %1").arg(query.lastError().text()));
        return audioFiles;
    }

    while (query.next()) {
        AudioFile audio;
        audio.id = query.value("id").toInt();
        audio.passageId = query.value("passage_id").toInt();
        audio.filePath = query.value("file_path").toString();
        audio.durationMs = query.value("duration_ms").toInt();
        audio.voiceId = query.value("voice_id").toString();
        audio.createdAt = query.value("created_at").toDateTime();
        audioFiles.append(audio);
    }

    return audioFiles;
}

std::optional<AudioFile> AudioRepository::findLatestByPassageId(int passageId) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM audio_files WHERE passage_id = :passage_id ORDER BY created_at DESC LIMIT 1");
    query.bindValue(":passage_id", passageId);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    AudioFile audio;
    audio.id = query.value("id").toInt();
    audio.passageId = query.value("passage_id").toInt();
    audio.filePath = query.value("file_path").toString();
    audio.durationMs = query.value("duration_ms").toInt();
    audio.voiceId = query.value("voice_id").toString();
    audio.createdAt = query.value("created_at").toDateTime();

    return audio;
}

bool AudioRepository::update(const AudioFile& audio) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        UPDATE audio_files SET
            file_path = :file_path,
            duration_ms = :duration_ms,
            voice_id = :voice_id
        WHERE id = :id
    )");
    query.bindValue(":id", audio.id);
    query.bindValue(":file_path", audio.filePath);
    query.bindValue(":duration_ms", audio.durationMs);
    query.bindValue(":voice_id", audio.voiceId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to update audio file: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool AudioRepository::remove(int id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("DELETE FROM audio_files WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to delete audio file: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool AudioRepository::removeByPassageId(int passageId) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("DELETE FROM audio_files WHERE passage_id = :passage_id");
    query.bindValue(":passage_id", passageId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to delete audio files: %1").arg(query.lastError().text()));
        return false;
    }

    return true;
}

QVector<AudioFile> AudioRepository::findByProjectId(int projectId) {
    QVector<AudioFile> audioFiles;
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        SELECT a.* FROM audio_files a
        JOIN passages p ON a.passage_id = p.id
        WHERE p.project_id = :project_id
        ORDER BY p.order_index, a.created_at DESC
    )");
    query.bindValue(":project_id", projectId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to fetch audio files by project: %1").arg(query.lastError().text()));
        return audioFiles;
    }

    while (query.next()) {
        AudioFile audio;
        audio.id = query.value("id").toInt();
        audio.passageId = query.value("passage_id").toInt();
        audio.filePath = query.value("file_path").toString();
        audio.durationMs = query.value("duration_ms").toInt();
        audio.voiceId = query.value("voice_id").toString();
        audio.createdAt = query.value("created_at").toDateTime();
        audioFiles.append(audio);
    }

    return audioFiles;
}

int AudioRepository::getTotalDuration(int projectId) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        SELECT SUM(a.duration_ms) FROM audio_files a
        JOIN passages p ON a.passage_id = p.id
        WHERE p.project_id = :project_id
    )");
    query.bindValue(":project_id", projectId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

} // namespace codex::db
