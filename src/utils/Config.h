#pragma once

#include <QString>
#include <QJsonObject>
#include <memory>

namespace codex::utils {

class Config {
public:
    static Config& instance();

    bool load();
    bool save();

    // API settings
    QString claudeModel() const;
    QString imagenModel() const;
    QString elevenLabsVoiceId() const;

    // Paths
    QString codexFilePath() const;
    QString outputImagesPath() const;
    QString outputAudioPath() const;

    // Setters
    void setCodexFilePath(const QString& path);
    void setElevenLabsVoiceId(const QString& voiceId);

private:
    Config();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    QString configFilePath() const;

    QJsonObject m_config;
    bool m_loaded = false;
};

} // namespace codex::utils
