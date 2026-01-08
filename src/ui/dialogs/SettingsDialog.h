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
    void onTestImagen();
    void onTestElevenLabs();

private:
    void setupUi();
    void loadSettings();
    void saveSettings();

    // API Keys tab
    QLineEdit* m_claudeKeyEdit;
    QLineEdit* m_imagenKeyEdit;
    QLineEdit* m_elevenLabsKeyEdit;
    QComboBox* m_voiceCombo;

    // Paths tab
    QLineEdit* m_codexPathEdit;
    QLineEdit* m_outputPathEdit;

    QTabWidget* m_tabWidget;
};

} // namespace codex::ui
