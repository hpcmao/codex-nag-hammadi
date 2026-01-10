#include "ApiPricingDockWidget.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QStyle>

namespace codex::ui {

ApiPricingDockWidget::ApiPricingDockWidget(QWidget* parent)
    : QDockWidget("API & Tarifs", parent)
{
    setObjectName("ApiPricingDock");
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetMovable |
                QDockWidget::DockWidgetFloatable |
                QDockWidget::DockWidgetClosable);

    setupUi();
    populateModels();

    // Connect to pricing manager signals
    auto& manager = codex::utils::ApiPricingManager::instance();
    connect(&manager, &codex::utils::ApiPricingManager::priceChanged,
            this, &ApiPricingDockWidget::onPriceChanged);
    connect(&manager, &codex::utils::ApiPricingManager::pricesUpdated,
            this, &ApiPricingDockWidget::populateModels);

    // Notification timer
    m_notificationTimer = new QTimer(this);
    m_notificationTimer->setSingleShot(true);
}

void ApiPricingDockWidget::setupUi() {
    m_mainWidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(m_mainWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // === TEXT MODELS ===
    auto* textGroup = new QGroupBox("Texte (LLM)", m_mainWidget);
    auto* textLayout = new QGridLayout(textGroup);
    textLayout->setSpacing(5);

    m_textModelCombo = new QComboBox(textGroup);
    m_textModelCombo->setMinimumWidth(150);
    connect(m_textModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ApiPricingDockWidget::onTextModelChanged);

    m_textPriceLabel = new QLabel("$0.00/1M", textGroup);
    m_textPriceLabel->setStyleSheet("font-weight: bold; color: #4a9eff;");

    textLayout->addWidget(new QLabel("Modele:"), 0, 0);
    textLayout->addWidget(m_textModelCombo, 0, 1);
    textLayout->addWidget(m_textPriceLabel, 0, 2);

    mainLayout->addWidget(textGroup);

    // === IMAGE MODELS ===
    auto* imageGroup = new QGroupBox("Images (Imagen)", m_mainWidget);
    auto* imageLayout = new QGridLayout(imageGroup);
    imageLayout->setSpacing(5);

    m_imageModelCombo = new QComboBox(imageGroup);
    connect(m_imageModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ApiPricingDockWidget::onImageModelChanged);

    m_imagePriceLabel = new QLabel("$0.00/img", imageGroup);
    m_imagePriceLabel->setStyleSheet("font-weight: bold; color: #4aff4a;");

    imageLayout->addWidget(new QLabel("Modele:"), 0, 0);
    imageLayout->addWidget(m_imageModelCombo, 0, 1);
    imageLayout->addWidget(m_imagePriceLabel, 0, 2);

    mainLayout->addWidget(imageGroup);

    // === VIDEO MODELS ===
    auto* videoGroup = new QGroupBox("Videos (Veo)", m_mainWidget);
    auto* videoLayout = new QGridLayout(videoGroup);
    videoLayout->setSpacing(5);

    m_videoModelCombo = new QComboBox(videoGroup);
    connect(m_videoModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ApiPricingDockWidget::onVideoModelChanged);

    m_videoPriceLabel = new QLabel("$0.00/sec", videoGroup);
    m_videoPriceLabel->setStyleSheet("font-weight: bold; color: #ff4a4a;");

    videoLayout->addWidget(new QLabel("Modele:"), 0, 0);
    videoLayout->addWidget(m_videoModelCombo, 0, 1);
    videoLayout->addWidget(m_videoPriceLabel, 0, 2);

    mainLayout->addWidget(videoGroup);

    // === AUDIO MODELS ===
    auto* audioGroup = new QGroupBox("Audio (TTS)", m_mainWidget);
    auto* audioLayout = new QGridLayout(audioGroup);
    audioLayout->setSpacing(5);

    m_audioModelCombo = new QComboBox(audioGroup);

    m_audioPriceLabel = new QLabel("Gratuit", audioGroup);
    m_audioPriceLabel->setStyleSheet("font-weight: bold; color: #ffaa4a;");

    audioLayout->addWidget(new QLabel("Modele:"), 0, 0);
    audioLayout->addWidget(m_audioModelCombo, 0, 1);
    audioLayout->addWidget(m_audioPriceLabel, 0, 2);

    mainLayout->addWidget(audioGroup);

    // === COST ESTIMATION ===
    auto* costGroup = new QGroupBox("Estimation des couts", m_mainWidget);
    auto* costLayout = new QGridLayout(costGroup);
    costLayout->setSpacing(5);

    // Image estimation
    costLayout->addWidget(new QLabel("Images:"), 0, 0);
    m_imageCountSpin = new QSpinBox(costGroup);
    m_imageCountSpin->setRange(0, 100);
    m_imageCountSpin->setValue(4);
    m_imageCountSpin->setSuffix(" images");
    connect(m_imageCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ApiPricingDockWidget::onEstimationChanged);
    costLayout->addWidget(m_imageCountSpin, 0, 1);
    m_imageCostLabel = new QLabel("= $0.00", costGroup);
    m_imageCostLabel->setStyleSheet("color: #4aff4a;");
    costLayout->addWidget(m_imageCostLabel, 0, 2);

    // Video estimation
    costLayout->addWidget(new QLabel("Video:"), 1, 0);
    m_videoSecondsSpin = new QSpinBox(costGroup);
    m_videoSecondsSpin->setRange(0, 60);
    m_videoSecondsSpin->setValue(8);
    m_videoSecondsSpin->setSuffix(" sec");
    connect(m_videoSecondsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ApiPricingDockWidget::onEstimationChanged);
    costLayout->addWidget(m_videoSecondsSpin, 1, 1);
    m_videoCostLabel = new QLabel("= $0.00", costGroup);
    m_videoCostLabel->setStyleSheet("color: #ff4a4a;");
    costLayout->addWidget(m_videoCostLabel, 1, 2);

    // Separator line
    auto* separator = new QFrame(costGroup);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: #555;");
    costLayout->addWidget(separator, 2, 0, 1, 3);

    // Total
    costLayout->addWidget(new QLabel("TOTAL:"), 3, 0);
    m_totalCostLabel = new QLabel("$0.00", costGroup);
    m_totalCostLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #fff;");
    costLayout->addWidget(m_totalCostLabel, 3, 2);

    mainLayout->addWidget(costGroup);

    // === BUTTONS ===
    auto* buttonLayout = new QHBoxLayout();

    m_refreshBtn = new QPushButton("Actualiser tarifs", m_mainWidget);
    m_refreshBtn->setToolTip("Verifier les tarifs en ligne");
    connect(m_refreshBtn, &QPushButton::clicked, this, &ApiPricingDockWidget::onRefreshPrices);
    buttonLayout->addWidget(m_refreshBtn);

    m_editPriceBtn = new QPushButton("Modifier prix", m_mainWidget);
    m_editPriceBtn->setToolTip("Modifier manuellement un prix");
    connect(m_editPriceBtn, &QPushButton::clicked, this, &ApiPricingDockWidget::onEditPrice);
    buttonLayout->addWidget(m_editPriceBtn);

    mainLayout->addLayout(buttonLayout);

    // Last check timestamp
    m_lastCheckLabel = new QLabel("Derniere verif: --", m_mainWidget);
    m_lastCheckLabel->setStyleSheet("color: #888; font-size: 10px;");
    m_lastCheckLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_lastCheckLabel);

    mainLayout->addStretch();

    setWidget(m_mainWidget);
}

void ApiPricingDockWidget::populateModels() {
    auto& manager = codex::utils::ApiPricingManager::instance();

    // Block signals during population
    m_textModelCombo->blockSignals(true);
    m_imageModelCombo->blockSignals(true);
    m_videoModelCombo->blockSignals(true);
    m_audioModelCombo->blockSignals(true);

    // Clear combos
    m_textModelCombo->clear();
    m_imageModelCombo->clear();
    m_videoModelCombo->clear();
    m_audioModelCombo->clear();

    // Populate text models
    auto textModels = manager.modelsByCategory(codex::utils::ApiCategory::Text);
    for (const auto& m : textModels) {
        m_textModelCombo->addItem(QString("%1 (%2)").arg(m.name, m.priceString()), m.id);
        if (m.isActive) {
            m_textModelCombo->setCurrentIndex(m_textModelCombo->count() - 1);
        }
    }

    // Populate image models
    auto imageModels = manager.modelsByCategory(codex::utils::ApiCategory::Image);
    for (const auto& m : imageModels) {
        m_imageModelCombo->addItem(QString("%1 (%2)").arg(m.name, m.priceString()), m.id);
        if (m.isActive) {
            m_imageModelCombo->setCurrentIndex(m_imageModelCombo->count() - 1);
        }
    }

    // Populate video models
    auto videoModels = manager.modelsByCategory(codex::utils::ApiCategory::Video);
    for (const auto& m : videoModels) {
        QString cost8s = QString("~$%1/8s").arg(m.price * 8, 0, 'f', 2);
        m_videoModelCombo->addItem(QString("%1 (%2)").arg(m.name, cost8s), m.id);
        if (m.isActive) {
            m_videoModelCombo->setCurrentIndex(m_videoModelCombo->count() - 1);
        }
    }

    // Populate audio models
    auto audioModels = manager.modelsByCategory(codex::utils::ApiCategory::Audio);
    for (const auto& m : audioModels) {
        QString priceStr = (m.price == 0) ? "Gratuit" : m.priceString();
        m_audioModelCombo->addItem(QString("%1 (%2)").arg(m.name, priceStr), m.id);
        if (m.isActive) {
            m_audioModelCombo->setCurrentIndex(m_audioModelCombo->count() - 1);
        }
    }

    // Unblock signals
    m_textModelCombo->blockSignals(false);
    m_imageModelCombo->blockSignals(false);
    m_videoModelCombo->blockSignals(false);
    m_audioModelCombo->blockSignals(false);

    updateCostDisplay();
}

void ApiPricingDockWidget::updateCostDisplay() {
    auto& manager = codex::utils::ApiPricingManager::instance();

    // Get active models
    auto textModel = manager.activeModel(codex::utils::ApiCategory::Text);
    auto imageModel = manager.activeModel(codex::utils::ApiCategory::Image);
    auto videoModel = manager.activeModel(codex::utils::ApiCategory::Video);
    auto audioModel = manager.activeModel(codex::utils::ApiCategory::Audio);

    // Update price labels
    m_textPriceLabel->setText(textModel.priceString());
    m_imagePriceLabel->setText(imageModel.priceString());
    m_videoPriceLabel->setText(videoModel.priceString());
    m_audioPriceLabel->setText(audioModel.price == 0 ? "Gratuit" : audioModel.priceString());

    // Calculate costs
    int imageCount = m_imageCountSpin->value();
    int videoSeconds = m_videoSecondsSpin->value();

    double imageCost = imageModel.estimateCost(imageCount);
    double videoCost = videoModel.estimateCost(videoSeconds);
    double totalCost = imageCost + videoCost;

    m_imageCostLabel->setText(QString("= $%1").arg(imageCost, 0, 'f', 2));
    m_videoCostLabel->setText(QString("= $%1").arg(videoCost, 0, 'f', 2));
    m_totalCostLabel->setText(QString("$%1").arg(totalCost, 0, 'f', 2));

    // Color total based on cost
    if (totalCost < 1.0) {
        m_totalCostLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #4aff4a;");
    } else if (totalCost < 5.0) {
        m_totalCostLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #ffaa4a;");
    } else {
        m_totalCostLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #ff4a4a;");
    }

    // Update last check
    m_lastCheckLabel->setText(QString("Derniere verif: %1")
        .arg(QDateTime::currentDateTime().toString("dd/MM hh:mm")));
}

void ApiPricingDockWidget::onTextModelChanged(int index) {
    if (index < 0) return;
    QString modelId = m_textModelCombo->itemData(index).toString();
    codex::utils::ApiPricingManager::instance().setActiveModel(
        codex::utils::ApiCategory::Text, modelId);
    updateCostDisplay();
    emit modelSelectionChanged();
}

void ApiPricingDockWidget::onImageModelChanged(int index) {
    if (index < 0) return;
    QString modelId = m_imageModelCombo->itemData(index).toString();
    codex::utils::ApiPricingManager::instance().setActiveModel(
        codex::utils::ApiCategory::Image, modelId);
    updateCostDisplay();
    emit modelSelectionChanged();
}

void ApiPricingDockWidget::onVideoModelChanged(int index) {
    if (index < 0) return;
    QString modelId = m_videoModelCombo->itemData(index).toString();
    codex::utils::ApiPricingManager::instance().setActiveModel(
        codex::utils::ApiCategory::Video, modelId);
    updateCostDisplay();
    emit modelSelectionChanged();
}

void ApiPricingDockWidget::onRefreshPrices() {
    codex::utils::ApiPricingManager::instance().checkPricesOnline();
    QMessageBox::information(this, "Tarifs",
        "Les tarifs ont ete mis a jour.\n\n"
        "Note: La verification automatique en ligne sera implementee prochainement.");
}

void ApiPricingDockWidget::onPriceChanged(const QString& modelId, double oldPrice, double newPrice) {
    auto model = codex::utils::ApiPricingManager::instance().model(modelId);
    showPriceChangeNotification(model.name, oldPrice, newPrice);
    populateModels();
}

void ApiPricingDockWidget::onEditPrice() {
    QStringList items;
    auto& manager = codex::utils::ApiPricingManager::instance();
    auto models = manager.allModels();

    for (const auto& m : models) {
        items << QString("%1 - %2").arg(m.name, m.priceString());
    }

    bool ok;
    QString selected = QInputDialog::getItem(this, "Modifier un prix",
        "Selectionnez le modele:", items, 0, false, &ok);

    if (!ok || selected.isEmpty()) return;

    int idx = items.indexOf(selected);
    if (idx < 0 || idx >= models.size()) return;

    auto model = models[idx];
    double newPrice = QInputDialog::getDouble(this, "Nouveau prix",
        QString("Prix pour %1 (en $):").arg(model.name),
        model.price, 0, 100, 3, &ok);

    if (ok) {
        manager.updatePrice(model.id, newPrice);
    }
}

void ApiPricingDockWidget::onEstimationChanged() {
    updateCostDisplay();
}

void ApiPricingDockWidget::showPriceChangeNotification(const QString& modelName,
                                                        double oldPrice, double newPrice) {
    QString direction = (newPrice < oldPrice) ? "BAISSE" : "HAUSSE";
    QString color = (newPrice < oldPrice) ? "#4aff4a" : "#ff4a4a";

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Changement de tarif!");
    msgBox.setText(QString("<b style='color:%1'>%2 DE PRIX</b>").arg(color, direction));
    msgBox.setInformativeText(QString("%1\n\nAncien: $%2\nNouveau: $%3")
        .arg(modelName)
        .arg(oldPrice, 0, 'f', 2)
        .arg(newPrice, 0, 'f', 2));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

QString ApiPricingDockWidget::selectedImageModel() const {
    return m_imageModelCombo->currentData().toString();
}

QString ApiPricingDockWidget::selectedVideoModel() const {
    return m_videoModelCombo->currentData().toString();
}

QString ApiPricingDockWidget::selectedTextModel() const {
    return m_textModelCombo->currentData().toString();
}

void ApiPricingDockWidget::updateEstimation(int imageCount, int videoSeconds, int textTokens) {
    Q_UNUSED(textTokens);
    m_imageCountSpin->setValue(imageCount);
    m_videoSecondsSpin->setValue(videoSeconds);
    updateCostDisplay();
}

} // namespace codex::ui
