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
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QColorDialog>
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

    // Google AI API (Gemini, Imagen, Veo)
    auto* googleGroup = new QGroupBox("Google AI (Gemini, Imagen 3, Veo 2)", apiTab);
    auto* googleMainLayout = new QVBoxLayout(googleGroup);

    auto* googleKeyLayout = new QHBoxLayout();
    m_googleAiKeyEdit = new QLineEdit(googleGroup);
    m_googleAiKeyEdit->setEchoMode(QLineEdit::Password);
    m_googleAiKeyEdit->setPlaceholderText("AIza...");
    auto* testGoogleBtn = new QPushButton("Tester", googleGroup);
    connect(testGoogleBtn, &QPushButton::clicked, this, &SettingsDialog::onTestGoogleAI);
    googleKeyLayout->addWidget(new QLabel("Cle API:"));
    googleKeyLayout->addWidget(m_googleAiKeyEdit, 1);
    googleKeyLayout->addWidget(testGoogleBtn);
    googleMainLayout->addLayout(googleKeyLayout);

    // Google AI Provider selection (AI Studio vs Vertex AI)
    auto* googleProviderLayout = new QHBoxLayout();
    m_googleProviderCombo = new QComboBox(googleGroup);
    m_googleProviderCombo->addItem("AI Studio (generativelanguage.googleapis.com)", "aistudio");
    m_googleProviderCombo->addItem("Vertex AI (aiplatform.googleapis.com)", "vertex");
    googleProviderLayout->addWidget(new QLabel("Endpoint:"));
    googleProviderLayout->addWidget(m_googleProviderCombo, 1);
    googleMainLayout->addLayout(googleProviderLayout);

    auto* googleInfoLabel = new QLabel(
        "<i>Meme cle API pour les deux. Vertex AI offre des quotas plus eleves.</i>",
        googleGroup
    );
    googleInfoLabel->setStyleSheet("color: #888; font-size: 10px;");
    googleMainLayout->addWidget(googleInfoLabel);

    apiLayout->addWidget(googleGroup);

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
    m_tabWidget->addTab(apiTab, "Clés API");

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
        QString path = QFileDialog::getExistingDirectory(
            this, "Selectionner dossier images"
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
        QString path = QFileDialog::getExistingDirectory(
            this, "Selectionner dossier videos"
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

    appearanceLayout->addStretch();
    m_tabWidget->addTab(appearanceTab, "Apparence");

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
    if (storage.hasApiKey(storage.SERVICE_IMAGEN)) {
        m_googleAiKeyEdit->setText("••••••••••••••••");
        m_googleAiKeyEdit->setProperty("hasKey", true);
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

    // Load Google AI provider selection (AI Studio vs Vertex)
    QString googleProvider = config.googleAiProvider();
    int googleProviderIndex = m_googleProviderCombo->findData(googleProvider);
    if (googleProviderIndex >= 0) {
        m_googleProviderCombo->setCurrentIndex(googleProviderIndex);
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

    auto fonts = theme.fontSettings();
    m_uiFontCombo->setCurrentFont(QFont(fonts.uiFamily));
    m_uiFontSizeSpin->setValue(fonts.uiSize);
    m_textFontCombo->setCurrentFont(QFont(fonts.textFamily));
    m_textFontSizeSpin->setValue(fonts.textSize);
}

void SettingsDialog::saveSettings() {
    auto& storage = codex::utils::SecureStorage::instance();
    auto& config = codex::utils::Config::instance();

    // Save API keys (only if changed - not the masked placeholder)
    QString claudeKey = m_claudeKeyEdit->text();
    if (!claudeKey.isEmpty() && !claudeKey.startsWith("•")) {
        storage.storeApiKey(storage.SERVICE_CLAUDE, claudeKey);
    }

    QString googleKey = m_googleAiKeyEdit->text();
    if (!googleKey.isEmpty() && !googleKey.startsWith("•")) {
        storage.storeApiKey(storage.SERVICE_IMAGEN, googleKey);
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

    // Save Google AI provider selection
    QString googleProvider = m_googleProviderCombo->currentData().toString();
    config.setGoogleAiProvider(googleProvider);

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
    theme.save();

    LOG_INFO("Settings saved");
}

void SettingsDialog::applyAppearance() {
    auto& theme = codex::utils::ThemeManager::instance();
    theme.setTheme(m_themeCombo->currentData().toString());
    theme.setAccentColor(m_currentAccentColor);

    codex::utils::FontSettings fonts;
    fonts.uiFamily = m_uiFontCombo->currentFont().family();
    fonts.uiSize = m_uiFontSizeSpin->value();
    fonts.textFamily = m_textFontCombo->currentFont().family();
    fonts.textSize = m_textFontSizeSpin->value();
    theme.setFontSettings(fonts);

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
    QString key = m_googleAiKeyEdit->text();
    if (key.isEmpty() || key.startsWith("•")) {
        key = codex::utils::SecureStorage::instance().getApiKey(
            codex::utils::SecureStorage::SERVICE_IMAGEN
        );
    }

    if (key.isEmpty()) {
        codex::utils::MessageBox::warning(this, "Test", "Veuillez d'abord entrer une clé API.");
        return;
    }

    setCursor(Qt::WaitCursor);

    // Determine endpoint based on provider selection
    QString provider = m_googleProviderCombo->currentData().toString();
    QString url;
    if (provider == "vertex") {
        url = QString("https://aiplatform.googleapis.com/v1/publishers/google/models/gemini-2.0-flash:generateContent?key=%1").arg(key);
    } else {
        url = QString("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=%1").arg(key);
    }

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
        codex::utils::MessageBox::warning(this, "Test Google AI", "Timeout - pas de reponse du serveur (30s).");
        reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QString endpoint = (provider == "vertex") ? "Vertex AI" : "AI Studio";
        codex::utils::MessageBox::info(this, "Test Google AI",
            QString("Connexion reussie !\nEndpoint: %1").arg(endpoint));
    } else {
        QString errorMsg = reply->errorString();
        QByteArray responseData = reply->readAll();
        if (!responseData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isObject() && doc.object().contains("error")) {
                errorMsg = doc.object()["error"].toObject()["message"].toString();
            }
        }
        codex::utils::MessageBox::warning(this, "Test Google AI", QString("Erreur: %1").arg(errorMsg));
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
