#include "Config.h"

#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>

namespace codex::utils {

Config& Config::instance() {
    static Config instance;
    return instance;
}

Config::Config() {
    load();
}

QString Config::configFilePath() const {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    return appData + "/config.json";
}

bool Config::load() {
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        // Create default config
        m_config = QJsonObject{
            {"apis", QJsonObject{
                {"llm_provider", "gemini"},  // "claude" or "gemini"
                {"claude", QJsonObject{
                    {"model", "claude-sonnet-4-20250514"},
                    {"max_tokens", 1000}
                }},
                {"gemini", QJsonObject{
                    {"model", "gemini-3-pro-preview"}
                }},
                {"imagen", QJsonObject{
                    {"model", "imagen-3.0-generate-001"},
                    {"aspect_ratio", "16:9"}
                }},
                {"veo", QJsonObject{
                    {"model", "veo-2.0-generate-001"},
                    {"duration_seconds", 5}
                }},
                {"elevenlabs", QJsonObject{
                    {"model_id", "eleven_multilingual_v2"},
                    {"voice_id", ""},
                    {"speed", 0.85}
                }},
                {"tts_provider", "edge"},  // "edge" or "elevenlabs"
                {"edge_tts", QJsonObject{
                    {"voice_id", "fr-FR-HenriNeural"}
                }},
                {"google_ai_provider", "vertex"},      // "aistudio" or "vertex" (for images/videos)
                {"llm_google_provider", "aistudio"},   // "aistudio" or "vertex" (for Gemini LLM)
                {"vertex", QJsonObject{
                    {"project_id", ""},
                    {"region", "us-central1"},
                    {"service_account_path", ""}
                }}
            }},
            {"paths", QJsonObject{
                {"codex_file", ""},
                {"output_images", "./images"},
                {"output_audio", "./images"},
                {"output_videos", "./videos"}
            }},
            {"appearance", QJsonObject{
                {"theme", "dark"},
                {"accent_color", "#094771"},
                {"font_family", "Segoe UI"},
                {"font_size", 10},
                {"text_font_family", "Consolas"},
                {"text_font_size", 11}
            }}
        };
        m_loaded = true;
        return save();
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    m_config = doc.object();
    m_loaded = true;
    return true;
}

bool Config::save() {
    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(m_config);
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

QVariant Config::value(const QString& path, const QVariant& defaultValue) const {
    QStringList parts = path.split('/');
    if (parts.isEmpty()) {
        return defaultValue;
    }

    QJsonValue current = m_config;
    for (const QString& part : parts) {
        if (!current.isObject()) {
            return defaultValue;
        }
        current = current.toObject()[part];
    }

    if (current.isUndefined() || current.isNull()) {
        return defaultValue;
    }

    return current.toVariant();
}

void Config::setValue(const QString& path, const QVariant& value) {
    QStringList parts = path.split('/');
    if (parts.isEmpty()) {
        return;
    }

    // Navigate to parent and set the value
    if (parts.size() == 1) {
        m_config[parts[0]] = QJsonValue::fromVariant(value);
    } else if (parts.size() == 2) {
        QJsonObject section = m_config[parts[0]].toObject();
        section[parts[1]] = QJsonValue::fromVariant(value);
        m_config[parts[0]] = section;
    } else if (parts.size() == 3) {
        QJsonObject section = m_config[parts[0]].toObject();
        QJsonObject subsection = section[parts[1]].toObject();
        subsection[parts[2]] = QJsonValue::fromVariant(value);
        section[parts[1]] = subsection;
        m_config[parts[0]] = section;
    }
}

QString Config::claudeModel() const {
    return m_config["apis"].toObject()["claude"].toObject()["model"].toString();
}

QString Config::imagenModel() const {
    return m_config["apis"].toObject()["imagen"].toObject()["model"].toString();
}

QString Config::elevenLabsVoiceId() const {
    return m_config["apis"].toObject()["elevenlabs"].toObject()["voice_id"].toString();
}

QString Config::codexFilePath() const {
    return m_config["paths"].toObject()["codex_file"].toString();
}

QString Config::outputImagesPath() const {
    return m_config["paths"].toObject()["output_images"].toString();
}

QString Config::outputAudioPath() const {
    return m_config["paths"].toObject()["output_audio"].toString();
}

QString Config::outputVideosPath() const {
    return m_config["paths"].toObject()["output_videos"].toString("./output/videos");
}

void Config::setCodexFilePath(const QString& path) {
    QJsonObject paths = m_config["paths"].toObject();
    paths["codex_file"] = path;
    m_config["paths"] = paths;
    save();
}

void Config::setElevenLabsVoiceId(const QString& voiceId) {
    QJsonObject apis = m_config["apis"].toObject();
    QJsonObject elevenlabs = apis["elevenlabs"].toObject();
    elevenlabs["voice_id"] = voiceId;
    apis["elevenlabs"] = elevenlabs;
    m_config["apis"] = apis;
    save();
}

QString Config::llmProvider() const {
    return m_config["apis"].toObject()["llm_provider"].toString("claude");
}

void Config::setLlmProvider(const QString& provider) {
    QJsonObject apis = m_config["apis"].toObject();
    apis["llm_provider"] = provider;
    m_config["apis"] = apis;
    save();
}

void Config::setOutputImagesPath(const QString& path) {
    QJsonObject paths = m_config["paths"].toObject();
    paths["output_images"] = path;
    m_config["paths"] = paths;
    save();
}

void Config::setOutputVideosPath(const QString& path) {
    QJsonObject paths = m_config["paths"].toObject();
    paths["output_videos"] = path;
    m_config["paths"] = paths;
    save();
}

QString Config::ttsProvider() const {
    return m_config["apis"].toObject()["tts_provider"].toString("edge");
}

void Config::setTtsProvider(const QString& provider) {
    QJsonObject apis = m_config["apis"].toObject();
    apis["tts_provider"] = provider;
    m_config["apis"] = apis;
    save();
}

QString Config::edgeTtsVoice() const {
    return m_config["apis"].toObject()["edge_tts"].toObject()["voice_id"].toString("fr-FR-HenriNeural");
}

void Config::setEdgeTtsVoice(const QString& voiceId) {
    QJsonObject apis = m_config["apis"].toObject();
    QJsonObject edgeTts = apis["edge_tts"].toObject();
    edgeTts["voice_id"] = voiceId;
    apis["edge_tts"] = edgeTts;
    m_config["apis"] = apis;
    save();
}

QString Config::googleAiProvider() const {
    return m_config["apis"].toObject()["google_ai_provider"].toString("vertex");
}

void Config::setGoogleAiProvider(const QString& provider) {
    QJsonObject apis = m_config["apis"].toObject();
    apis["google_ai_provider"] = provider;
    m_config["apis"] = apis;
    save();
}

QString Config::llmGoogleProvider() const {
    return m_config["apis"].toObject()["llm_google_provider"].toString("aistudio");
}

void Config::setLlmGoogleProvider(const QString& provider) {
    QJsonObject apis = m_config["apis"].toObject();
    apis["llm_google_provider"] = provider;
    m_config["apis"] = apis;
    save();
}

QString Config::geminiModel() const {
    return m_config["apis"].toObject()["gemini"].toObject()["model"].toString("gemini-3-pro-preview");
}

void Config::setGeminiModel(const QString& model) {
    QJsonObject apis = m_config["apis"].toObject();
    QJsonObject gemini = apis["gemini"].toObject();
    gemini["model"] = model;
    apis["gemini"] = gemini;
    m_config["apis"] = apis;
    save();
}

QString Config::vertexProjectId() const {
    return m_config["apis"].toObject()["vertex"].toObject()["project_id"].toString();
}

void Config::setVertexProjectId(const QString& projectId) {
    QJsonObject apis = m_config["apis"].toObject();
    QJsonObject vertex = apis["vertex"].toObject();
    vertex["project_id"] = projectId;
    apis["vertex"] = vertex;
    m_config["apis"] = apis;
    save();
}

QString Config::vertexRegion() const {
    return m_config["apis"].toObject()["vertex"].toObject()["region"].toString("us-central1");
}

void Config::setVertexRegion(const QString& region) {
    QJsonObject apis = m_config["apis"].toObject();
    QJsonObject vertex = apis["vertex"].toObject();
    vertex["region"] = region;
    apis["vertex"] = vertex;
    m_config["apis"] = apis;
    save();
}

QString Config::vertexServiceAccountPath() const {
    return m_config["apis"].toObject()["vertex"].toObject()["service_account_path"].toString();
}

void Config::setVertexServiceAccountPath(const QString& path) {
    QJsonObject apis = m_config["apis"].toObject();
    QJsonObject vertex = apis["vertex"].toObject();
    vertex["service_account_path"] = path;
    apis["vertex"] = vertex;
    m_config["apis"] = apis;
    save();
}

} // namespace codex::utils
