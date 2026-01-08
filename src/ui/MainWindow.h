#pragma once

#include <QMainWindow>
#include <QTimer>
#include <memory>
#include "db/repositories/ProjectRepository.h"

namespace codex::api {
class ElevenLabsClient;
}

namespace codex::core {
class TextParser;
class PipelineController;
enum class PipelineState;
}

namespace codex::ui {

class TextViewerWidget;
class ImageViewerWidget;
class TreatiseListWidget;
class PassagePreviewWidget;
class AudioPlayerWidget;
class SlideshowWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onNewProject();
    void onOpenProject();
    void onOpenFile();
    void onSaveProject();
    void onAutoSave();
    void onShowSettings();
    void onShowAbout();

    void onPassageSelected(const QString& text, int start, int end);
    void onGenerateImage();

    void onTreatiseSelected(const QString& code, const QString& title, const QString& category);
    void onTreatiseDoubleClicked(const QString& code, const QString& title, const QString& category);

    void onGenerateImageFromPreview(const QString& passage);
    void onGenerateAudioFromPreview(const QString& passage);

    void onPipelineStateChanged(codex::core::PipelineState state, const QString& message);
    void onPipelineProgress(int percent, const QString& step);
    void onPipelineCompleted(const QPixmap& image, const QString& prompt);
    void onPipelineFailed(const QString& error);
    void onSaveImage();

    void onAudioGenerated(const QByteArray& audioData, int durationMs);
    void onAudioError(const QString& error);

    void onStartSlideshow();

private:
    void setupUi();
    void setupMenus();
    void setupConnections();
    void applyDarkTheme();
    void loadCodexAndRefreshUI(const QString& filePath);

    TreatiseListWidget* m_treatiseList = nullptr;
    TextViewerWidget* m_textViewer = nullptr;
    ImageViewerWidget* m_imageViewer = nullptr;
    PassagePreviewWidget* m_passagePreview = nullptr;
    AudioPlayerWidget* m_audioPlayer = nullptr;

    codex::core::TextParser* m_textParser = nullptr;
    codex::core::PipelineController* m_pipelineController = nullptr;
    codex::api::ElevenLabsClient* m_elevenLabsClient = nullptr;
    SlideshowWidget* m_slideshowWidget = nullptr;

    // Project management
    codex::db::Project m_currentProject;
    QTimer* m_autoSaveTimer = nullptr;
    bool m_projectModified = false;

    QString m_selectedPassage;
    QString m_currentTreatiseCode;
    QString m_currentCategory;

    void updateWindowTitle();
    void setProjectModified(bool modified);
    void loadProject(const codex::db::Project& project);
    void showRecentProjectsOnStartup();
};

} // namespace codex::ui
