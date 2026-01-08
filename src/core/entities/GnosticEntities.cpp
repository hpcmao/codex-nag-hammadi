#include "GnosticEntities.h"
#include "utils/Logger.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace codex::core {

GnosticEntities& GnosticEntities::instance() {
    static GnosticEntities instance;
    return instance;
}

GnosticEntities::GnosticEntities() {
    initializeDefaultEntities();
}

void GnosticEntities::initializeDefaultEntities() {
    // Plerome - Divine fullness
    m_entities["Plerome"] = {
        "Plerome",
        "Plenitude divine, totalite des emanations",
        {"plerome", "plenitude", "totalite", "pleroma"},
        {"infinite light", "cosmic void", "golden radiance", "divine fullness"},
        {"#FFD700", "#FFFFFF", "#4169E1"}
    };

    // Eons - Divine emanations
    m_entities["Eons"] = {
        "Eons",
        "Emanations divines, entites celestes",
        {"eon", "eons", "aion", "aiones", "aeon"},
        {"luminous beings", "ethereal figures", "light forms", "celestial entities"},
        {"#E6E6FA", "#FFD700", "#87CEEB"}
    };

    // Sophia - Wisdom
    m_entities["Sophia"] = {
        "Sophia",
        "Sagesse divine, figure feminine tragique",
        {"sophia", "sagesse", "pistis", "pistis sophia", "achamoth"},
        {"weeping figure", "falling light", "blue tears", "tragic beauty"},
        {"#191970", "#C0C0C0", "#4682B4"}
    };

    // Archontes - Rulers
    m_entities["Archontes"] = {
        "Archontes",
        "Dirigeants du monde materiel, forces hostiles",
        {"archonte", "archontes", "dirigeant", "puissance", "gouverneur"},
        {"dark rulers", "shadowy figures", "oppressive presence", "cosmic tyrants"},
        {"#8B0000", "#000000", "#2F4F4F"}
    };

    // Demiurge - Creator god
    m_entities["Demiurge"] = {
        "Demiurge",
        "Createur du monde materiel, Yaldabaoth",
        {"demiurge", "yaldabaoth", "saklas", "samael", "createur", "ialdabaoth"},
        {"lion-headed serpent", "fire and darkness", "throne of ignorance", "blind god"},
        {"#B22222", "#000000", "#CD853F"}
    };

    // Christ - Savior
    m_entities["Christ"] = {
        "Christ",
        "Figure de redemption, porteur de gnose",
        {"christ", "sauveur", "jesus", "seigneur", "messie", "oint"},
        {"radiant figure", "descending light", "gentle illumination", "divine messenger"},
        {"#FFFAF0", "#FFD700", "#F0E68C"}
    };

    // Pere - Father
    m_entities["Pere"] = {
        "Pere",
        "Le Dieu inconnaissable, source de tout",
        {"pere", "ineffable", "inconnaissable", "premier", "invisible", "abime"},
        {"invisible presence", "absolute light", "transcendent", "infinite depth"},
        {"#FFFFFF", "#FFFFF0", "#F5F5F5"}
    };

    // Esprit - Spirit
    m_entities["Esprit"] = {
        "Esprit",
        "L'Esprit Saint, souffle divin",
        {"esprit", "souffle", "pneuma", "paraclet"},
        {"breath of light", "flowing energy", "invisible wind", "divine breath"},
        {"#E0FFFF", "#B0E0E6", "#ADD8E6"}
    };

    // Lumiere - Light
    m_entities["Lumiere"] = {
        "Lumiere",
        "Symbole du divin, de la connaissance",
        {"lumiere", "lumineux", "clarte", "brillant", "resplendissant", "eclat"},
        {"radiant beams", "golden glow", "illumination", "divine light"},
        {"#FFD700", "#FFFF00", "#FFFACD"}
    };

    // Tenebres - Darkness
    m_entities["Tenebres"] = {
        "Tenebres",
        "Ignorance, monde materiel, mal",
        {"tenebres", "obscurite", "noir", "ombre", "nuit"},
        {"deep shadows", "void", "darkness", "abyssal depths"},
        {"#000000", "#1C1C1C", "#363636"}
    };

    // Gnose - Knowledge
    m_entities["Gnose"] = {
        "Gnose",
        "Connaissance salvifique, revelation",
        {"gnose", "connaissance", "savoir", "revelation"},
        {"secret wisdom", "hidden truth", "enlightenment", "inner vision"},
        {"#DAA520", "#F5DEB3", "#FFD700"}
    };

    // Adam - First man
    m_entities["Adam"] = {
        "Adam",
        "Premier homme, archetype humain",
        {"adam", "premier homme", "anthropos"},
        {"primordial man", "clay figure", "awakening consciousness", "divine spark"},
        {"#8B4513", "#DEB887", "#D2B48C"}
    };

    // Eve - First woman
    m_entities["Eve"] = {
        "Eve",
        "Premiere femme, porteuse de vie",
        {"eve", "femme", "zoe", "vie"},
        {"mother figure", "giver of life", "feminine wisdom", "serpent companion"},
        {"#228B22", "#90EE90", "#98FB98"}
    };

    // Seth - Third son
    m_entities["Seth"] = {
        "Seth",
        "Fils d'Adam, ancetre des gnostiques",
        {"seth", "race de seth", "sethien"},
        {"chosen seed", "spiritual lineage", "pure descendant", "guardian figure"},
        {"#4682B4", "#5F9EA0", "#6495ED"}
    };

    // Barbelo - First feminine eon
    m_entities["Barbelo"] = {
        "Barbelo",
        "Premier eon feminin, pensee du Pere",
        {"barbelo", "premiere pensee", "protennoia", "ennoia"},
        {"divine feminine", "first thought", "mirror of light", "mother of all"},
        {"#DDA0DD", "#EE82EE", "#DA70D6"}
    };

    // Autogene - Self-born
    m_entities["Autogene"] = {
        "Autogene",
        "Le fils auto-engendre, Christ celeste",
        {"autogene", "auto-engendre", "monogene"},
        {"self-born light", "divine emanation", "perfect son", "eternal child"},
        {"#FAFAD2", "#FFE4B5", "#FFEFD5"}
    };

    // Norea - Noah's wife
    m_entities["Norea"] = {
        "Norea",
        "Femme de Noe, figure salvatrice",
        {"norea", "vierge"},
        {"righteous woman", "fire resistant", "divine protection", "spiritual purity"},
        {"#FF6347", "#FF7F50", "#FFA07A"}
    };

    // Sabaoth - Repentant archon
    m_entities["Sabaoth"] = {
        "Sabaoth",
        "Archonte repenti, fils du Demiurge",
        {"sabaoth", "repenti", "converti"},
        {"converted ruler", "throne of glory", "celestial warrior", "redeemed power"},
        {"#4169E1", "#6A5ACD", "#7B68EE"}
    };

    // Pronoia - Providence
    m_entities["Pronoia"] = {
        "Pronoia",
        "Providence divine, pensee anticipatrice",
        {"pronoia", "providence", "prescience"},
        {"all-seeing eye", "divine plan", "foreknowledge", "guiding light"},
        {"#9370DB", "#BA55D3", "#9932CC"}
    };

    // Logos - Word
    m_entities["Logos"] = {
        "Logos",
        "Parole divine, principe createur",
        {"logos", "verbe", "parole"},
        {"spoken light", "creative word", "divine reason", "cosmic order"},
        {"#F0E68C", "#EEE8AA", "#BDB76B"}
    };

    // Pneumatique - Spiritual being
    m_entities["Pneumatique"] = {
        "Pneumatique",
        "Etre spirituel, porteur de l'etincelle",
        {"pneumatique", "spirituel", "elu", "semence"},
        {"spiritual being", "inner spark", "chosen soul", "light carrier"},
        {"#87CEEB", "#87CEFA", "#00BFFF"}
    };

    // Psychique - Soul being
    m_entities["Psychique"] = {
        "Psychique",
        "Etre d'ame, entre matiere et esprit",
        {"psychique", "animique", "ame"},
        {"soul being", "middle nature", "potential vessel"},
        {"#DDA0DD", "#D8BFD8", "#E6E6FA"}
    };

    // Hylique - Material being
    m_entities["Hylique"] = {
        "Hylique",
        "Etre materiel, prisonnier de la chair",
        {"hylique", "materiel", "charnel", "terrestre"},
        {"material being", "earthbound", "dense form"},
        {"#8B4513", "#A0522D", "#6B4423"}
    };

    // Ogdoade - Eighth sphere
    m_entities["Ogdoade"] = {
        "Ogdoade",
        "Huitieme sphere, region celeste",
        {"ogdoade", "huitieme"},
        {"eighth heaven", "starry realm", "cosmic boundary", "celestial sphere"},
        {"#000080", "#191970", "#00008B"}
    };

    LOG_INFO(QString("Initialized %1 gnostic entities").arg(m_entities.size()));
}

bool GnosticEntities::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARN(QString("Could not open entities file: %1").arg(filePath));
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonObject entities = root["entities"].toObject();

    int loadedCount = 0;
    for (auto it = entities.begin(); it != entities.end(); ++it) {
        QJsonObject entityObj = it.value().toObject();
        GnosticEntity entity;
        entity.name = it.key();
        entity.description = entityObj["description"].toString();

        for (const auto& kw : entityObj["keywords"].toArray()) {
            entity.keywords.append(kw.toString());
        }
        for (const auto& vk : entityObj["visual_keywords"].toArray()) {
            entity.visualKeywords.append(vk.toString());
        }
        for (const auto& c : entityObj["palette"].toArray()) {
            entity.palette.append(c.toString());
        }

        m_entities[entity.name] = entity;
        loadedCount++;
    }

    LOG_INFO(QString("Loaded %1 entities from file: %2").arg(loadedCount).arg(filePath));
    return true;
}

QStringList GnosticEntities::detect(const QString& text) const {
    QStringList detected;
    QString lowerText = text.toLower();

    for (auto it = m_entities.constBegin(); it != m_entities.constEnd(); ++it) {
        for (const QString& keyword : it->keywords) {
            if (lowerText.contains(keyword.toLower())) {
                if (!detected.contains(it.key())) {
                    detected.append(it.key());
                }
                break;
            }
        }
    }

    return detected;
}

GnosticEntity GnosticEntities::getEntity(const QString& name) const {
    return m_entities.value(name);
}

QStringList GnosticEntities::getAllEntityNames() const {
    return m_entities.keys();
}

} // namespace codex::core
