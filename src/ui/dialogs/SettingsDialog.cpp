#include "SettingsDialog.h"
#include "utils/SecureStorage.h"
#include "utils/Config.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

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

    // Claude API
    auto* claudeGroup = new QGroupBox("Claude API (Anthropic)", apiTab);
    auto* claudeLayout = new QHBoxLayout(claudeGroup);
    m_claudeKeyEdit = new QLineEdit(claudeGroup);
    m_claudeKeyEdit->setEchoMode(QLineEdit::Password);
    m_claudeKeyEdit->setPlaceholderText("sk-ant-...");
    auto* testClaudeBtn = new QPushButton("Tester", claudeGroup);
    connect(testClaudeBtn, &QPushButton::clicked, this, &SettingsDialog::onTestClaude);
    claudeLayout->addWidget(new QLabel("Clé API:"));
    claudeLayout->addWidget(m_claudeKeyEdit, 1);
    claudeLayout->addWidget(testClaudeBtn);
    apiLayout->addWidget(claudeGroup);

    // Imagen API
    auto* imagenGroup = new QGroupBox("Imagen 3 (Google Cloud)", apiTab);
    auto* imagenLayout = new QHBoxLayout(imagenGroup);
    m_imagenKeyEdit = new QLineEdit(imagenGroup);
    m_imagenKeyEdit->setEchoMode(QLineEdit::Password);
    m_imagenKeyEdit->setPlaceholderText("AIza...");
    auto* testImagenBtn = new QPushButton("Tester", imagenGroup);
    connect(testImagenBtn, &QPushButton::clicked, this, &SettingsDialog::onTestImagen);
    imagenLayout->addWidget(new QLabel("Clé API:"));
    imagenLayout->addWidget(m_imagenKeyEdit, 1);
    imagenLayout->addWidget(testImagenBtn);
    apiLayout->addWidget(imagenGroup);

    // ElevenLabs API
    auto* elevenGroup = new QGroupBox("ElevenLabs", apiTab);
    auto* elevenMainLayout = new QVBoxLayout(elevenGroup);

    // API Key row
    auto* elevenKeyLayout = new QHBoxLayout();
    m_elevenLabsKeyEdit = new QLineEdit(elevenGroup);
    m_elevenLabsKeyEdit->setEchoMode(QLineEdit::Password);
    m_elevenLabsKeyEdit->setPlaceholderText("Votre clé ElevenLabs");
    auto* testElevenBtn = new QPushButton("Tester", elevenGroup);
    connect(testElevenBtn, &QPushButton::clicked, this, &SettingsDialog::onTestElevenLabs);
    elevenKeyLayout->addWidget(new QLabel("Clé API:"));
    elevenKeyLayout->addWidget(m_elevenLabsKeyEdit, 1);
    elevenKeyLayout->addWidget(testElevenBtn);
    elevenMainLayout->addLayout(elevenKeyLayout);

    // Voice selection row
    auto* voiceLayout = new QHBoxLayout();
    m_voiceCombo = new QComboBox(elevenGroup);
    // Add predefined ElevenLabs voices (suitable for narration)
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
    voiceLayout->addWidget(new QLabel("Voix:"));
    voiceLayout->addWidget(m_voiceCombo, 1);
    elevenMainLayout->addLayout(voiceLayout);

    apiLayout->addWidget(elevenGroup);

    // Info
    auto* infoLabel = new QLabel(
        "<i>Les clés API sont stockées de manière sécurisée sur votre machine "
        "(DPAPI sur Windows, Keychain sur macOS).</i>",
        apiTab
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #888;");
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

    // Output path
    auto* outputPathLayout = new QHBoxLayout();
    m_outputPathEdit = new QLineEdit(pathsTab);
    auto* browseOutputBtn = new QPushButton("Parcourir...", pathsTab);
    connect(browseOutputBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getExistingDirectory(
            this, "Sélectionner dossier de sortie"
        );
        if (!path.isEmpty()) {
            m_outputPathEdit->setText(path);
        }
    });
    outputPathLayout->addWidget(m_outputPathEdit, 1);
    outputPathLayout->addWidget(browseOutputBtn);
    pathsLayout->addRow("Dossier de sortie:", outputPathLayout);

    m_tabWidget->addTab(pathsTab, "Chemins");

    mainLayout->addWidget(m_tabWidget);

    // ========== Buttons ==========
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* cancelBtn = new QPushButton("Annuler", this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);

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
        m_imagenKeyEdit->setText("••••••••••••••••");
        m_imagenKeyEdit->setProperty("hasKey", true);
    }
    if (storage.hasApiKey(storage.SERVICE_ELEVENLABS)) {
        m_elevenLabsKeyEdit->setText("••••••••••••••••");
        m_elevenLabsKeyEdit->setProperty("hasKey", true);
    }

    // Load voice selection
    QString voiceId = config.elevenLabsVoiceId();
    if (!voiceId.isEmpty()) {
        int index = m_voiceCombo->findData(voiceId);
        if (index >= 0) {
            m_voiceCombo->setCurrentIndex(index);
        }
    }

    // Load paths
    m_codexPathEdit->setText(config.codexFilePath());
    m_outputPathEdit->setText(config.outputImagesPath());
}

void SettingsDialog::saveSettings() {
    auto& storage = codex::utils::SecureStorage::instance();
    auto& config = codex::utils::Config::instance();

    // Save API keys (only if changed - not the masked placeholder)
    QString claudeKey = m_claudeKeyEdit->text();
    if (!claudeKey.isEmpty() && !claudeKey.startsWith("•")) {
        storage.storeApiKey(storage.SERVICE_CLAUDE, claudeKey);
    }

    QString imagenKey = m_imagenKeyEdit->text();
    if (!imagenKey.isEmpty() && !imagenKey.startsWith("•")) {
        storage.storeApiKey(storage.SERVICE_IMAGEN, imagenKey);
    }

    QString elevenKey = m_elevenLabsKeyEdit->text();
    if (!elevenKey.isEmpty() && !elevenKey.startsWith("•")) {
        storage.storeApiKey(storage.SERVICE_ELEVENLABS, elevenKey);
    }

    // Save voice selection
    QString selectedVoiceId = m_voiceCombo->currentData().toString();
    config.setElevenLabsVoiceId(selectedVoiceId);

    // Save paths
    config.setCodexFilePath(m_codexPathEdit->text());

    LOG_INFO("Settings saved");
}

void SettingsDialog::onSave() {
    saveSettings();
    QMessageBox::information(this, "Paramètres", "Paramètres sauvegardés avec succès.");
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
        QMessageBox::warning(this, "Test", "Veuillez d'abord entrer une clé API.");
        return;
    }

    // TODO: Implement actual API test
    QMessageBox::information(this, "Test Claude", "Test de connexion à implémenter.");
}

void SettingsDialog::onTestImagen() {
    QString key = m_imagenKeyEdit->text();
    if (key.isEmpty() || key.startsWith("•")) {
        key = codex::utils::SecureStorage::instance().getApiKey(
            codex::utils::SecureStorage::SERVICE_IMAGEN
        );
    }

    if (key.isEmpty()) {
        QMessageBox::warning(this, "Test", "Veuillez d'abord entrer une clé API.");
        return;
    }

    // TODO: Implement actual API test
    QMessageBox::information(this, "Test Imagen", "Test de connexion à implémenter.");
}

void SettingsDialog::onTestElevenLabs() {
    QString key = m_elevenLabsKeyEdit->text();
    if (key.isEmpty() || key.startsWith("•")) {
        key = codex::utils::SecureStorage::instance().getApiKey(
            codex::utils::SecureStorage::SERVICE_ELEVENLABS
        );
    }

    if (key.isEmpty()) {
        QMessageBox::warning(this, "Test", "Veuillez d'abord entrer une clé API.");
        return;
    }

    // TODO: Implement actual API test
    QMessageBox::information(this, "Test ElevenLabs", "Test de connexion à implémenter.");
}

} // namespace codex::ui
