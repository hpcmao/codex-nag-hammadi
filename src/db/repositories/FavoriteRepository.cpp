#include "FavoriteRepository.h"
#include "../Database.h"
#include "utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

namespace codex::db {

FavoriteRepository::FavoriteRepository() {
}

QString FavoriteRepository::typeToString(FavoriteType type) const {
    return type == FavoriteType::Heart ? "heart" : "star";
}

FavoriteType FavoriteRepository::stringToType(const QString& str) const {
    return str == "heart" ? FavoriteType::Heart : FavoriteType::Star;
}

int FavoriteRepository::addFavorite(const Favorite& favorite) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        INSERT OR REPLACE INTO favorites
        (treatise_code, passage_excerpt, start_position, end_position, favorite_type)
        VALUES (:treatise_code, :passage_excerpt, :start_position, :end_position, :favorite_type)
    )");
    query.bindValue(":treatise_code", favorite.treatiseCode);
    query.bindValue(":passage_excerpt", favorite.passageExcerpt.left(200));
    query.bindValue(":start_position", favorite.startPosition);
    query.bindValue(":end_position", favorite.endPosition);
    query.bindValue(":favorite_type", typeToString(favorite.type));

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to add favorite: %1").arg(query.lastError().text()));
        return -1;
    }

    LOG_INFO(QString("Favorite added: %1, type: %2")
             .arg(favorite.treatiseCode, typeToString(favorite.type)));
    return query.lastInsertId().toInt();
}

bool FavoriteRepository::removeFavorite(int id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("DELETE FROM favorites WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to remove favorite: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool FavoriteRepository::removeFavorite(const QString& treatiseCode, int startPos, int endPos) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        DELETE FROM favorites
        WHERE treatise_code = :treatise_code
        AND start_position = :start_position
        AND end_position = :end_position
    )");
    query.bindValue(":treatise_code", treatiseCode);
    query.bindValue(":start_position", startPos);
    query.bindValue(":end_position", endPos);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to remove favorite: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

std::optional<Favorite> FavoriteRepository::findById(int id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM favorites WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Favorite fav;
    fav.id = query.value("id").toInt();
    fav.treatiseCode = query.value("treatise_code").toString();
    fav.passageExcerpt = query.value("passage_excerpt").toString();
    fav.startPosition = query.value("start_position").toInt();
    fav.endPosition = query.value("end_position").toInt();
    fav.type = stringToType(query.value("favorite_type").toString());
    fav.createdAt = query.value("created_at").toDateTime();

    return fav;
}

QVector<Favorite> FavoriteRepository::findByTreatise(const QString& treatiseCode) {
    QVector<Favorite> favorites;
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM favorites WHERE treatise_code = :treatise_code ORDER BY start_position");
    query.bindValue(":treatise_code", treatiseCode);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to fetch favorites: %1").arg(query.lastError().text()));
        return favorites;
    }

    while (query.next()) {
        Favorite fav;
        fav.id = query.value("id").toInt();
        fav.treatiseCode = query.value("treatise_code").toString();
        fav.passageExcerpt = query.value("passage_excerpt").toString();
        fav.startPosition = query.value("start_position").toInt();
        fav.endPosition = query.value("end_position").toInt();
        fav.type = stringToType(query.value("favorite_type").toString());
        fav.createdAt = query.value("created_at").toDateTime();
        favorites.append(fav);
    }

    return favorites;
}

QVector<Favorite> FavoriteRepository::findByType(FavoriteType type) {
    QVector<Favorite> favorites;
    QSqlQuery query(Database::instance().connection());
    query.prepare("SELECT * FROM favorites WHERE favorite_type = :type ORDER BY created_at DESC");
    query.bindValue(":type", typeToString(type));

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to fetch favorites by type: %1").arg(query.lastError().text()));
        return favorites;
    }

    while (query.next()) {
        Favorite fav;
        fav.id = query.value("id").toInt();
        fav.treatiseCode = query.value("treatise_code").toString();
        fav.passageExcerpt = query.value("passage_excerpt").toString();
        fav.startPosition = query.value("start_position").toInt();
        fav.endPosition = query.value("end_position").toInt();
        fav.type = stringToType(query.value("favorite_type").toString());
        fav.createdAt = query.value("created_at").toDateTime();
        favorites.append(fav);
    }

    return favorites;
}

QVector<Favorite> FavoriteRepository::findAll() {
    QVector<Favorite> favorites;
    QSqlQuery query(Database::instance().connection());

    if (!query.exec("SELECT * FROM favorites ORDER BY created_at DESC")) {
        LOG_ERROR(QString("Failed to fetch all favorites: %1").arg(query.lastError().text()));
        return favorites;
    }

    while (query.next()) {
        Favorite fav;
        fav.id = query.value("id").toInt();
        fav.treatiseCode = query.value("treatise_code").toString();
        fav.passageExcerpt = query.value("passage_excerpt").toString();
        fav.startPosition = query.value("start_position").toInt();
        fav.endPosition = query.value("end_position").toInt();
        fav.type = stringToType(query.value("favorite_type").toString());
        fav.createdAt = query.value("created_at").toDateTime();
        favorites.append(fav);
    }

    return favorites;
}

bool FavoriteRepository::isFavorite(const QString& treatiseCode, int startPos, int endPos) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        SELECT COUNT(*) FROM favorites
        WHERE treatise_code = :treatise_code
        AND start_position = :start_position
        AND end_position = :end_position
    )");
    query.bindValue(":treatise_code", treatiseCode);
    query.bindValue(":start_position", startPos);
    query.bindValue(":end_position", endPos);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

std::optional<Favorite> FavoriteRepository::getFavorite(const QString& treatiseCode, int startPos, int endPos) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(R"(
        SELECT * FROM favorites
        WHERE treatise_code = :treatise_code
        AND start_position = :start_position
        AND end_position = :end_position
    )");
    query.bindValue(":treatise_code", treatiseCode);
    query.bindValue(":start_position", startPos);
    query.bindValue(":end_position", endPos);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Favorite fav;
    fav.id = query.value("id").toInt();
    fav.treatiseCode = query.value("treatise_code").toString();
    fav.passageExcerpt = query.value("passage_excerpt").toString();
    fav.startPosition = query.value("start_position").toInt();
    fav.endPosition = query.value("end_position").toInt();
    fav.type = stringToType(query.value("favorite_type").toString());
    fav.createdAt = query.value("created_at").toDateTime();

    return fav;
}

bool FavoriteRepository::toggleFavorite(const QString& treatiseCode, const QString& excerpt,
                                         int startPos, int endPos, FavoriteType type) {
    auto existing = getFavorite(treatiseCode, startPos, endPos);

    if (existing.has_value()) {
        // If same type, remove it (toggle off)
        if (existing->type == type) {
            return removeFavorite(existing->id);
        }
        // If different type, update it
        return updateType(existing->id, type);
    }

    // Add new favorite
    Favorite fav;
    fav.treatiseCode = treatiseCode;
    fav.passageExcerpt = excerpt;
    fav.startPosition = startPos;
    fav.endPosition = endPos;
    fav.type = type;

    return addFavorite(fav) > 0;
}

bool FavoriteRepository::updateType(int id, FavoriteType type) {
    QSqlQuery query(Database::instance().connection());
    query.prepare("UPDATE favorites SET favorite_type = :type WHERE id = :id");
    query.bindValue(":id", id);
    query.bindValue(":type", typeToString(type));

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to update favorite type: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

QStringList FavoriteRepository::getTreatisesWithFavorites() {
    QStringList treatises;
    QSqlQuery query(Database::instance().connection());

    if (!query.exec("SELECT DISTINCT treatise_code FROM favorites ORDER BY treatise_code")) {
        LOG_ERROR(QString("Failed to fetch treatises with favorites: %1").arg(query.lastError().text()));
        return treatises;
    }

    while (query.next()) {
        treatises.append(query.value(0).toString());
    }

    return treatises;
}

} // namespace codex::db
