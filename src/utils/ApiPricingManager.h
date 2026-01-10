#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QJsonObject>
#include <QMap>

namespace codex::utils {

// Categories of API models
enum class ApiCategory {
    Text,
    Image,
    Video,
    Audio
};

// Structure representing an API model with pricing
struct ApiModel {
    QString id;              // e.g., "veo-3.1-generate-preview"
    QString name;            // e.g., "Veo 3.1 Standard"
    ApiCategory category;
    double price;            // Price per unit
    QString priceUnit;       // "seconde", "image", "1M tokens"
    QString description;     // Brief description
    bool isActive = false;   // Currently selected model
    QDateTime lastPriceCheck;
    double previousPrice = 0; // For price change detection

    // Helper to format price display
    QString priceString() const {
        if (priceUnit == "seconde") {
            return QString("$%1/sec").arg(price, 0, 'f', 2);
        } else if (priceUnit == "image") {
            return QString("$%1/img").arg(price, 0, 'f', 2);
        } else {
            return QString("$%1/%2").arg(price, 0, 'f', 2).arg(priceUnit);
        }
    }

    // Estimate cost for a given quantity
    double estimateCost(double quantity) const {
        return price * quantity;
    }
};

class ApiPricingManager : public QObject {
    Q_OBJECT

public:
    static ApiPricingManager& instance();

    // Get all models
    QVector<ApiModel> allModels() const { return m_models; }
    QVector<ApiModel> modelsByCategory(ApiCategory category) const;

    // Get/Set active model for each category
    ApiModel activeModel(ApiCategory category) const;
    void setActiveModel(ApiCategory category, const QString& modelId);

    // Get model by ID
    ApiModel model(const QString& modelId) const;

    // Update prices (manual or from web)
    void updatePrice(const QString& modelId, double newPrice);
    void checkPricesOnline();

    // Persistence
    void load();
    void save();

    // Price change detection
    QVector<ApiModel> getRecentPriceChanges() const;
    void clearPriceChangeFlags();

    // Category helpers
    static QString categoryName(ApiCategory category);
    static QString categoryIcon(ApiCategory category);

signals:
    void priceChanged(const QString& modelId, double oldPrice, double newPrice);
    void modelActivated(ApiCategory category, const QString& modelId);
    void pricesUpdated();

private:
    ApiPricingManager();
    void initializeDefaultModels();
    QString configFilePath() const;

    QVector<ApiModel> m_models;
    QMap<ApiCategory, QString> m_activeModels;  // category -> modelId
};

} // namespace codex::utils
