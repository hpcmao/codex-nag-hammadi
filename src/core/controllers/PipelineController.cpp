#include "PipelineController.h"
#include "api/ClaudeClient.h"
#include "api/GeminiClient.h"
#include "api/ImagenClient.h"
#include "core/services/TextParser.h"
#include "core/services/PromptBuilder.h"
#include "core/services/MythicClassifier.h"
#include "utils/SecureStorage.h"
#include "utils/Config.h"
#include "utils/Logger.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace codex::core {

PipelineController::PipelineController(QObject* parent)
    : QObject(parent)
{
    // Create API clients
    m_claudeClient = new codex::api::ClaudeClient(this);
    m_geminiClient = new codex::api::GeminiClient(this);
    m_imagenClient = new codex::api::ImagenClient(this);
    m_textParser = new TextParser();
    m_promptBuilder = new PromptBuilder();
    m_mythicClassifier = new MythicClassifier();

    // Load API keys and configure providers
    auto& storage = codex::utils::SecureStorage::instance();
    auto& config = codex::utils::Config::instance();

    m_claudeClient->setApiKey(storage.getApiKey(storage.SERVICE_CLAUDE));

    // Configure Gemini (for prompt enrichment) - uses AI Studio (separate key)
    m_geminiClient->setProvider(codex::api::GoogleAIProvider::AIStudio);
    m_geminiClient->setApiKey(storage.getApiKey(storage.SERVICE_AISTUDIO));  // AI Studio key (free tier)
    m_geminiClient->setModel(config.geminiModel());
    LOG_INFO("Gemini LLM: Using AI Studio endpoint (free tier)");
    LOG_INFO(QString("Gemini model: %1").arg(config.geminiModel()));

    // Configure Google AI provider for images/videos (Vertex AI by default)
    QString googleProvider = config.googleAiProvider();
    if (googleProvider == "vertex") {
        m_imagenClient->setProvider(codex::api::GoogleAIProvider::VertexAI);
        LOG_INFO("Imagen: Using Vertex AI endpoint");
    } else {
        m_imagenClient->setProvider(codex::api::GoogleAIProvider::AIStudio);
        LOG_INFO("Imagen: Using AI Studio endpoint");
    }
    m_imagenClient->setApiKey(storage.getApiKey(storage.SERVICE_IMAGEN));

    // Connect Claude signals (fallback)
    connect(m_claudeClient, &codex::api::ClaudeClient::enrichmentCompleted,
            this, &PipelineController::onClaudeEnrichmentCompleted);
    connect(m_claudeClient, &codex::api::ClaudeClient::errorOccurred,
            this, &PipelineController::onClaudeError);

    // Connect Gemini signals
    connect(m_geminiClient, &codex::api::GeminiClient::enrichmentCompleted,
            this, &PipelineController::onGeminiEnrichmentCompleted);
    connect(m_geminiClient, &codex::api::GeminiClient::errorOccurred,
            this, &PipelineController::onGeminiError);

    // Connect Imagen signals
    connect(m_imagenClient, &codex::api::ImagenClient::imageGenerated,
            this, &PipelineController::onImagenImageGenerated);
    connect(m_imagenClient, &codex::api::ImagenClient::errorOccurred,
            this, &PipelineController::onImagenError);
    connect(m_imagenClient, &codex::api::ImagenClient::generationProgress,
            this, &PipelineController::onImagenProgress);

    LOG_INFO("PipelineController initialized");
}

PipelineController::~PipelineController() {
    delete m_textParser;
    delete m_promptBuilder;
    delete m_mythicClassifier;
}

bool PipelineController::isRunning() const {
    return m_state != PipelineState::Idle &&
           m_state != PipelineState::Completed &&
           m_state != PipelineState::Failed &&
           m_state != PipelineState::Cancelled;
}

void PipelineController::startGeneration(const QString& passageText,
                                          const QString& treatiseCode,
                                          const QString& category) {
    if (isRunning()) {
        LOG_WARN("Pipeline already running, ignoring start request");
        return;
    }

    // Reset state
    m_cancelled = false;
    m_lastResult = PipelineResult();
    m_currentPassage = passageText;
    m_currentTreatiseCode = treatiseCode;

    // Auto-classify if no category provided
    if (category.isEmpty() && !treatiseCode.isEmpty()) {
        MythicCategory mythicCat = m_mythicClassifier->classifyTreatise(treatiseCode);
        m_currentCategory = m_mythicClassifier->categoryName(mythicCat);
    } else {
        m_currentCategory = category.isEmpty() ? "Gnose" : category;
    }

    LOG_INFO(QString("Starting pipeline for passage: %1 chars, treatise: %2, category: %3")
             .arg(passageText.length()).arg(treatiseCode).arg(m_currentCategory));

    // Step 1: Analyze passage
    setState(PipelineState::AnalyzingText, "Analyse du passage...");
    emit progressUpdated(5, "Analyse du texte");

    analyzePassage();
}

void PipelineController::cancel() {
    if (!isRunning()) return;

    m_cancelled = true;
    setState(PipelineState::Cancelled, "Generation annulee");
    emit progressUpdated(0, "Annule");

    LOG_INFO("Pipeline cancelled by user");
}

void PipelineController::setState(PipelineState state, const QString& message) {
    m_state = state;
    emit stateChanged(state, message);

    QString stateStr;
    switch (state) {
        case PipelineState::Idle: stateStr = "Idle"; break;
        case PipelineState::AnalyzingText: stateStr = "AnalyzingText"; break;
        case PipelineState::EnrichingWithClaude: stateStr = "EnrichingWithClaude"; break;
        case PipelineState::GeneratingImage: stateStr = "GeneratingImage"; break;
        case PipelineState::Completed: stateStr = "Completed"; break;
        case PipelineState::Failed: stateStr = "Failed"; break;
        case PipelineState::Cancelled: stateStr = "Cancelled"; break;
    }
    LOG_INFO(QString("Pipeline state: %1 - %2").arg(stateStr, message));
}

void PipelineController::analyzePassage() {
    if (m_cancelled) return;

    // Detect gnostic entities in the passage
    m_detectedEntities = m_textParser->detectGnosticEntities(m_currentPassage);

    LOG_INFO(QString("Detected entities: %1").arg(m_detectedEntities.join(", ")));

    emit progressUpdated(10, "Entites detectees");

    // Step 2: Enrich with Claude
    enrichWithClaude();
}

void PipelineController::enrichWithClaude() {
    if (m_cancelled) return;

    auto& config = codex::utils::Config::instance();
    QString llmProvider = config.llmProvider();

    // Use Gemini if configured (default)
    if (llmProvider == "gemini") {
        if (!m_geminiClient->isConfigured()) {
            LOG_WARN("Gemini API not configured, using direct Imagen prompt");
            m_enrichedScene = m_currentPassage.left(500);
            m_enrichedEmotion = "mystique";
            m_visualKeywords = m_detectedEntities;
            emit progressUpdated(50, "Enrichissement ignore (pas de cle Gemini)");
            generateImage();
            return;
        }

        setState(PipelineState::EnrichingWithClaude, "Enrichissement avec Gemini 3 Pro...");
        emit progressUpdated(20, "Appel Gemini API");

        QString geminiPrompt = m_promptBuilder->buildClaudePrompt(
            m_currentPassage,
            m_detectedEntities,
            m_currentCategory
        );

        m_geminiClient->enrichPassage(geminiPrompt);
        return;
    }

    // Fallback to Claude
    if (!m_claudeClient->isConfigured()) {
        LOG_WARN("Claude API not configured, using direct Imagen prompt");
        m_enrichedScene = m_currentPassage.left(500);
        m_enrichedEmotion = "mystique";
        m_visualKeywords = m_detectedEntities;

        emit progressUpdated(50, "Enrichissement ignore (pas de cle Claude)");
        generateImage();
        return;
    }

    setState(PipelineState::EnrichingWithClaude, "Enrichissement avec Claude...");
    emit progressUpdated(20, "Appel Claude API");

    QString claudePrompt = m_promptBuilder->buildClaudePrompt(
        m_currentPassage,
        m_detectedEntities,
        m_currentCategory
    );

    m_claudeClient->enrichPassage(claudePrompt);
}

void PipelineController::onClaudeEnrichmentCompleted(const QJsonObject& response) {
    if (m_cancelled) return;

    LOG_INFO("Claude enrichment completed");
    m_lastResult.claudeResponse = response;

    // Parse Claude response
    m_enrichedScene = response["scene"].toString();
    if (m_enrichedScene.isEmpty()) {
        m_enrichedScene = response["text"].toString();
    }

    m_enrichedEmotion = response["emotion"].toString();
    if (m_enrichedEmotion.isEmpty()) {
        m_enrichedEmotion = "mystique";
    }

    // Get visual elements
    QJsonArray visualElements = response["visual_elements"].toArray();
    m_visualKeywords.clear();
    for (const auto& elem : visualElements) {
        m_visualKeywords.append(elem.toString());
    }
    if (m_visualKeywords.isEmpty()) {
        m_visualKeywords = m_detectedEntities;
    }

    emit progressUpdated(50, "Enrichissement termine");

    // Step 3: Generate image
    generateImage();
}

void PipelineController::onClaudeError(const QString& error) {
    if (m_cancelled) return;

    LOG_WARN(QString("Claude error: %1 - falling back to direct generation").arg(error));

    // Fallback: use passage directly
    m_enrichedScene = m_currentPassage.left(500);
    m_enrichedEmotion = "mystique";
    m_visualKeywords = m_detectedEntities;

    emit progressUpdated(50, "Enrichissement echoue, generation directe");
    generateImage();
}

void PipelineController::onGeminiEnrichmentCompleted(const QJsonObject& response) {
    if (m_cancelled) return;

    LOG_INFO("Gemini enrichment completed");
    m_lastResult.claudeResponse = response;  // Reuse same field for response

    // Parse Gemini response - it returns text directly
    QString text = response["text"].toString();
    if (!text.isEmpty()) {
        // Try to parse JSON from text if it looks like JSON
        if (text.contains("{") && text.contains("}")) {
            int start = text.indexOf("{");
            int end = text.lastIndexOf("}") + 1;
            QString jsonStr = text.mid(start, end - start);
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (!doc.isNull()) {
                QJsonObject parsed = doc.object();
                m_enrichedScene = parsed["scene"].toString();
                m_enrichedEmotion = parsed["emotion"].toString();
                QJsonArray visualElements = parsed["visual_elements"].toArray();
                m_visualKeywords.clear();
                for (const auto& elem : visualElements) {
                    m_visualKeywords.append(elem.toString());
                }
            }
        }

        // If parsing failed or no structured data, use text directly
        if (m_enrichedScene.isEmpty()) {
            m_enrichedScene = text.left(1000);
        }
        if (m_enrichedEmotion.isEmpty()) {
            m_enrichedEmotion = "mystique";
        }
        if (m_visualKeywords.isEmpty()) {
            m_visualKeywords = m_detectedEntities;
        }
    } else {
        m_enrichedScene = m_currentPassage.left(500);
        m_enrichedEmotion = "mystique";
        m_visualKeywords = m_detectedEntities;
    }

    emit progressUpdated(50, "Enrichissement Gemini termine");

    // Step 3: Generate image
    generateImage();
}

void PipelineController::onGeminiError(const QString& error) {
    if (m_cancelled) return;

    LOG_WARN(QString("Gemini error: %1 - falling back to direct generation").arg(error));

    // Fallback: use passage directly
    m_enrichedScene = m_currentPassage.left(500);
    m_enrichedEmotion = "mystique";
    m_visualKeywords = m_detectedEntities;

    emit progressUpdated(50, "Enrichissement Gemini echoue, generation directe");
    generateImage();
}

void PipelineController::generateImage() {
    if (m_cancelled) return;

    // Check if Imagen is configured
    if (!m_imagenClient->isConfigured()) {
        finishWithError("Cle API Imagen non configuree. Veuillez configurer votre cle dans les parametres.");
        return;
    }

    setState(PipelineState::GeneratingImage, "Generation de l'image...");
    emit progressUpdated(60, "Appel Imagen API");

    // Get category style from MythicClassifier
    MythicCategory mythicCat = m_mythicClassifier->classifyTreatise(m_currentTreatiseCode);
    CategoryStyle style = m_mythicClassifier->getStyleForCategory(mythicCat);

    // Build palette from category or use default
    QStringList palette;
    if (!style.palette.isEmpty()) {
        for (const QString& color : style.palette) {
            palette.append(color);
        }
    } else {
        palette = {"desaturated", "amber", "teal", "shadow"};
    }

    // Merge visual keywords from category with detected entities
    QStringList allVisualKeywords = m_visualKeywords;
    for (const QString& kw : style.visualKeywords) {
        if (!allVisualKeywords.contains(kw)) {
            allVisualKeywords.append(kw);
        }
    }

    // Add lighting style to emotion/mood
    QString enrichedEmotion = m_enrichedEmotion;
    if (!style.lightingStyle.isEmpty()) {
        enrichedEmotion += ", " + style.lightingStyle;
    }

    QString imagenPrompt = m_promptBuilder->buildImagenPrompt(
        m_enrichedScene,
        enrichedEmotion,
        allVisualKeywords,
        palette
    );

    m_lastResult.imagenPrompt = imagenPrompt;
    LOG_INFO(QString("Imagen prompt (category: %1): %2").arg(style.name).arg(imagenPrompt.left(200)));

    // Generate image
    codex::api::ImageGenerationParams params;
    params.prompt = imagenPrompt;
    params.aspectRatio = "16:9";
    params.numberOfImages = 1;

    m_imagenClient->generateImage(params);
}

void PipelineController::onImagenProgress(int percent) {
    if (m_cancelled) return;

    // Map Imagen progress (0-100) to overall progress (60-95)
    int overallProgress = 60 + (percent * 35 / 100);
    emit progressUpdated(overallProgress, QString("Generation: %1%").arg(percent));
}

void PipelineController::onImagenImageGenerated(const QPixmap& image, const QString& prompt) {
    if (m_cancelled) return;

    LOG_INFO(QString("Image generated: %1x%2").arg(image.width()).arg(image.height()));
    m_lastResult.enrichedPrompt = prompt;

    finishWithSuccess(image);
}

void PipelineController::onImagenError(const QString& error) {
    if (m_cancelled) return;

    finishWithError(QString("Erreur Imagen: %1").arg(error));
}

void PipelineController::finishWithError(const QString& error) {
    m_lastResult.success = false;
    m_lastResult.errorMessage = error;

    setState(PipelineState::Failed, error);
    emit progressUpdated(0, "Echec");
    emit generationFailed(error);

    LOG_ERROR(QString("Pipeline failed: %1").arg(error));
}

void PipelineController::finishWithSuccess(const QPixmap& image) {
    m_lastResult.success = true;
    m_lastResult.generatedImage = image;

    setState(PipelineState::Completed, "Generation terminee");
    emit progressUpdated(100, "Termine");
    emit generationCompleted(image, m_lastResult.imagenPrompt);

    LOG_INFO("Pipeline completed successfully");
}

} // namespace codex::core
