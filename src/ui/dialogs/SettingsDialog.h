#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTabWidget>
#include <QComboBox>

namespace codex::ui {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onSave();
    void onTestClaude();
    void onTestGoogleAI();
    void onTestElevenLabs();

private:
    void setupUi();
    void loadSettings();
    void saveSettings();

    // API Keys tab
    QLineEdit* m_claudeKeyEdit;
    QLineEdit* m_googleAiKeyEdit;  // Shared key for Gemini, Imagen, Veo
    QLineEdit* m_elevenLabsKeyEdit;
    QComboBox* m_voiceCombo;
    QComboBox* m_llmProviderCombo;  // Claude vs Gemini

    // Paths tab
    QLineEdit* m_codexPathEdit;
    QLineEdit* m_outputPathEdit;

    QTabWidget* m_tabWidget;
};

} // namespace codex::ui
