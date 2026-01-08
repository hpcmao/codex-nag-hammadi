#pragma once

#include "ApiClient.h"
#include <QByteArray>

namespace codex::api {

struct VideoGenerationParams {
    QString prompt;
    QString aspectRatio = "16:9";
    int durationSeconds = 5;  // 5-8 seconds for Veo 2
};

class VeoClient : public ApiClient {
    Q_OBJECT

public:
    explicit VeoClient(QObject* parent = nullptr);

    void generateVideo(const VideoGenerationParams& params);

signals:
    void videoGenerated(const QByteArray& videoData, const QString& prompt);
    void generationProgress(int percent);
    void operationPending(const QString& operationId);

private slots:
    void onGenerateReplyFinished(QNetworkReply* reply, const QString& originalPrompt);
    void onPollReplyFinished(QNetworkReply* reply, const QString& originalPrompt);

private:
    void pollOperation(const QString& operationName, const QString& originalPrompt);

    QString m_model = "veo-2.0-generate-001";
    int m_pollIntervalMs = 5000;
};

} // namespace codex::api
