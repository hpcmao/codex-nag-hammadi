#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <QColor>

namespace codex::core {

enum class MythicCategory {
    Plerome,      // Lumière divine
    Sophia,       // Chute
    Demiurge,     // Création matérielle
    Gnosis,       // Révélation
    Ascension,    // Retour
    Liturgy,      // Prières
    Hermetic,     // Philosophie mixte
    Narrative,    // Récits
    Fragments     // Incomplets
};

struct CategoryStyle {
    MythicCategory category;
    QString name;
    QVector<QString> palette;
    QStringList visualKeywords;
    QString lightingStyle;
};

class MythicClassifier {
public:
    MythicClassifier();

    // Classifie un traité par son code
    MythicCategory classifyTreatise(const QString& treatiseCode);

    // Retourne le nom de la catégorie
    QString categoryName(MythicCategory category) const;

    // Retourne le style visuel pour une catégorie
    CategoryStyle getStyleForCategory(MythicCategory category);

    // Override manuel de la classification
    void setManualClassification(const QString& treatiseCode, MythicCategory category);

private:
    QMap<QString, MythicCategory> m_treatiseMap;
    QMap<MythicCategory, CategoryStyle> m_styleMap;

    void initializeTreatiseMap();
    void initializeStyleMap();
};

} // namespace codex::core
