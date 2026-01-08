#pragma once

#include <QString>
#include <QMap>
#include <QStringList>
#include <QVector>

namespace codex::core {

struct GnosticEntity {
    QString name;
    QString description;
    QStringList keywords;
    QStringList visualKeywords;
    QVector<QString> palette;
};

class GnosticEntities {
public:
    static GnosticEntities& instance();

    bool loadFromFile(const QString& filePath);

    QStringList detect(const QString& text) const;

    GnosticEntity getEntity(const QString& name) const;
    QStringList getAllEntityNames() const;

private:
    GnosticEntities();
    void initializeDefaultEntities();

    QMap<QString, GnosticEntity> m_entities;
};

} // namespace codex::core
