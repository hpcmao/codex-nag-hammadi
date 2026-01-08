#pragma once

#include <QVector>
#include <QString>
#include <QDateTime>
#include <optional>

namespace codex::db {

struct Passage {
    int id = -1;
    int projectId = -1;
    QString textContent;
    int startPosition = 0;
    int endPosition = 0;
    int orderIndex = 0;
    QDateTime createdAt;
};

class PassageRepository {
public:
    PassageRepository();

    int create(const Passage& passage);
    std::optional<Passage> findById(int id);
    QVector<Passage> findByProjectId(int projectId);
    bool update(const Passage& passage);
    bool remove(int id);
    bool removeByProjectId(int projectId);

    // Reorder passages in a project
    bool updateOrder(int passageId, int newOrderIndex);
    int getMaxOrderIndex(int projectId);
};

} // namespace codex::db
