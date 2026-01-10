#pragma once

#include <QDockWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>
#include <QTimer>

#include "utils/ApiPricingManager.h"

namespace codex::ui {

class ApiPricingDockWidget : public QDockWidget {
    Q_OBJECT

public:
    explicit ApiPricingDockWidget(QWidget* parent = nullptr);

    // Get currently selected models
    QString selectedImageModel() const;
    QString selectedVideoModel() const;
    QString selectedTextModel() const;

    // Cost estimation
    void updateEstimation(int imageCount, int videoSeconds, int textTokens);

signals:
    void modelSelectionChanged();

private slots:
    void onImageModelChanged(int index);
    void onVideoModelChanged(int index);
    void onTextModelChanged(int index);
    void onRefreshPrices();
    void onPriceChanged(const QString& modelId, double oldPrice, double newPrice);
    void onEditPrice();
    void onEstimationChanged();

private:
    void setupUi();
    void populateModels();
    void updateCostDisplay();
    void showPriceChangeNotification(const QString& modelName, double oldPrice, double newPrice);

    // UI Elements
    QWidget* m_mainWidget = nullptr;

    // Model selectors
    QComboBox* m_textModelCombo = nullptr;
    QComboBox* m_imageModelCombo = nullptr;
    QComboBox* m_videoModelCombo = nullptr;
    QComboBox* m_audioModelCombo = nullptr;

    // Price labels
    QLabel* m_textPriceLabel = nullptr;
    QLabel* m_imagePriceLabel = nullptr;
    QLabel* m_videoPriceLabel = nullptr;
    QLabel* m_audioPriceLabel = nullptr;

    // Estimation inputs
    QSpinBox* m_imageCountSpin = nullptr;
    QSpinBox* m_videoSecondsSpin = nullptr;
    QSpinBox* m_textTokensSpin = nullptr;

    // Cost estimation display
    QLabel* m_imageCostLabel = nullptr;
    QLabel* m_videoCostLabel = nullptr;
    QLabel* m_textCostLabel = nullptr;
    QLabel* m_totalCostLabel = nullptr;

    // Buttons
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_editPriceBtn = nullptr;

    // Last check label
    QLabel* m_lastCheckLabel = nullptr;

    // Price change notification
    QTimer* m_notificationTimer = nullptr;
};

} // namespace codex::ui
