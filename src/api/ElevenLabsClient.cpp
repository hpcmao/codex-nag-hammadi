#include "ElevenLabsClient.h"
#include "utils/Logger.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace codex::api {

ElevenLabsClient::ElevenLabsClient(QObject* parent)
    : ApiClient(parent)
{
    m_baseUrl = "https://api.elevenlabs.io/v1";
}

void ElevenLabsClient::generateSpeech(const QString& text, const VoiceSettings& settings) {
    if (!isConfigured()) {
        emit errorOccurred("ElevenLabs API key not configured");
        return;
    }

    if (settings.voiceId.isEmpty()) {
        emit errorOccurred("Voice ID not configured");
        return;
    }

    emit requestStarted();

    QString endpoint = QString("/text-to-speech/%1").arg(settings.voiceId);
    QNetworkRequest request = createRequest(endpoint);
    request.setRawHeader("xi-api-key", m_apiKey.toUtf8());

    QJsonObject body;
    body["text"] = text;
    body["model_id"] = m_modelId;

    QJsonObject voiceSettings;
    voiceSettings["stability"] = settings.stability;
    voiceSettings["similarity_boost"] = settings.similarityBoost;
    voiceSettings["speed"] = settings.speed;
    body["voice_settings"] = voiceSettings;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSpeechReplyFinished(reply);
    });
}

void ElevenLabsClient::fetchAvailableVoices() {
    if (!isConfigured()) {
        emit errorOccurred("ElevenLabs API key not configured");
        return;
    }

    emit requestStarted();

    QNetworkRequest request = createRequest("/voices");
    request.setRawHeader("xi-api-key", m_apiKey.toUtf8());

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onVoicesReplyFinished(reply);
    });
}

void ElevenLabsClient::onSpeechReplyFinished(QNetworkReply* reply) {
    emit requestFinished();

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        reply->deleteLater();
        return;
    }

    QByteArray audioData = reply->readAll();

    // Estimate duration (rough: ~150 words per minute for slow narration)
    // This is a placeholder - actual duration would come from audio metadata
    int estimatedDurationMs = audioData.size() / 32; // Very rough estimate

    emit speechGenerated(audioData, estimatedDurationMs);
    reply->deleteLater();
}

void ElevenLabsClient::onVoicesReplyFinished(QNetworkReply* reply) {
    emit requestFinished();

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray voices = doc.object()["voices"].toArray();

    emit voicesListReceived(voices);
    reply->deleteLater();
}

} // namespace codex::api
