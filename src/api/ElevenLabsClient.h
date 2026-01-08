#pragma once

#include "ApiClient.h"
#include <QByteArray>
#include <QJsonArray>

namespace codex::api {

struct VoiceSettings {
    QString voiceId;
    double stability = 0.5;
    double similarityBoost = 0.75;
    double speed = 0.85;
};

class ElevenLabsClient : public ApiClient {
    Q_OBJECT

public:
    explicit ElevenLabsClient(QObject* parent = nullptr);

    void generateSpeech(const QString& text, const VoiceSettings& settings);
    void fetchAvailableVoices();

signals:
    void speechGenerated(const QByteArray& audioData, int durationMs);
    void voicesListReceived(const QJsonArray& voices);

private slots:
    void onSpeechReplyFinished(QNetworkReply* reply);
    void onVoicesReplyFinished(QNetworkReply* reply);

private:
    QString m_modelId = "eleven_multilingual_v2";
};

} // namespace codex::api
