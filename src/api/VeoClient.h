#pragma once

#include "ApiClient.h"
#include <QByteArray>

namespace codex::api {

struct VideoGenerationParams {
    QString prompt;
    QString aspectRatio = "16:9";
    int durationSeconds = 8;  // 5-8 seconds
    bool generateAudio = true;  // Veo 3.1 supports native audio
    QByteArray referenceImage;  // Optional: for image-to-video
    QString referenceImageMimeType = "image/png";
};

class VeoClient : public ApiClient {
    Q_OBJECT

public:
    explicit VeoClient(QObject* parent = nullptr);

    void generateVideo(const VideoGenerationParams& params);

    // Simple API key check (inherited from ApiClient)
    bool isConfigured() const override;

    // Model selection
    void setModel(const QString& model);
    QString model() const { return m_model; }

    // Available models
    static QString modelStandard() { return "veo-3.1-generate-preview"; }
    static QString modelFast() { return "veo-3.1-fast-generate-preview"; }
    static QString modelVeo3() { return "veo-3.0-generate-preview"; }

signals:
    void videoGenerated(const QByteArray& videoData, const QString& prompt);
    void generationProgress(int percent);
    void operationPending(const QString& operationId);

private slots:
    void onGenerateReplyFinished(QNetworkReply* reply, const QString& originalPrompt);
    void onPollReplyFinished(QNetworkReply* reply, const QString& originalPrompt);

private:
    void pollOperation(const QString& operationName, const QString& originalPrompt);

    QString m_model = "veo-3.1-generate-preview";
    int m_pollIntervalMs = 5000;
};

} // namespace codex::api
