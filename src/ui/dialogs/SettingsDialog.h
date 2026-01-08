#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTabWidget>
#include <QComboBox>
#include <QFontComboBox>
#include <QSpinBox>
#include <QPushButton>

namespace codex::ui {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onSave();
    void onApply();
    void onTestClaude();
    void onTestGoogleAI();
    void onTestElevenLabs();
    void onTestEdgeTTS();
    void onChooseAccentColor();

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    void applyAppearance();

    // API Keys tab
    QLineEdit* m_claudeKeyEdit;
    QLineEdit* m_googleAiKeyEdit;  // Shared key for Gemini, Imagen, Veo
    QLineEdit* m_elevenLabsKeyEdit;
    QComboBox* m_voiceCombo;           // ElevenLabs voices
    QComboBox* m_llmProviderCombo;     // Claude vs Gemini
    QComboBox* m_ttsProviderCombo;     // ElevenLabs vs Edge TTS
    QComboBox* m_edgeVoiceCombo;       // Edge TTS voices

    // Google AI endpoint selection
    QComboBox* m_googleProviderCombo;  // AI Studio vs Vertex AI

    // Paths tab
    QLineEdit* m_codexPathEdit;
    QLineEdit* m_outputImagesPathEdit;
    QLineEdit* m_outputVideosPathEdit;

    // Appearance tab
    QComboBox* m_themeCombo;
    QPushButton* m_accentColorBtn;
    QFontComboBox* m_uiFontCombo;
    QSpinBox* m_uiFontSizeSpin;
    QFontComboBox* m_textFontCombo;
    QSpinBox* m_textFontSizeSpin;
    QString m_currentAccentColor;

    QTabWidget* m_tabWidget;
};

} // namespace codex::ui
