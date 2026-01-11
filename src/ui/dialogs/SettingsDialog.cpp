#include "SettingsDialog.h"
#include "utils/SecureStorage.h"
#include "utils/Config.h"
#include "utils/Logger.h"
#include "utils/MessageBox.h"
#include "utils/ThemeManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QColorDialog>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QProcess>

namespace codex::ui {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Paramètres");
    setMinimumSize(500, 400);
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);

    // ========== API Keys Tab ==========
    auto* apiScrollArea = new QScrollArea();
    apiScrollArea->setWidgetResizable(true);
    apiScrollArea->setFrameShape(QFrame::NoFrame);
    auto* apiTab = new QWidget();
    auto* apiLayout = new QVBoxLayout(apiTab);

    // LLM Provider selection
    auto* llmGroup = new QGroupBox("Fournisseur LLM (Analyse de texte)", apiTab);
    auto* llmMainLayout = new QVBoxLayout(llmGroup);

    auto* providerLayout = new QHBoxLayout();
    m_llmProviderCombo = new QComboBox(llmGroup);
    m_llmProviderCombo->addItem("Claude (Anthropic)", "claude");
    m_llmProviderCombo->addItem("Gemini (Google)", "gemini");
    providerLayout->addWidget(new QLabel("Fournisseur:"));
    providerLayout->addWidget(m_llmProviderCombo, 1);
    llmMainLayout->addLayout(providerLayout);
    apiLayout->addWidget(llmGroup);

    // Claude API
    auto* claudeGroup = new QGroupBox("Claude API (Anthropic)", apiTab);
    auto* claudeLayout = new QHBoxLayout(claudeGroup);
    m_claudeKeyEdit = new QLineEdit(claudeGroup);
    m_claudeKeyEdit->setEchoMode(QLineEdit::Password);
    m_claudeKeyEdit->setPlaceholderText("sk-ant-...");
    auto* testClaudeBtn = new QPushButton("Tester", claudeGroup);
    connect(testClaudeBtn, &QPushButton::clicked, this, &SettingsDialog::onTestClaude);
    claudeLayout->addWidget(new QLabel("Cle API:"));
    claudeLayout->addWidget(m_claudeKeyEdit, 1);
    claudeLayout->addWidget(testClaudeBtn);
    apiLayout->addWidget(claudeGroup);

    // Google AI Studio (for Gemini prompts - free tier)
    auto* aiStudioGroup = new QGroupBox("AI Studio (Gemini - Prompts)", apiTab);
    auto* aiStudioMainLayout = new QVBoxLayout(aiStudioGroup);

    auto* aiStudioKeyLayout = new QHBoxLayout();
    m_aiStudioKeyEdit = new QLineEdit(aiStudioGroup);
    m_aiStudioKeyEdit->setEchoMode(QLineEdit::Password);
    m_aiStudioKeyEdit->setPlaceholderText("AIza... (cle AI Studio)");
    auto* testAiStudioBtn = new QPushButton("Tester", aiStudioGroup);
    connect(testAiStudioBtn, &QPushButton::clicked, this, &SettingsDialog::onTestGoogleAI);
    aiStudioKeyLayout->addWidget(new QLabel("Cle API:"));
    aiStudioKeyLayout->addWidget(m_aiStudioKeyEdit, 1);
    aiStudioKeyLayout->addWidget(testAiStudioBtn);
    aiStudioMainLayout->addLayout(aiStudioKeyLayout);

    auto* aiStudioInfoLabel = new QLabel(
        "<i>Gratuit avec quotas genereux. Pour Gemini 3 Pro (generation de prompts).</i>",
        aiStudioGroup
    );
    aiStudioInfoLabel->setStyleSheet("color: #81c784; font-size: 10px;");
    aiStudioMainLayout->addWidget(aiStudioInfoLabel);

    apiLayout->addWidget(aiStudioGroup);

    // Vertex AI (for Imagen/Veo - paid)
    auto* vertexGroup = new QGroupBox("Vertex AI (Imagen, Veo - Images/Videos)", apiTab);
    auto* vertexMainLayout = new QVBoxLayout(vertexGroup);

    auto* vertexKeyLayout = new QHBoxLayout();
    m_vertexAiKeyEdit = new QLineEdit(vertexGroup);
    m_vertexAiKeyEdit->setEchoMode(QLineEdit::Password);
    m_vertexAiKeyEdit->setPlaceholderText("AIza... (cle Vertex AI/GCP)");
    auto* testVertexBtn = new QPushButton("Tester", vertexGroup);
    connect(testVertexBtn, &QPushButton::clicked, this, [this]() {
        // Test Vertex AI endpoint
        QString key = m_vertexAiKeyEdit->text();
        if (key.isEmpty() || key.startsWith("•")) {
            key = codex::utils::SecureStorage::instance().getApiKey(
                codex::utils::SecureStorage::SERVICE_IMAGEN
            );
        }
        if (key.isEmpty()) {
            codex::utils::MessageBox::warning(this, "Test", "Veuillez d'abord entrer une cle API.");
            return;
        }
        setCursor(Qt::WaitCursor);
        QString testUrl = QString("https://aiplatform.googleapis.com/v1/publishers/google/models/gemini-2.0-flash:generateContent?key=%1").arg(key);
        QNetworkAccessManager* netManager = new QNetworkAccessManager(this);
        QUrl testQUrl(testUrl);
        QNetworkRequest testReq;
        testReq.setUrl(testQUrl);
        testReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QJsonObject testBody;
        QJsonArray testContents;
        QJsonObject testContent;
        testContent["role"] = "user";
        QJsonArray testParts;
        QJsonObject testPart;
        testPart["text"] = "Hi";
        testParts.append(testPart);
        testContent["parts"] = testParts;
        testContents.append(testContent);
        testBody["contents"] = testContents;
        QJsonObject testGenConfig;
        testGenConfig["maxOutputTokens"] = 10;
        testBody["generationConfig"] = testGenConfig;
        QNetworkReply* testReply = netManager->post(testReq, QJsonDocument(testBody).toJson());
        connect(testReply, &QNetworkReply::finished, this, [this, testReply, netManager]() {
            setCursor(Qt::ArrowCursor);
            if (testReply->error() == QNetworkReply::NoError) {
                codex::utils::MessageBox::info(this, "Test Vertex AI", "Connexion Vertex AI reussie !");
            } else {
                QString errorMsg = testReply->errorString();
                QByteArray responseData = testReply->readAll();
                if (!responseData.isEmpty()) {
                    QJsonDocument doc = QJsonDocument::fromJson(responseData);
                    if (doc.isObject() && doc.object().contains("error")) {
                        errorMsg = doc.object()["error"].toObject()["message"].toString();
                    }
                }
                codex::utils::MessageBox::warning(this, "Test Vertex AI", QString("Erreur: %1").arg(errorMsg));
            }
            testReply->deleteLater();
            netManager->deleteLater();
        });
    });
    vertexKeyLayout->addWidget(new QLabel("Cle API:"));
    vertexKeyLayout->addWidget(m_vertexAiKeyEdit, 1);
    vertexKeyLayout->addWidget(testVertexBtn);
    vertexMainLayout->addLayout(vertexKeyLayout);

    auto* vertexInfoLabel = new QLabel(
        "<i>Payant. Pour Imagen (images) et Veo (videos). Quotas entreprise.</i>",
        vertexGroup
    );
    vertexInfoLabel->setStyleSheet("color: #ffb74d; font-size: 10px;");
    vertexMainLayout->addWidget(vertexInfoLabel);

    apiLayout->addWidget(vertexGroup);

    // TTS (Text-to-Speech) Section
    auto* ttsGroup = new QGroupBox("Synthese Vocale (TTS)", apiTab);
    auto* ttsMainLayout = new QVBoxLayout(ttsGroup);

    // TTS Provider selection
    auto* ttsProviderLayout = new QHBoxLayout();
    m_ttsProviderCombo = new QComboBox(ttsGroup);
    m_ttsProviderCombo->addItem("Edge TTS (Gratuit)", "edge");
    m_ttsProviderCombo->addItem("ElevenLabs (Premium)", "elevenlabs");
    ttsProviderLayout->addWidget(new QLabel("Fournisseur:"));
    ttsProviderLayout->addWidget(m_ttsProviderCombo, 1);
    ttsMainLayout->addLayout(ttsProviderLayout);

    // Edge TTS voice selection
    auto* edgeVoiceLayout = new QHBoxLayout();
    m_edgeVoiceCombo = new QComboBox(ttsGroup);
    m_edgeVoiceCombo->addItem("Henri (Homme, narrateur)", "fr-FR-HenriNeural");
    m_edgeVoiceCombo->addItem("Denise (Femme)", "fr-FR-DeniseNeural");
    m_edgeVoiceCombo->addItem("Eloise (Femme, douce)", "fr-FR-EloiseNeural");
    m_edgeVoiceCombo->addItem("Alain (Homme)", "fr-FR-AlainNeural");
    m_edgeVoiceCombo->addItem("Claude (Homme, chaleureux)", "fr-FR-ClaudeNeural");
    m_edgeVoiceCombo->addItem("Maurice (Homme, narrateur)", "fr-FR-MauriceNeural");
    m_edgeVoiceCombo->addItem("Jacqueline (Femme, pro)", "fr-FR-JacquelineNeural");
    m_edgeVoiceCombo->addItem("Jerome (Homme)", "fr-FR-JeromeNeural");
    m_edgeVoiceCombo->addItem("Guy (Anglais US)", "en-US-GuyNeural");
    m_edgeVoiceCombo->addItem("Ryan (Anglais UK)", "en-GB-RyanNeural");
    auto* testEdgeBtn = new QPushButton("Tester", ttsGroup);
    connect(testEdgeBtn, &QPushButton::clicked, this, &SettingsDialog::onTestEdgeTTS);
    edgeVoiceLayout->addWidget(new QLabel("Voix Edge:"));
    edgeVoiceLayout->addWidget(m_edgeVoiceCombo, 1);
    edgeVoiceLayout->addWidget(testEdgeBtn);
    ttsMainLayout->addLayout(edgeVoiceLayout);

    // ElevenLabs API Key
    auto* elevenKeyLayout = new QHBoxLayout();
    m_elevenLabsKeyEdit = new QLineEdit(ttsGroup);
    m_elevenLabsKeyEdit->setEchoMode(QLineEdit::Password);
    m_elevenLabsKeyEdit->setPlaceholderText("Cle ElevenLabs (optionnel)");
    auto* testElevenBtn = new QPushButton("Tester", ttsGroup);
    connect(testElevenBtn, &QPushButton::clicked, this, &SettingsDialog::onTestElevenLabs);
    elevenKeyLayout->addWidget(new QLabel("Cle ElevenLabs:"));
    elevenKeyLayout->addWidget(m_elevenLabsKeyEdit, 1);
    elevenKeyLayout->addWidget(testElevenBtn);
    ttsMainLayout->addLayout(elevenKeyLayout);

    // ElevenLabs voice selection
    auto* voiceLayout = new QHBoxLayout();
    m_voiceCombo = new QComboBox(ttsGroup);
    m_voiceCombo->addItem("Daniel (Narrateur profond)", "onwK4e9ZLuTAKqWW03F9");
    m_voiceCombo->addItem("Adam (Narrateur clair)", "pNInz6obpgDQGcFmaJgB");
    m_voiceCombo->addItem("Antoni (Bien articule)", "ErXwobaYiN019PkySvjV");
    m_voiceCombo->addItem("Arnold (Grave, autoritaire)", "VR6AewLTigWG4xSOukaG");
    m_voiceCombo->addItem("Josh (Jeune, dynamique)", "TxGEqnHWrfWFTfGW9XjX");
    m_voiceCombo->addItem("Sam (Narrateur epique)", "yoZ06aMxZJJ28mfd3POQ");
    m_voiceCombo->addItem("Rachel (Feminin, doux)", "21m00Tcm4TlvDq8ikWAM");
    m_voiceCombo->addItem("Domi (Feminin, energique)", "AZnzlk1XvdvUeBnXmlld");
    m_voiceCombo->addItem("Bella (Feminin, chaleureux)", "EXAVITQu4vr4xnSDxMaL");
    m_voiceCombo->addItem("Elli (Feminin, clair)", "MF3mGyEYCl7XYWbV9V6O");
    voiceLayout->addWidget(new QLabel("Voix ElevenLabs:"));
    voiceLayout->addWidget(m_voiceCombo, 1);
    ttsMainLayout->addLayout(voiceLayout);

    auto* ttsInfoLabel = new QLabel(
        "<i>Edge TTS: Gratuit, voix Microsoft de haute qualite.<br>"
        "ElevenLabs: Premium, voix ultra-realistes (cle API requise).</i>",
        ttsGroup
    );
    ttsInfoLabel->setStyleSheet("color: #888; font-size: 10px;");
    ttsMainLayout->addWidget(ttsInfoLabel);

    apiLayout->addWidget(ttsGroup);

    // Info
    auto* infoLabel = new QLabel(
        "<i>Les clés API sont stockées de manière sécurisée sur votre machine "
        "(DPAPI sur Windows, Keychain sur macOS).</i>",
        apiTab
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #888;");
    infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    apiLayout->addWidget(infoLabel);

    apiLayout->addStretch();
    apiScrollArea->setWidget(apiTab);
    m_tabWidget->addTab(apiScrollArea, "Clés API");

    // ========== Paths Tab ==========
    auto* pathsTab = new QWidget();
    auto* pathsLayout = new QFormLayout(pathsTab);

    // Codex file path
    auto* codexPathLayout = new QHBoxLayout();
    m_codexPathEdit = new QLineEdit(pathsTab);
    m_codexPathEdit->setReadOnly(true);
    auto* browseCodexBtn = new QPushButton("Parcourir...", pathsTab);
    connect(browseCodexBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, "Sélectionner fichier Codex", QString(), "Markdown (*.md)"
        );
        if (!path.isEmpty()) {
            m_codexPathEdit->setText(path);
        }
    });
    codexPathLayout->addWidget(m_codexPathEdit, 1);
    codexPathLayout->addWidget(browseCodexBtn);
    pathsLayout->addRow("Fichier Codex:", codexPathLayout);

    // Images output path
    auto* imagesPathLayout = new QHBoxLayout();
    m_outputImagesPathEdit = new QLineEdit(pathsTab);
    auto* browseImagesBtn = new QPushButton("Parcourir...", pathsTab);
    connect(browseImagesBtn, &QPushButton::clicked, this, [this]() {
        QString startDir = m_outputImagesPathEdit->text();
        if (startDir.isEmpty()) startDir = QDir::homePath();
        QString path = QFileDialog::getExistingDirectory(
            this, "Selectionner dossier images", startDir,
            QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog
        );
        if (!path.isEmpty()) {
            m_outputImagesPathEdit->setText(path);
        }
    });
    imagesPathLayout->addWidget(m_outputImagesPathEdit, 1);
    imagesPathLayout->addWidget(browseImagesBtn);
    pathsLayout->addRow("Dossier images:", imagesPathLayout);

    // Videos output path
    auto* videosPathLayout = new QHBoxLayout();
    m_outputVideosPathEdit = new QLineEdit(pathsTab);
    auto* browseVideosBtn = new QPushButton("Parcourir...", pathsTab);
    connect(browseVideosBtn, &QPushButton::clicked, this, [this]() {
        QString startDir = m_outputVideosPathEdit->text();
        if (startDir.isEmpty()) startDir = QDir::homePath();
        QString path = QFileDialog::getExistingDirectory(
            this, "Selectionner dossier videos", startDir,
            QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog
        );
        if (!path.isEmpty()) {
            m_outputVideosPathEdit->setText(path);
        }
    });
    videosPathLayout->addWidget(m_outputVideosPathEdit, 1);
    videosPathLayout->addWidget(browseVideosBtn);
    pathsLayout->addRow("Dossier videos:", videosPathLayout);

    m_tabWidget->addTab(pathsTab, "Chemins");

    // ========== Appearance Tab ==========
    auto* appearanceScrollArea = new QScrollArea();
    appearanceScrollArea->setWidgetResizable(true);
    appearanceScrollArea->setFrameShape(QFrame::NoFrame);
    auto* appearanceTab = new QWidget();
    auto* appearanceLayout = new QVBoxLayout(appearanceTab);

    // Theme section
    auto* themeGroup = new QGroupBox("Theme", appearanceTab);
    auto* themeLayout = new QFormLayout(themeGroup);

    m_themeCombo = new QComboBox(themeGroup);
    m_themeCombo->addItem("Sombre", "dark");
    m_themeCombo->addItem("Clair", "light");
    themeLayout->addRow("Theme:", m_themeCombo);

    // Accent color
    auto* accentLayout = new QHBoxLayout();
    m_accentColorBtn = new QPushButton(themeGroup);
    m_accentColorBtn->setMinimumWidth(100);
    m_accentColorBtn->setMinimumHeight(30);
    connect(m_accentColorBtn, &QPushButton::clicked, this, &SettingsDialog::onChooseAccentColor);
    accentLayout->addWidget(m_accentColorBtn);
    accentLayout->addStretch();
    themeLayout->addRow("Couleur d'accent:", accentLayout);

    // Selection background color
    auto* selBgLayout = new QHBoxLayout();
    m_selectionBgColorBtn = new QPushButton(themeGroup);
    m_selectionBgColorBtn->setMinimumWidth(100);
    m_selectionBgColorBtn->setMinimumHeight(30);
    connect(m_selectionBgColorBtn, &QPushButton::clicked, this, &SettingsDialog::onChooseSelectionBgColor);
    selBgLayout->addWidget(m_selectionBgColorBtn);
    selBgLayout->addStretch();
    themeLayout->addRow("Fond selection:", selBgLayout);

    // Selection text color
    auto* selTextLayout = new QHBoxLayout();
    m_selectionTextColorBtn = new QPushButton(themeGroup);
    m_selectionTextColorBtn->setMinimumWidth(100);
    m_selectionTextColorBtn->setMinimumHeight(30);
    connect(m_selectionTextColorBtn, &QPushButton::clicked, this, &SettingsDialog::onChooseSelectionTextColor);
    selTextLayout->addWidget(m_selectionTextColorBtn);
    selTextLayout->addStretch();
    themeLayout->addRow("Texte selection:", selTextLayout);

    appearanceLayout->addWidget(themeGroup);

    // UI Font section
    auto* uiFontGroup = new QGroupBox("Police Interface", appearanceTab);
    auto* uiFontLayout = new QFormLayout(uiFontGroup);

    m_uiFontCombo = new QFontComboBox(uiFontGroup);
    uiFontLayout->addRow("Famille:", m_uiFontCombo);

    m_uiFontSizeSpin = new QSpinBox(uiFontGroup);
    m_uiFontSizeSpin->setRange(8, 16);
    m_uiFontSizeSpin->setSuffix(" pt");
    uiFontLayout->addRow("Taille:", m_uiFontSizeSpin);

    appearanceLayout->addWidget(uiFontGroup);

    // Text Font section
    auto* textFontGroup = new QGroupBox("Police Texte (Codex)", appearanceTab);
    auto* textFontLayout = new QFormLayout(textFontGroup);

    m_textFontCombo = new QFontComboBox(textFontGroup);
    m_textFontCombo->setFontFilters(QFontComboBox::MonospacedFonts);
    textFontLayout->addRow("Famille:", m_textFontCombo);

    m_textFontSizeSpin = new QSpinBox(textFontGroup);
    m_textFontSizeSpin->setRange(8, 24);
    m_textFontSizeSpin->setSuffix(" pt");
    textFontLayout->addRow("Taille:", m_textFontSizeSpin);

    appearanceLayout->addWidget(textFontGroup);

    // Damier (alternating rows) section
    auto* damierGroup = new QGroupBox("Damier (Lignes alternees)", appearanceTab);
    auto* damierLayout = new QFormLayout(damierGroup);

    m_damierEnabledCheck = new QCheckBox("Activer le damier", damierGroup);
    damierLayout->addRow("", m_damierEnabledCheck);

    auto* contrastLayout = new QHBoxLayout();
    m_damierContrastSlider = new QSlider(Qt::Horizontal, damierGroup);
    m_damierContrastSlider->setRange(0, 100);
    m_damierContrastSlider->setTickPosition(QSlider::TicksBelow);
    m_damierContrastSlider->setTickInterval(25);
    m_damierContrastLabel = new QLabel("30%", damierGroup);
    m_damierContrastLabel->setMinimumWidth(40);
    contrastLayout->addWidget(m_damierContrastSlider, 1);
    contrastLayout->addWidget(m_damierContrastLabel);
    damierLayout->addRow("Contraste:", contrastLayout);

    // Update label when slider changes
    connect(m_damierContrastSlider, &QSlider::valueChanged, this, [this](int value) {
        m_damierContrastLabel->setText(QString("%1%").arg(value));
    });

    // Enable/disable slider based on checkbox
    connect(m_damierEnabledCheck, &QCheckBox::toggled, m_damierContrastSlider, &QSlider::setEnabled);

    appearanceLayout->addWidget(damierGroup);

    appearanceLayout->addStretch();
    appearanceScrollArea->setWidget(appearanceTab);
    m_tabWidget->addTab(appearanceScrollArea, "Apparence");

    mainLayout->addWidget(m_tabWidget);

    // ========== Buttons ==========
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* cancelBtn = new QPushButton("Annuler", this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);

    auto* applyBtn = new QPushButton("Appliquer", this);
    connect(applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
    buttonLayout->addWidget(applyBtn);

    auto* saveBtn = new QPushButton("Sauvegarder", this);
    saveBtn->setDefault(true);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    buttonLayout->addWidget(saveBtn);

    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::loadSettings() {
    auto& storage = codex::utils::SecureStorage::instance();
    auto& config = codex::utils::Config::instance();

    // Load API keys (show masked)
    if (storage.hasApiKey(storage.SERVICE_CLAUDE)) {
        m_claudeKeyEdit->setText("••••••••••••••••");
        m_claudeKeyEdit->setProperty("hasKey", true);
    }
    if (storage.hasApiKey(storage.SERVICE_AISTUDIO)) {
        m_aiStudioKeyEdit->setText("••••••••••••••••");
        m_aiStudioKeyEdit->setProperty("hasKey", true);
    }
    if (storage.hasApiKey(storage.SERVICE_IMAGEN)) {
        m_vertexAiKeyEdit->setText("••••••••••••••••");
        m_vertexAiKeyEdit->setProperty("hasKey", true);
    }
    if (storage.hasApiKey(storage.SERVICE_ELEVENLABS)) {
        m_elevenLabsKeyEdit->setText("••••••••••••••••");
        m_elevenLabsKeyEdit->setProperty("hasKey", true);
    }

    // Load LLM provider selection
    QString llmProvider = config.llmProvider();
    int providerIndex = m_llmProviderCombo->findData(llmProvider);
    if (providerIndex >= 0) {
        m_llmProviderCombo->setCurrentIndex(providerIndex);
    }

    // Load TTS provider selection
    QString ttsProvider = config.ttsProvider();
    int ttsIndex = m_ttsProviderCombo->findData(ttsProvider);
    if (ttsIndex >= 0) {
        m_ttsProviderCombo->setCurrentIndex(ttsIndex);
    }

    // Load Edge TTS voice selection
    QString edgeVoice = config.edgeTtsVoice();
    if (!edgeVoice.isEmpty()) {
        int index = m_edgeVoiceCombo->findData(edgeVoice);
        if (index >= 0) {
            m_edgeVoiceCombo->setCurrentIndex(index);
        }
    }

    // Load ElevenLabs voice selection
    QString voiceId = config.elevenLabsVoiceId();
    if (!voiceId.isEmpty()) {
        int index = m_voiceCombo->findData(voiceId);
        if (index >= 0) {
            m_voiceCombo->setCurrentIndex(index);
        }
    }

    // Load paths
    m_codexPathEdit->setText(config.codexFilePath());
    m_outputImagesPathEdit->setText(config.outputImagesPath());
    m_outputVideosPathEdit->setText(config.outputVideosPath());

    // Load appearance settings
    auto& theme = codex::utils::ThemeManager::instance();
    int themeIndex = m_themeCombo->findData(theme.currentTheme());
    if (themeIndex >= 0) {
        m_themeCombo->setCurrentIndex(themeIndex);
    }

    m_currentAccentColor = theme.accentColor();
    m_accentColorBtn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #888;").arg(m_currentAccentColor));

    // Load selection colors
    auto colors = theme.colors();
    m_currentSelectionBgColor = colors.selection;
    m_currentSelectionTextColor = colors.selectionText;
    m_selectionBgColorBtn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #888;").arg(m_currentSelectionBgColor));
    m_selectionTextColorBtn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #888;").arg(m_currentSelectionTextColor));

    auto fonts = theme.fontSettings();
    m_uiFontCombo->setCurrentFont(QFont(fonts.uiFamily));
    m_uiFontSizeSpin->setValue(fonts.uiSize);
    m_textFontCombo->setCurrentFont(QFont(fonts.textFamily));
    m_textFontSizeSpin->setValue(fonts.textSize);

    // Load damier settings
    auto damier = theme.damierSettings();
    m_damierEnabledCheck->setChecked(damier.enabled);
    m_damierContrastSlider->setValue(damier.contrast);
    m_damierContrastSlider->setEnabled(damier.enabled);
    m_damierContrastLabel->setText(QString("%1%").arg(damier.contrast));
}

void SettingsDialog::saveSettings() {
    auto& storage = codex::utils::SecureStorage::instance();
    auto& config = codex::utils::Config::instance();

    // Save API keys (only if changed - not the masked placeholder)
    QString claudeKey = m_claudeKeyEdit->text();
    if (!claudeKey.isEmpty() && !claudeKey.startsWith("•")) {
        storage.storeApiKey(storage.SERVICE_CLAUDE, claudeKey);
    }

    QString aiStudioKey = m_aiStudioKeyEdit->text();
    if (!aiStudioKey.isEmpty() && !aiStudioKey.startsWith("•")) {
        storage.storeApiKey(storage.SERVICE_AISTUDIO, aiStudioKey);
    }

    QString vertexKey = m_vertexAiKeyEdit->text();
    if (!vertexKey.isEmpty() && !vertexKey.startsWith("•")) {
        storage.storeApiKey(storage.SERVICE_IMAGEN, vertexKey);
    }

    // Save LLM provider selection
    QString llmProvider = m_llmProviderCombo->currentData().toString();
    config.setLlmProvider(llmProvider);

    QString elevenKey = m_elevenLabsKeyEdit->text();
    if (!elevenKey.isEmpty() && !elevenKey.startsWith("•")) {
        storage.storeApiKey(storage.SERVICE_ELEVENLABS, elevenKey);
    }

    // Save TTS provider selection
    QString ttsProvider = m_ttsProviderCombo->currentData().toString();
    config.setTtsProvider(ttsProvider);

    // Save Edge TTS voice selection
    QString edgeVoice = m_edgeVoiceCombo->currentData().toString();
    config.setEdgeTtsVoice(edgeVoice);

    // Save ElevenLabs voice selection
    QString selectedVoiceId = m_voiceCombo->currentData().toString();
    config.setElevenLabsVoiceId(selectedVoiceId);

    // Save paths
    config.setCodexFilePath(m_codexPathEdit->text());
    config.setOutputImagesPath(m_outputImagesPathEdit->text());
    config.setOutputVideosPath(m_outputVideosPathEdit->text());

    // Save appearance settings
    auto& theme = codex::utils::ThemeManager::instance();
    theme.setTheme(m_themeCombo->currentData().toString());
    theme.setAccentColor(m_currentAccentColor);

    codex::utils::FontSettings fonts;
    fonts.uiFamily = m_uiFontCombo->currentFont().family();
    fonts.uiSize = m_uiFontSizeSpin->value();
    fonts.textFamily = m_textFontCombo->currentFont().family();
    fonts.textSize = m_textFontSizeSpin->value();
    theme.setFontSettings(fonts);

    // Save damier settings
    codex::utils::DamierSettings damier;
    damier.enabled = m_damierEnabledCheck->isChecked();
    damier.contrast = m_damierContrastSlider->value();
    theme.setDamierSettings(damier);

    theme.save();

    LOG_INFO("Settings saved");
}

void SettingsDialog::applyAppearance() {
    auto& theme = codex::utils::ThemeManager::instance();
    theme.setTheme(m_themeCombo->currentData().toString());
    theme.setAccentColor(m_currentAccentColor);
    theme.setSelectionColors(m_currentSelectionBgColor, m_currentSelectionTextColor);

    codex::utils::FontSettings fonts;
    fonts.uiFamily = m_uiFontCombo->currentFont().family();
    fonts.uiSize = m_uiFontSizeSpin->value();
    fonts.textFamily = m_textFontCombo->currentFont().family();
    fonts.textSize = m_textFontSizeSpin->value();
    theme.setFontSettings(fonts);

    // Apply damier settings
    codex::utils::DamierSettings damier;
    damier.enabled = m_damierEnabledCheck->isChecked();
    damier.contrast = m_damierContrastSlider->value();
    theme.setDamierSettings(damier);

    theme.apply();
}

void SettingsDialog::onApply() {
    applyAppearance();
    LOG_INFO("Appearance applied");
}

void SettingsDialog::onChooseAccentColor() {
    QColor color = QColorDialog::getColor(QColor(m_currentAccentColor), this, "Choisir couleur d'accent");
    if (color.isValid()) {
        m_currentAccentColor = color.name();
        m_accentColorBtn->setStyleSheet(
            QString("background-color: %1; border: 1px solid #888;").arg(m_currentAccentColor));
    }
}

void SettingsDialog::onChooseSelectionBgColor() {
    QColor color = QColorDialog::getColor(QColor(m_currentSelectionBgColor), this, "Choisir fond de selection");
    if (color.isValid()) {
        m_currentSelectionBgColor = color.name();
        m_selectionBgColorBtn->setStyleSheet(
            QString("background-color: %1; border: 1px solid #888;").arg(m_currentSelectionBgColor));
    }
}

void SettingsDialog::onChooseSelectionTextColor() {
    QColor color = QColorDialog::getColor(QColor(m_currentSelectionTextColor), this, "Choisir couleur texte selection");
    if (color.isValid()) {
        m_currentSelectionTextColor = color.name();
        m_selectionTextColorBtn->setStyleSheet(
            QString("background-color: %1; border: 1px solid #888;").arg(m_currentSelectionTextColor));
    }
}

void SettingsDialog::onSave() {
    saveSettings();
    applyAppearance();
    codex::utils::MessageBox::info(this, "Paramètres", "Paramètres sauvegardés avec succès.");
    accept();
}

void SettingsDialog::onTestClaude() {
    QString key = m_claudeKeyEdit->text();
    if (key.isEmpty() || key.startsWith("•")) {
        key = codex::utils::SecureStorage::instance().getApiKey(
            codex::utils::SecureStorage::SERVICE_CLAUDE
        );
    }

    if (key.isEmpty()) {
        codex::utils::MessageBox::warning(this, "Test", "Veuillez d'abord entrer une clé API.");
        return;
    }

    setCursor(Qt::WaitCursor);

    QNetworkAccessManager manager;
    QUrl reqUrl("https://api.anthropic.com/v1/messages");
    QNetworkRequest req(reqUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-api-key", key.toUtf8());
    req.setRawHeader("anthropic-version", "2023-06-01");

    QJsonObject body;
    body["model"] = "claude-sonnet-4-20250514";
    body["max_tokens"] = 10;
    QJsonArray messages;
    QJsonObject msg;
    msg["role"] = "user";
    msg["content"] = "Hi";
    messages.append(msg);
    body["messages"] = messages;

    QByteArray postData = QJsonDocument(body).toJson();
    QNetworkReply* reply = manager.post(req, postData);

    // Attendre la réponse avec timeout de 30 secondes
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(30000);
    loop.exec();

    setCursor(Qt::ArrowCursor);

    if (reply->isRunning()) {
        reply->abort();
        codex::utils::MessageBox::warning(this, "Test Claude", "Timeout - pas de reponse du serveur (30s).");
        reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        codex::utils::MessageBox::info(this, "Test Claude", "Connexion reussie !");
    } else {
        QString errorMsg = reply->errorString();
        QByteArray responseData = reply->readAll();
        if (!responseData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isObject() && doc.object().contains("error")) {
                errorMsg = doc.object()["error"].toObject()["message"].toString();
            }
        }
        codex::utils::MessageBox::warning(this, "Test Claude", QString("Erreur: %1").arg(errorMsg));
    }
    reply->deleteLater();
}

void SettingsDialog::onTestGoogleAI() {
    // Test AI Studio key
    QString key = m_aiStudioKeyEdit->text();
    if (key.isEmpty() || key.startsWith("•")) {
        key = codex::utils::SecureStorage::instance().getApiKey(
            codex::utils::SecureStorage::SERVICE_AISTUDIO
        );
    }

    if (key.isEmpty()) {
        codex::utils::MessageBox::warning(this, "Test", "Veuillez d'abord entrer une cle API AI Studio.");
        return;
    }

    setCursor(Qt::WaitCursor);

    // AI Studio endpoint
    QString url = QString("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=%1").arg(key);

    QNetworkAccessManager manager;
    QUrl reqUrl(url);
    QNetworkRequest req(reqUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    QJsonArray contents;
    QJsonObject content;
    content["role"] = "user";
    QJsonArray parts;
    QJsonObject part;
    part["text"] = "Hi";
    parts.append(part);
    content["parts"] = parts;
    contents.append(content);
    body["contents"] = contents;

    QJsonObject genConfig;
    genConfig["maxOutputTokens"] = 10;
    body["generationConfig"] = genConfig;

    QByteArray postData = QJsonDocument(body).toJson();
    QNetworkReply* reply = manager.post(req, postData);

    // Attendre la réponse avec timeout de 30 secondes
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(30000);
    loop.exec();

    setCursor(Qt::ArrowCursor);

    if (reply->isRunning()) {
        reply->abort();
        codex::utils::MessageBox::warning(this, "Test AI Studio", "Timeout - pas de reponse du serveur (30s).");
        reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        codex::utils::MessageBox::info(this, "Test AI Studio", "Connexion AI Studio reussie !");
    } else {
        QString errorMsg = reply->errorString();
        QByteArray responseData = reply->readAll();
        if (!responseData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isObject() && doc.object().contains("error")) {
                errorMsg = doc.object()["error"].toObject()["message"].toString();
            }
        }
        codex::utils::MessageBox::warning(this, "Test AI Studio", QString("Erreur: %1").arg(errorMsg));
    }
    reply->deleteLater();
}

void SettingsDialog::onTestElevenLabs() {
    QString key = m_elevenLabsKeyEdit->text();
    if (key.isEmpty() || key.startsWith("•")) {
        key = codex::utils::SecureStorage::instance().getApiKey(
            codex::utils::SecureStorage::SERVICE_ELEVENLABS
        );
    }

    if (key.isEmpty()) {
        codex::utils::MessageBox::warning(this, "Test", "Veuillez d'abord entrer une clé API.");
        return;
    }

    setCursor(Qt::WaitCursor);

    QNetworkAccessManager manager;
    QUrl reqUrl("https://api.elevenlabs.io/v1/user");
    QNetworkRequest req(reqUrl);
    req.setRawHeader("xi-api-key", key.toUtf8());

    QNetworkReply* reply = manager.get(req);

    // Attendre la réponse avec timeout de 30 secondes
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(30000);
    loop.exec();

    setCursor(Qt::ArrowCursor);

    if (reply->isRunning()) {
        reply->abort();
        codex::utils::MessageBox::warning(this, "Test ElevenLabs", "Timeout - pas de reponse du serveur (30s).");
        reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QString subscription = doc.object()["subscription"].toObject()["tier"].toString();
        int charCount = doc.object()["subscription"].toObject()["character_count"].toInt();
        int charLimit = doc.object()["subscription"].toObject()["character_limit"].toInt();
        codex::utils::MessageBox::info(this, "Test ElevenLabs",
            QString("Connexion reussie !\nAbonnement: %1\nCaracteres: %2 / %3")
                .arg(subscription).arg(charCount).arg(charLimit));
    } else {
        QString errorMsg = reply->errorString();
        QByteArray responseData = reply->readAll();
        if (!responseData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isObject() && doc.object().contains("detail")) {
                errorMsg = doc.object()["detail"].toObject()["message"].toString();
            }
        }
        codex::utils::MessageBox::warning(this, "Test ElevenLabs", QString("Erreur: %1").arg(errorMsg));
    }
    reply->deleteLater();
}

void SettingsDialog::onTestEdgeTTS() {
    QString voiceId = m_edgeVoiceCombo->currentData().toString();
    QString voiceName = m_edgeVoiceCombo->currentText();

    setCursor(Qt::WaitCursor);

    // Texte de test selon la langue
    QString testText;
    if (voiceId.startsWith("fr-")) {
        testText = "Bonjour, ceci est un test de la voix Edge.";
    } else {
        testText = "Hello, this is a test of the Edge voice.";
    }

    // Utiliser PowerShell avec Windows SAPI
    QString escapedText = testText;
    escapedText.replace("'", "''");

    QString script = QString(
        "Add-Type -AssemblyName System.Speech; "
        "$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer; "
        "$synth.Rate = 0; "
        "$synth.Volume = 100; "
        "$synth.Speak('%1')"
    ).arg(escapedText);

    QProcess process;
    process.start("powershell", QStringList() << "-NoProfile" << "-Command" << script);

    if (!process.waitForFinished(15000)) {
        setCursor(Qt::ArrowCursor);
        codex::utils::MessageBox::warning(this, "Test Edge TTS", "Timeout - la synthese vocale a pris trop de temps.");
        return;
    }

    setCursor(Qt::ArrowCursor);

    if (process.exitCode() == 0) {
        codex::utils::MessageBox::info(this, "Test Edge TTS",
            QString("Test termine !\nVoix: %1").arg(voiceName));
    } else {
        QString error = QString::fromUtf8(process.readAllStandardError());
        codex::utils::MessageBox::warning(this, "Test Edge TTS",
            QString("Erreur: %1").arg(error.isEmpty() ? "Echec de la synthese vocale" : error));
    }
}

} // namespace codex::ui
