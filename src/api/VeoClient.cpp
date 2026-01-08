#include "VeoClient.h"
#include "utils/Logger.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

namespace codex::api {

VeoClient::VeoClient(QObject* parent)
    : ApiClient(parent)
{
    m_baseUrl = "https://generativelanguage.googleapis.com/v1beta/models";
}

void VeoClient::generateVideo(const VideoGenerationParams& params) {
    if (!isConfigured()) {
        emit errorOccurred("Veo API key not configured");
        return;
    }

    emit requestStarted();
    emit generationProgress(5);

    QString endpoint = QString("/%1:generateVideos?key=%2").arg(m_model, m_apiKey);
    QNetworkRequest request = createRequest(endpoint);

    QJsonObject body;
    body["prompt"] = params.prompt;

    QJsonObject videoConfig;
    videoConfig["aspectRatio"] = params.aspectRatio;
    videoConfig["durationSeconds"] = params.durationSeconds;
    body["videoGenerationConfig"] = videoConfig;

    LOG_INFO(QString("Starting video generation: %1").arg(params.prompt.left(100)));

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, params]() {
        onGenerateReplyFinished(reply, params.prompt);
    });
}

void VeoClient::onGenerateReplyFinished(QNetworkReply* reply, const QString& originalPrompt) {
    if (reply->error() != QNetworkReply::NoError) {
        emit requestFinished();
        handleNetworkError(reply);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject response = doc.object();

    // Veo returns an operation name for async processing
    QString operationName = response["name"].toString();

    if (!operationName.isEmpty()) {
        LOG_INFO(QString("Video generation started, operation: %1").arg(operationName));
        emit operationPending(operationName);
        emit generationProgress(10);

        // Start polling for result
        QTimer::singleShot(m_pollIntervalMs, this, [this, operationName, originalPrompt]() {
            pollOperation(operationName, originalPrompt);
        });
    } else {
        // Check if video is returned directly (for some configurations)
        QJsonArray videos = response["videos"].toArray();
        if (!videos.isEmpty()) {
            QString base64Data = videos[0].toObject()["video"].toString();
            QByteArray videoData = QByteArray::fromBase64(base64Data.toUtf8());
            emit requestFinished();
            emit generationProgress(100);
            emit videoGenerated(videoData, originalPrompt);
        } else {
            emit requestFinished();
            emit errorOccurred("No operation or video in response");
        }
    }

    reply->deleteLater();
}

void VeoClient::pollOperation(const QString& operationName, const QString& originalPrompt) {
    if (!isConfigured()) {
        emit requestFinished();
        emit errorOccurred("API key not configured");
        return;
    }

    // Poll the operation status
    QString endpoint = QString("/v1beta/%1?key=%2").arg(operationName, m_apiKey);

    QNetworkRequest request;
    request.setUrl(QUrl("https://generativelanguage.googleapis.com" + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, originalPrompt]() {
        onPollReplyFinished(reply, originalPrompt);
    });
}

void VeoClient::onPollReplyFinished(QNetworkReply* reply, const QString& originalPrompt) {
    if (reply->error() != QNetworkReply::NoError) {
        emit requestFinished();
        handleNetworkError(reply);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject response = doc.object();

    bool done = response["done"].toBool();
    QString operationName = response["name"].toString();

    if (done) {
        // Operation completed
        QJsonObject result = response["response"].toObject();
        QJsonArray videos = result["videos"].toArray();

        if (!videos.isEmpty()) {
            QString base64Data = videos[0].toObject()["video"].toString();
            QByteArray videoData = QByteArray::fromBase64(base64Data.toUtf8());

            emit requestFinished();
            emit generationProgress(100);
            emit videoGenerated(videoData, originalPrompt);
            LOG_INFO("Video generation completed successfully");
        } else {
            // Check for error
            QJsonObject error = response["error"].toObject();
            if (!error.isEmpty()) {
                QString errorMsg = error["message"].toString();
                emit requestFinished();
                emit errorOccurred(QString("Video generation failed: %1").arg(errorMsg));
            } else {
                emit requestFinished();
                emit errorOccurred("No video in response");
            }
        }
    } else {
        // Still processing, continue polling
        QJsonObject metadata = response["metadata"].toObject();
        int progress = metadata.contains("progress") ? metadata["progress"].toInt() : 20;
        emit generationProgress(qMin(progress, 95));

        LOG_INFO(QString("Video generation in progress: %1%").arg(progress));

        QTimer::singleShot(m_pollIntervalMs, this, [this, operationName, originalPrompt]() {
            pollOperation(operationName, originalPrompt);
        });
    }

    reply->deleteLater();
}

} // namespace codex::api
