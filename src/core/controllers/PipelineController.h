#pragma once

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QJsonObject>

namespace codex::api {
class ClaudeClient;
class ImagenClient;
}

namespace codex::core {

class TextParser;
class PromptBuilder;
class MythicClassifier;
enum class MythicCategory;

// Pipeline state machine states
enum class PipelineState {
    Idle,
    AnalyzingText,
    EnrichingWithClaude,
    GeneratingImage,
    Completed,
    Failed,
    Cancelled
};

// Pipeline execution result
struct PipelineResult {
    bool success = false;
    QString errorMessage;
    QPixmap generatedImage;
    QString enrichedPrompt;
    QString imagenPrompt;
    QJsonObject claudeResponse;
};

class PipelineController : public QObject {
    Q_OBJECT

public:
    explicit PipelineController(QObject* parent = nullptr);
    ~PipelineController();

    // Start the generation pipeline
    void startGeneration(const QString& passageText,
                         const QString& treatiseCode = QString(),
                         const QString& category = QString());

    // Cancel ongoing generation
    void cancel();

    // Get current state
    PipelineState state() const { return m_state; }
    bool isRunning() const;

    // Get last result
    PipelineResult lastResult() const { return m_lastResult; }

signals:
    void stateChanged(PipelineState state, const QString& message);
    void progressUpdated(int percent, const QString& step);
    void generationCompleted(const QPixmap& image, const QString& prompt);
    void generationFailed(const QString& error);

private slots:
    void onClaudeEnrichmentCompleted(const QJsonObject& response);
    void onClaudeError(const QString& error);
    void onImagenImageGenerated(const QPixmap& image, const QString& prompt);
    void onImagenError(const QString& error);
    void onImagenProgress(int percent);

private:
    void setState(PipelineState state, const QString& message = QString());
    void analyzePassage();
    void enrichWithClaude();
    void generateImage();
    void finishWithError(const QString& error);
    void finishWithSuccess(const QPixmap& image);

    // Components
    codex::api::ClaudeClient* m_claudeClient = nullptr;
    codex::api::ImagenClient* m_imagenClient = nullptr;
    TextParser* m_textParser = nullptr;
    PromptBuilder* m_promptBuilder = nullptr;
    MythicClassifier* m_mythicClassifier = nullptr;

    // State
    PipelineState m_state = PipelineState::Idle;
    PipelineResult m_lastResult;
    bool m_cancelled = false;

    // Current generation context
    QString m_currentPassage;
    QString m_currentTreatiseCode;
    QString m_currentCategory;
    QStringList m_detectedEntities;
    QString m_enrichedScene;
    QString m_enrichedEmotion;
    QStringList m_visualKeywords;
};

} // namespace codex::core
