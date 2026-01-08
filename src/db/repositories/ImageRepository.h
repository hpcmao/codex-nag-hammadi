#pragma once

#include <QVector>
#include <QString>
#include <QDateTime>
#include <optional>

namespace codex::db {

struct GeneratedImage {
    int id = -1;
    int passageId = -1;
    QString promptUsed;
    QString filePath;
    QString generationParams;  // JSON string with parameters
    QDateTime createdAt;
};

class ImageRepository {
public:
    ImageRepository();

    int create(const GeneratedImage& image);
    std::optional<GeneratedImage> findById(int id);
    QVector<GeneratedImage> findByPassageId(int passageId);
    std::optional<GeneratedImage> findLatestByPassageId(int passageId);
    bool update(const GeneratedImage& image);
    bool remove(int id);
    bool removeByPassageId(int passageId);

    // Get all images for a project (via passages)
    QVector<GeneratedImage> findByProjectId(int projectId);
};

} // namespace codex::db
