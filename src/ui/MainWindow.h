#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QComboBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QTabWidget>
#include <QTextEdit>
#include <memory>
#include "db/repositories/ProjectRepository.h"

namespace codex::api {
class ElevenLabsClient;
class EdgeTTSClient;
class VeoClient;
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
class InfoDockWidget;

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
    void onGenerateVideoFromPreview(const QString& passage);
    void onGeneratePlateFromPreview(const QString& passage, int cols, int rows);
    void onPlateSizeChanged(int cols, int rows);
    void onVideoGenerated(const QByteArray& videoData, const QString& prompt);
    void onVideoProgress(int percent);
    void onVideoError(const QString& error);

    void onPipelineStateChanged(codex::core::PipelineState state, const QString& message);
    void onPipelineProgress(int percent, const QString& step);
    void onPipelineCompleted(const QPixmap& image, const QString& prompt);
    void onPipelineFailed(const QString& error);
    void onSaveImage();

    void onAudioGenerated(const QByteArray& audioData, int durationMs);
    void onAudioError(const QString& error);
    void onEdgeAudioGenerated(const QByteArray& audioData, int durationMs);
    void onEdgeAudioError(const QString& error);

    void onStartSlideshow();
    void onGenerateAllAndSlideshow();
    void onFullGenerationCompleted();

private:
    void setupUi();
    void setupMenus();
    void setupConnections();
    void loadCodexAndRefreshUI(const QString& filePath);

    TreatiseListWidget* m_treatiseList = nullptr;
    TextViewerWidget* m_textViewer = nullptr;
    ImageViewerWidget* m_imageViewer = nullptr;
    PassagePreviewWidget* m_passagePreview = nullptr;
    AudioPlayerWidget* m_audioPlayer = nullptr;

    codex::core::TextParser* m_textParser = nullptr;
    codex::core::PipelineController* m_pipelineController = nullptr;
    codex::api::ElevenLabsClient* m_elevenLabsClient = nullptr;
    codex::api::EdgeTTSClient* m_edgeTTSClient = nullptr;
    codex::api::VeoClient* m_veoClient = nullptr;
    SlideshowWidget* m_slideshowWidget = nullptr;
    InfoDockWidget* m_infoDock = nullptr;

    // Project management
    codex::db::Project m_currentProject;
    QTimer* m_autoSaveTimer = nullptr;
    bool m_projectModified = false;

    QString m_selectedPassage;
    QString m_currentTreatiseCode;
    QString m_currentCategory;

    // Plate generation state
    QStringList m_plateTextSegments;
    int m_plateNextIndex = 0;
    int m_plateCols = 0;
    int m_plateRows = 0;
    bool m_plateGenerating = false;

    // Quick settings in toolbar
    QComboBox* m_voiceCombo = nullptr;
    QLineEdit* m_outputFolderEdit = nullptr;
    QLineEdit* m_videoFolderEdit = nullptr;

    // Central tab widget for passages/prompts
    QTabWidget* m_centerTabWidget = nullptr;
    QTextEdit* m_promptEdit = nullptr;

    // Progress bar for generation
    QProgressBar* m_progressBar = nullptr;

    // Full generation state (prompt + image + audio)
    bool m_fullGenerating = false;
    int m_fullGenStep = 0;  // 0=prompt, 1=image, 2=audio, 3=done
    QString m_generatedPrompt;
    QString m_lastAudioPath;

    void updateWindowTitle();
    void onVoiceChanged(int index);
    void onTestVoice();
    void onBrowseOutputFolder();
    void onBrowseVideoFolder();
    void onOpenImagesFolder();
    void onOpenVideosFolder();
    void onOpenAudioFolder();
    void setProjectModified(bool modified);
    void loadProject(const codex::db::Project& project);
    void showRecentProjectsOnStartup();

    void generateNextPlateImage();
    QStringList splitTextForPlate(const QString& text, int count);
};

} // namespace codex::ui
