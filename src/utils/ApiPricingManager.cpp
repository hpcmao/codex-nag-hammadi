#include "ApiPricingManager.h"
#include "Logger.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>

namespace codex::utils {

ApiPricingManager& ApiPricingManager::instance() {
    static ApiPricingManager instance;
    return instance;
}

ApiPricingManager::ApiPricingManager() {
    initializeDefaultModels();
    load();
}

void ApiPricingManager::initializeDefaultModels() {
    m_models.clear();

    // === TEXT MODELS ===
    m_models.append({
        "gemini-2.5-flash",
        "Gemini 2.5 Flash",
        ApiCategory::Text,
        0.30,
        "1M tokens",
        "Rapide et economique, 1M context",
        false,
        QDateTime::currentDateTime(),
        0
    });

    m_models.append({
        "gemini-2.5-flash-lite",
        "Gemini 2.5 Flash-Lite",
        ApiCategory::Text,
        0.10,
        "1M tokens",
        "Le moins cher, ideal pour taches simples",
        false,
        QDateTime::currentDateTime(),
        0
    });

    m_models.append({
        "gemini-2.5-pro",
        "Gemini 2.5 Pro",
        ApiCategory::Text,
        1.25,
        "1M tokens",
        "Puissant pour raisonnement complexe",
        false,
        QDateTime::currentDateTime(),
        0
    });

    m_models.append({
        "gemini-3-pro-preview",
        "Gemini 3 Pro",
        ApiCategory::Text,
        2.00,
        "1M tokens",
        "Dernier modele, capacites multimodales",
        true,  // Default active
        QDateTime::currentDateTime(),
        0
    });

    // === IMAGE MODELS ===
    m_models.append({
        "imagen-3.0-generate-002",
        "Imagen 3",
        ApiCategory::Image,
        0.03,
        "image",
        "Generation d'images haute qualite",
        true,  // Default active
        QDateTime::currentDateTime(),
        0
    });

    m_models.append({
        "imagen-4.0-generate-preview-fast",
        "Imagen 4 Fast",
        ApiCategory::Image,
        0.02,
        "image",
        "Rapide, bonne qualite",
        false,
        QDateTime::currentDateTime(),
        0
    });

    m_models.append({
        "imagen-4.0-generate-preview",
        "Imagen 4 Standard",
        ApiCategory::Image,
        0.04,
        "image",
        "Meilleure qualite",
        false,
        QDateTime::currentDateTime(),
        0
    });

    m_models.append({
        "imagen-4.0-generate-preview-ultra",
        "Imagen 4 Ultra",
        ApiCategory::Image,
        0.06,
        "image",
        "Qualite maximale",
        false,
        QDateTime::currentDateTime(),
        0
    });

    // === VIDEO MODELS ===
    m_models.append({
        "veo-3.1-fast-generate-preview",
        "Veo 3.1 Fast",
        ApiCategory::Video,
        0.15,
        "seconde",
        "Rapide, ~$1.20 pour 8s",
        false,
        QDateTime::currentDateTime(),
        0
    });

    m_models.append({
        "veo-3.1-generate-preview",
        "Veo 3.1 Standard",
        ApiCategory::Video,
        0.40,
        "seconde",
        "Haute qualite, ~$3.20 pour 8s",
        true,  // Default active
        QDateTime::currentDateTime(),
        0
    });

    m_models.append({
        "veo-2.0-generate-001",
        "Veo 2",
        ApiCategory::Video,
        0.35,
        "seconde",
        "Generation precedente, ~$2.80 pour 8s",
        false,
        QDateTime::currentDateTime(),
        0
    });

    // === AUDIO MODELS ===
    m_models.append({
        "edge-tts",
        "Edge TTS (Microsoft)",
        ApiCategory::Audio,
        0.00,
        "caractere",
        "Gratuit, voix naturelles",
        true,  // Default active
        QDateTime::currentDateTime(),
        0
    });

    m_models.append({
        "gemini-flash-tts",
        "Gemini Flash TTS",
        ApiCategory::Audio,
        10.00,
        "1M tokens",
        "TTS via Gemini API",
        false,
        QDateTime::currentDateTime(),
        0
    });

    // Set default active models
    m_activeModels[ApiCategory::Text] = "gemini-3-pro-preview";
    m_activeModels[ApiCategory::Image] = "imagen-3.0-generate-002";
    m_activeModels[ApiCategory::Video] = "veo-3.1-generate-preview";
    m_activeModels[ApiCategory::Audio] = "edge-tts";
}

QVector<ApiModel> ApiPricingManager::modelsByCategory(ApiCategory category) const {
    QVector<ApiModel> result;
    for (const auto& model : m_models) {
        if (model.category == category) {
            result.append(model);
        }
    }
    return result;
}

ApiModel ApiPricingManager::activeModel(ApiCategory category) const {
    QString activeId = m_activeModels.value(category);
    for (const auto& m : m_models) {
        if (m.id == activeId) {
            return m;
        }
    }
    // Return first model of category if not found
    for (const auto& m : m_models) {
        if (m.category == category) {
            return m;
        }
    }
    return ApiModel();
}

void ApiPricingManager::setActiveModel(ApiCategory category, const QString& modelId) {
    // Update isActive flags
    for (auto& m : m_models) {
        if (m.category == category) {
            m.isActive = (m.id == modelId);
        }
    }

    m_activeModels[category] = modelId;
    emit modelActivated(category, modelId);
    save();

    LOG_INFO(QString("Active model changed: %1 -> %2")
             .arg(categoryName(category), modelId));
}

ApiModel ApiPricingManager::model(const QString& modelId) const {
    for (const auto& m : m_models) {
        if (m.id == modelId) {
            return m;
        }
    }
    return ApiModel();
}

void ApiPricingManager::updatePrice(const QString& modelId, double newPrice) {
    for (auto& m : m_models) {
        if (m.id == modelId) {
            if (qAbs(m.price - newPrice) > 0.001) {
                double oldPrice = m.price;
                m.previousPrice = oldPrice;
                m.price = newPrice;
                m.lastPriceCheck = QDateTime::currentDateTime();
                emit priceChanged(modelId, oldPrice, newPrice);
                LOG_INFO(QString("Price changed for %1: $%2 -> $%3")
                         .arg(modelId)
                         .arg(oldPrice, 0, 'f', 2)
                         .arg(newPrice, 0, 'f', 2));
            }
            break;
        }
    }
    save();
}

void ApiPricingManager::checkPricesOnline() {
    // TODO: Implement web scraping or API call to check current prices
    // For now, just update the timestamp
    for (auto& m : m_models) {
        m.lastPriceCheck = QDateTime::currentDateTime();
    }
    emit pricesUpdated();
    LOG_INFO("Price check completed (manual update only for now)");
}

QVector<ApiModel> ApiPricingManager::getRecentPriceChanges() const {
    QVector<ApiModel> changes;
    for (const auto& m : m_models) {
        if (m.previousPrice > 0 && qAbs(m.price - m.previousPrice) > 0.001) {
            changes.append(m);
        }
    }
    return changes;
}

void ApiPricingManager::clearPriceChangeFlags() {
    for (auto& m : m_models) {
        m.previousPrice = 0;
    }
}

QString ApiPricingManager::categoryName(ApiCategory category) {
    switch (category) {
        case ApiCategory::Text: return "Texte";
        case ApiCategory::Image: return "Images";
        case ApiCategory::Video: return "Videos";
        case ApiCategory::Audio: return "Audio";
    }
    return "Inconnu";
}

QString ApiPricingManager::categoryIcon(ApiCategory category) {
    switch (category) {
        case ApiCategory::Text: return "T";
        case ApiCategory::Image: return "I";
        case ApiCategory::Video: return "V";
        case ApiCategory::Audio: return "A";
    }
    return "?";
}

QString ApiPricingManager::configFilePath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return configDir + "/api_pricing.json";
}

void ApiPricingManager::load() {
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_INFO("No pricing config found, using defaults");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();

    // Load active models
    QJsonObject activeObj = root["activeModels"].toObject();
    if (activeObj.contains("text")) {
        m_activeModels[ApiCategory::Text] = activeObj["text"].toString();
    }
    if (activeObj.contains("image")) {
        m_activeModels[ApiCategory::Image] = activeObj["image"].toString();
    }
    if (activeObj.contains("video")) {
        m_activeModels[ApiCategory::Video] = activeObj["video"].toString();
    }
    if (activeObj.contains("audio")) {
        m_activeModels[ApiCategory::Audio] = activeObj["audio"].toString();
    }

    // Load custom prices
    QJsonObject pricesObj = root["customPrices"].toObject();
    for (auto& m : m_models) {
        if (pricesObj.contains(m.id)) {
            m.price = pricesObj[m.id].toDouble();
        }
        // Update isActive flag
        m.isActive = (m_activeModels.value(m.category) == m.id);
    }

    LOG_INFO("Pricing config loaded");
}

void ApiPricingManager::save() {
    QJsonObject root;

    // Save active models
    QJsonObject activeObj;
    activeObj["text"] = m_activeModels.value(ApiCategory::Text);
    activeObj["image"] = m_activeModels.value(ApiCategory::Image);
    activeObj["video"] = m_activeModels.value(ApiCategory::Video);
    activeObj["audio"] = m_activeModels.value(ApiCategory::Audio);
    root["activeModels"] = activeObj;

    // Save custom prices (only if different from defaults)
    QJsonObject pricesObj;
    for (const auto& m : m_models) {
        pricesObj[m.id] = m.price;
    }
    root["customPrices"] = pricesObj;

    QFile file(configFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        LOG_DEBUG("Pricing config saved");
    }
}

} // namespace codex::utils
