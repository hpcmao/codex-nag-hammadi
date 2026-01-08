#include "MythicClassifier.h"

namespace codex::core {

MythicClassifier::MythicClassifier() {
    initializeTreatiseMap();
    initializeStyleMap();
}

void MythicClassifier::initializeTreatiseMap() {
    // Plérôme
    m_treatiseMap["I-5"] = MythicCategory::Plerome;
    m_treatiseMap["III-3"] = MythicCategory::Plerome;
    m_treatiseMap["V-1"] = MythicCategory::Plerome;
    m_treatiseMap["XI-2"] = MythicCategory::Plerome;
    m_treatiseMap["X"] = MythicCategory::Plerome;
    m_treatiseMap["VIII-1"] = MythicCategory::Plerome;
    m_treatiseMap["XI-3"] = MythicCategory::Plerome;

    // Sophia
    m_treatiseMap["II-4"] = MythicCategory::Sophia;
    m_treatiseMap["II-5"] = MythicCategory::Sophia;
    m_treatiseMap["XIII-2"] = MythicCategory::Sophia;
    m_treatiseMap["VI-2"] = MythicCategory::Sophia;
    m_treatiseMap["II-6"] = MythicCategory::Sophia;
    m_treatiseMap["IX-2"] = MythicCategory::Sophia;
    m_treatiseMap["XIII-1"] = MythicCategory::Sophia;

    // Démiurge
    m_treatiseMap["II-1"] = MythicCategory::Demiurge;
    m_treatiseMap["III-1"] = MythicCategory::Demiurge;
    m_treatiseMap["IV-1"] = MythicCategory::Demiurge;
    m_treatiseMap["III-2"] = MythicCategory::Demiurge;
    m_treatiseMap["IV-2"] = MythicCategory::Demiurge;
    m_treatiseMap["VII-1"] = MythicCategory::Demiurge;
    m_treatiseMap["VI-4"] = MythicCategory::Demiurge;

    // Gnose
    m_treatiseMap["II-2"] = MythicCategory::Gnosis;
    m_treatiseMap["II-3"] = MythicCategory::Gnosis;
    m_treatiseMap["I-3"] = MythicCategory::Gnosis;
    m_treatiseMap["XII-2"] = MythicCategory::Gnosis;
    m_treatiseMap["III-4"] = MythicCategory::Gnosis;
    m_treatiseMap["III-5"] = MythicCategory::Gnosis;
    m_treatiseMap["I-2"] = MythicCategory::Gnosis;
    m_treatiseMap["II-7"] = MythicCategory::Gnosis;
    m_treatiseMap["VII-4"] = MythicCategory::Gnosis;
    m_treatiseMap["XII-1"] = MythicCategory::Gnosis;

    // Ascension
    m_treatiseMap["V-2"] = MythicCategory::Ascension;
    m_treatiseMap["V-3"] = MythicCategory::Ascension;
    m_treatiseMap["V-4"] = MythicCategory::Ascension;
    m_treatiseMap["V-5"] = MythicCategory::Ascension;
    m_treatiseMap["VII-3"] = MythicCategory::Ascension;
    m_treatiseMap["IX-1"] = MythicCategory::Ascension;
    m_treatiseMap["I-4"] = MythicCategory::Ascension;
    m_treatiseMap["VII-5"] = MythicCategory::Ascension;

    // Liturgie
    m_treatiseMap["I-1"] = MythicCategory::Liturgy;
    m_treatiseMap["VI-7"] = MythicCategory::Liturgy;
    m_treatiseMap["XI-2b"] = MythicCategory::Liturgy;

    // Hermétique
    m_treatiseMap["VI-6"] = MythicCategory::Hermetic;
    m_treatiseMap["VI-8"] = MythicCategory::Hermetic;
    m_treatiseMap["VI-5"] = MythicCategory::Hermetic;
    m_treatiseMap["VI-3"] = MythicCategory::Hermetic;
    m_treatiseMap["IX-3"] = MythicCategory::Hermetic;
    m_treatiseMap["XI-1"] = MythicCategory::Hermetic;

    // Narratif
    m_treatiseMap["VI-1"] = MythicCategory::Narrative;
    m_treatiseMap["VIII-2"] = MythicCategory::Narrative;
    m_treatiseMap["VII-2"] = MythicCategory::Narrative;

    // Fragments
    m_treatiseMap["XI-4"] = MythicCategory::Fragments;
    m_treatiseMap["XII-3"] = MythicCategory::Fragments;
}

void MythicClassifier::initializeStyleMap() {
    m_styleMap[MythicCategory::Plerome] = {
        MythicCategory::Plerome,
        "Plérôme",
        {"#FFD700", "#FFFFFF", "#4169E1"},
        {"infinite light", "cosmic void", "golden radiance", "ethereal"},
        "Diffuse golden light"
    };

    m_styleMap[MythicCategory::Sophia] = {
        MythicCategory::Sophia,
        "Sophia",
        {"#191970", "#C0C0C0", "#4682B4"},
        {"weeping figure", "falling light", "blue tears", "melancholy"},
        "Contre-jour, blue moonlight"
    };

    m_styleMap[MythicCategory::Demiurge] = {
        MythicCategory::Demiurge,
        "Démiurge",
        {"#8B0000", "#000000", "#2F4F4F"},
        {"dark ruler", "fire and shadow", "oppressive", "lion-headed"},
        "Harsh red firelight"
    };

    m_styleMap[MythicCategory::Gnosis] = {
        MythicCategory::Gnosis,
        "Gnose",
        {"#DAA520", "#F5DEB3", "#FFD700"},
        {"revelation", "secret teaching", "illumination", "intimate"},
        "Single ray piercing darkness"
    };

    m_styleMap[MythicCategory::Ascension] = {
        MythicCategory::Ascension,
        "Ascension",
        {"#FFD700", "#FFFAFA", "#E6E6FA"},
        {"ascending", "transparent", "celestial journey", "liberation"},
        "Ascending golden light"
    };

    m_styleMap[MythicCategory::Liturgy] = {
        MythicCategory::Liturgy,
        "Liturgie",
        {"#E6E6FA", "#FFD700", "#FFFAF0"},
        {"prayer", "invocation", "sacred", "devotion"},
        "Focused divine light"
    };

    m_styleMap[MythicCategory::Hermetic] = {
        MythicCategory::Hermetic,
        "Hermétique",
        {"#808080", "#D3D3D3", "#A9A9A9"},
        {"philosophical", "balanced", "intellectual", "neutral"},
        "Balanced natural light"
    };

    m_styleMap[MythicCategory::Narrative] = {
        MythicCategory::Narrative,
        "Narratif",
        {"#8B4513", "#DEB887", "#D2B48C"},
        {"journey", "apostolic", "earthly", "human scale"},
        "Natural daylight"
    };

    m_styleMap[MythicCategory::Fragments] = {
        MythicCategory::Fragments,
        "Fragments",
        {"#696969", "#A9A9A9", "#778899"},
        {"incomplete", "mysterious", "fragmentary"},
        "Dim, uncertain light"
    };
}

MythicCategory MythicClassifier::classifyTreatise(const QString& treatiseCode) {
    return m_treatiseMap.value(treatiseCode, MythicCategory::Gnosis);
}

QString MythicClassifier::categoryName(MythicCategory category) const {
    auto it = m_styleMap.find(category);
    if (it != m_styleMap.end()) {
        return it->name;
    }
    return "Inconnu";
}

CategoryStyle MythicClassifier::getStyleForCategory(MythicCategory category) {
    return m_styleMap.value(category, m_styleMap[MythicCategory::Gnosis]);
}

void MythicClassifier::setManualClassification(const QString& treatiseCode, MythicCategory category) {
    m_treatiseMap[treatiseCode] = category;
}

} // namespace codex::core
