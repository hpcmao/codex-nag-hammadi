#pragma once

#include <QString>
#include <QJsonObject>
#include <QVariant>
#include <memory>

namespace codex::utils {

class Config {
public:
    static Config& instance();

    bool load();
    bool save();

    // Generic value access (path format: "section/key")
    QVariant value(const QString& path, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& path, const QVariant& value);

    // API settings
    QString claudeModel() const;
    QString imagenModel() const;
    QString elevenLabsVoiceId() const;
    QString llmProvider() const;      // "claude" or "gemini"
    QString ttsProvider() const;      // "edge" or "elevenlabs"
    QString edgeTtsVoice() const;     // Edge TTS voice ID

    // Google AI provider settings
    QString googleAiProvider() const;       // "aistudio" or "vertex" (for images/videos)
    QString llmGoogleProvider() const;      // "aistudio" or "vertex" (for Gemini LLM)
    QString geminiModel() const;            // "gemini-3-pro-preview" etc.
    QString vertexProjectId() const;
    QString vertexRegion() const;
    QString vertexServiceAccountPath() const;

    // Paths
    QString codexFilePath() const;
    QString outputImagesPath() const;
    QString outputAudioPath() const;
    QString outputVideosPath() const;

    // Setters
    void setCodexFilePath(const QString& path);
    void setOutputImagesPath(const QString& path);
    void setOutputVideosPath(const QString& path);
    void setElevenLabsVoiceId(const QString& voiceId);
    void setLlmProvider(const QString& provider);
    void setTtsProvider(const QString& provider);
    void setEdgeTtsVoice(const QString& voiceId);
    void setGoogleAiProvider(const QString& provider);
    void setLlmGoogleProvider(const QString& provider);
    void setGeminiModel(const QString& model);
    void setVertexProjectId(const QString& projectId);
    void setVertexRegion(const QString& region);
    void setVertexServiceAccountPath(const QString& path);

private:
    Config();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    QString configFilePath() const;

    QJsonObject m_config;
    bool m_loaded = false;
};

} // namespace codex::utils
