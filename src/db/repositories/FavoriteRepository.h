#pragma once

#include <QVector>
#include <QString>
#include <QDateTime>
#include <optional>

namespace codex::db {

enum class FavoriteType {
    Star,   // Etoile - marquage rapide
    Heart   // Coeur - passage important/favori
};

struct Favorite {
    int id = -1;
    QString treatiseCode;
    QString passageExcerpt;  // First ~100 chars of passage
    int startPosition = 0;
    int endPosition = 0;
    FavoriteType type = FavoriteType::Star;
    QDateTime createdAt;
};

class FavoriteRepository {
public:
    FavoriteRepository();

    // Create or update favorite
    int addFavorite(const Favorite& favorite);

    // Remove favorite
    bool removeFavorite(int id);
    bool removeFavorite(const QString& treatiseCode, int startPos, int endPos);

    // Find favorites
    std::optional<Favorite> findById(int id);
    QVector<Favorite> findByTreatise(const QString& treatiseCode);
    QVector<Favorite> findByType(FavoriteType type);
    QVector<Favorite> findAll();

    // Check if a passage is favorited
    bool isFavorite(const QString& treatiseCode, int startPos, int endPos);
    std::optional<Favorite> getFavorite(const QString& treatiseCode, int startPos, int endPos);

    // Toggle favorite (add if not exists, remove if exists)
    bool toggleFavorite(const QString& treatiseCode, const QString& excerpt,
                        int startPos, int endPos, FavoriteType type);

    // Update favorite type
    bool updateType(int id, FavoriteType type);

    // Get unique treatise codes that have favorites
    QStringList getTreatisesWithFavorites();

private:
    QString typeToString(FavoriteType type) const;
    FavoriteType stringToType(const QString& str) const;
};

} // namespace codex::db
