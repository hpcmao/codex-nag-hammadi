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

bool VeoClient::isConfigured() const {
    return !m_apiKey.isEmpty();
}

void VeoClient::setModel(const QString& model) {
    m_model = model;
}

void VeoClient::generateVideo(const VideoGenerationParams& params) {
    if (!isConfigured()) {
        emit errorOccurred("Veo requiert une cle API Gemini (Paid Tier).\n\n"
                          "1. Allez sur https://aistudio.google.com/apikey\n"
                          "2. Creez une cle API sur un projet avec Paid Tier active\n"
                          "3. Configurez la cle dans Parametres > API");
        return;
    }

    emit requestStarted();
    emit generationProgress(5);

    // Build URL with API key - use predictLongRunning endpoint
    QString url = QString("%1/%2:predictLongRunning?key=%3")
        .arg(m_baseUrl, m_model, m_apiKey);

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Build request body in Vertex AI format
    QJsonObject body;

    // Build instances array
    QJsonArray instances;
    QJsonObject instance;
    instance["prompt"] = params.prompt;

    // Add reference image for image-to-video
    if (!params.referenceImage.isEmpty()) {
        QJsonObject image;
        image["bytesBase64Encoded"] = QString::fromLatin1(params.referenceImage.toBase64());
        image["mimeType"] = params.referenceImageMimeType;
        instance["image"] = image;
        LOG_INFO("Including reference image for image-to-video generation");
    }

    instances.append(instance);
    body["instances"] = instances;

    // Build parameters object
    QJsonObject parameters;
    parameters["aspectRatio"] = params.aspectRatio;
    parameters["durationSeconds"] = params.durationSeconds;  // Must be number, not string
    // Note: generateAudio not supported by veo-3.1-generate-preview via API key
    body["parameters"] = parameters;

    LOG_INFO(QString("Starting Veo video generation: model=%1, duration=%2s, prompt=%3")
             .arg(m_model)
             .arg(params.durationSeconds)
             .arg(params.prompt.left(100)));

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

    LOG_DEBUG(QString("Veo response: %1").arg(QString::fromUtf8(data.left(500))));

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
        // Check if video is returned directly
        QJsonArray generatedVideos = response["generatedVideos"].toArray();
        if (!generatedVideos.isEmpty()) {
            QJsonObject video = generatedVideos[0].toObject()["video"].toObject();
            QString base64Data = video["videoBytes"].toString();
            if (base64Data.isEmpty()) {
                // Try alternate field name
                base64Data = video["video"].toString();
            }

            if (!base64Data.isEmpty()) {
                QByteArray videoData = QByteArray::fromBase64(base64Data.toUtf8());
                emit requestFinished();
                emit generationProgress(100);
                emit videoGenerated(videoData, originalPrompt);
                LOG_INFO("Video received directly (no polling needed)");
            } else {
                emit requestFinished();
                emit errorOccurred("Video data empty in response");
            }
        } else {
            // Check for error
            QJsonObject error = response["error"].toObject();
            if (!error.isEmpty()) {
                QString errorMsg = error["message"].toString();
                int code = error["code"].toInt();
                emit requestFinished();
                emit errorOccurred(QString("Erreur Veo (%1): %2").arg(code).arg(errorMsg));
            } else {
                emit requestFinished();
                emit errorOccurred("Reponse inattendue de l'API Veo");
            }
        }
    }

    reply->deleteLater();
}

void VeoClient::pollOperation(const QString& operationName, const QString& originalPrompt) {
    if (!isConfigured()) {
        emit requestFinished();
        emit errorOccurred("API not configured");
        return;
    }

    // Poll operation status
    QString url = QString("https://generativelanguage.googleapis.com/v1beta/%1?key=%2")
        .arg(operationName, m_apiKey);

    QNetworkRequest request;
    request.setUrl(QUrl(url));
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
        // Operation completed - try multiple response formats
        QJsonObject result = response["response"].toObject();
        QString base64Data;
        QString videoUri;

        // Try Gemini API format: generateVideoResponse.generatedSamples[0].video.uri
        QJsonObject generateVideoResponse = result["generateVideoResponse"].toObject();
        QJsonArray generatedSamples = generateVideoResponse["generatedSamples"].toArray();
        if (!generatedSamples.isEmpty()) {
            QJsonObject video = generatedSamples[0].toObject()["video"].toObject();
            videoUri = video["uri"].toString();
            if (videoUri.isEmpty()) {
                base64Data = video["bytesBase64Encoded"].toString();
            }
        }

        // Try Vertex AI format: predictions[0].video.bytesBase64Encoded
        if (base64Data.isEmpty() && videoUri.isEmpty()) {
            QJsonArray predictions = result["predictions"].toArray();
            if (!predictions.isEmpty()) {
                QJsonObject prediction = predictions[0].toObject();
                QJsonObject video = prediction["video"].toObject();
                videoUri = video["uri"].toString();
                if (videoUri.isEmpty()) {
                    base64Data = video["bytesBase64Encoded"].toString();
                }
            }
        }

        // If we have a URI, download the video
        if (!videoUri.isEmpty()) {
            LOG_INFO(QString("Downloading video from URI: %1").arg(videoUri));
            emit generationProgress(95);

            // Add API key to URI if needed
            QString downloadUrl = videoUri;
            if (!downloadUrl.contains("key=")) {
                downloadUrl += (downloadUrl.contains("?") ? "&" : "?") + QString("key=%1").arg(m_apiKey);
            }

            QNetworkRequest downloadRequest;
            downloadRequest.setUrl(QUrl(downloadUrl));

            QNetworkReply* downloadReply = m_networkManager->get(downloadRequest);
            connect(downloadReply, &QNetworkReply::finished, this, [this, downloadReply, originalPrompt]() {
                if (downloadReply->error() != QNetworkReply::NoError) {
                    emit requestFinished();
                    handleNetworkError(downloadReply);
                } else {
                    QByteArray videoData = downloadReply->readAll();
                    emit requestFinished();
                    emit generationProgress(100);
                    emit videoGenerated(videoData, originalPrompt);
                    LOG_INFO(QString("Video downloaded successfully (%1 bytes)").arg(videoData.size()));
                }
                downloadReply->deleteLater();
            });
            reply->deleteLater();
            return;  // Exit early, download callback will handle the rest
        }

        if (!base64Data.isEmpty()) {
            QByteArray videoData = QByteArray::fromBase64(base64Data.toUtf8());
            emit requestFinished();
            emit generationProgress(100);
            emit videoGenerated(videoData, originalPrompt);
            LOG_INFO(QString("Video generation completed successfully (%1 bytes)").arg(videoData.size()));
        } else {
            // Check for error
            QJsonObject error = response["error"].toObject();
            if (!error.isEmpty()) {
                QString errorMsg = error["message"].toString();
                emit requestFinished();
                emit errorOccurred(QString("Video generation failed: %1").arg(errorMsg));
            } else {
                // Show response structure for debugging
                QString responseStr = QString::fromUtf8(data.left(2000));
                LOG_ERROR(QString("No video data found. Response: %1").arg(responseStr));
                emit requestFinished();
                emit errorOccurred(QString("No video data in completed response\n\nResponse structure:\n%1").arg(responseStr));
            }
        }
    } else {
        // Still processing, continue polling
        QJsonObject metadata = response["metadata"].toObject();
        int progress = 20;
        if (metadata.contains("progress")) {
            progress = metadata["progress"].toInt();
        }
        emit generationProgress(qMin(progress, 95));

        LOG_INFO(QString("Video generation in progress: %1%").arg(progress));

        QTimer::singleShot(m_pollIntervalMs, this, [this, operationName, originalPrompt]() {
            pollOperation(operationName, originalPrompt);
        });
    }

    reply->deleteLater();
}

} // namespace codex::api
