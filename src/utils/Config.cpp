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
                {"llm_provider", "claude"},  // "claude" or "gemini"
                {"claude", QJsonObject{
                    {"model", "claude-sonnet-4-20250514"},
                    {"max_tokens", 1000}
                }},
                {"gemini", QJsonObject{
                    {"model", "gemini-2.0-flash"}
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
                }}
            }},
            {"paths", QJsonObject{
                {"codex_file", ""},
                {"output_images", "./output/images"},
                {"output_audio", "./output/audio"}
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

} // namespace codex::utils
