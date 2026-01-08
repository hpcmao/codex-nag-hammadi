#pragma once

#include <QVector>
#include <QString>
#include <QDateTime>
#include <optional>

namespace codex::db {

struct AudioFile {
    int id = -1;
    int passageId = -1;
    QString filePath;
    int durationMs = 0;
    QString voiceId;
    QDateTime createdAt;
};

class AudioRepository {
public:
    AudioRepository();

    int create(const AudioFile& audio);
    std::optional<AudioFile> findById(int id);
    QVector<AudioFile> findByPassageId(int passageId);
    std::optional<AudioFile> findLatestByPassageId(int passageId);
    bool update(const AudioFile& audio);
    bool remove(int id);
    bool removeByPassageId(int passageId);

    // Get all audio files for a project (via passages)
    QVector<AudioFile> findByProjectId(int projectId);

    // Get total duration for a project
    int getTotalDuration(int projectId);
};

} // namespace codex::db
