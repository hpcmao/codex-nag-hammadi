#include "ImagenClient.h"
#include "utils/Logger.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>

namespace codex::api {

ImagenClient::ImagenClient(QObject* parent)
    : ApiClient(parent)
{
    m_baseUrl = "https://generativelanguage.googleapis.com/v1beta/models";
}

bool ImagenClient::isConfigured() const {
    // ImagenClient uses API key for both AI Studio and Vertex AI
    return !m_apiKey.isEmpty();
}

void ImagenClient::generateImage(const ImageGenerationParams& params) {
    if (!isConfigured()) {
        emit errorOccurred("Imagen API not configured");
        return;
    }

    emit requestStarted();
    emit generationProgress(10);

    QNetworkRequest request;
    QJsonObject body;
    QString url;

    if (m_provider == GoogleAIProvider::VertexAI) {
        // Vertex AI endpoint avec clé API
        url = QString("https://aiplatform.googleapis.com/v1/publishers/google/models/%1:predict?key=%2")
            .arg(m_model, m_apiKey);

        QJsonArray instances;
        QJsonObject instance;
        instance["prompt"] = params.prompt;
        instances.append(instance);
        body["instances"] = instances;

        QJsonObject parameters;
        parameters["aspectRatio"] = params.aspectRatio;
        parameters["sampleCount"] = params.numberOfImages;
        body["parameters"] = parameters;
    } else {
        // AI Studio endpoint
        url = QString("%1/%2:generateImages?key=%3").arg(m_baseUrl, m_model, m_apiKey);

        body["prompt"] = params.prompt;

        QJsonObject imageParams;
        imageParams["aspectRatio"] = params.aspectRatio;
        imageParams["numberOfImages"] = params.numberOfImages;
        body["imageGenerationConfig"] = imageParams;
    }
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            int progress = 10 + static_cast<int>(80.0 * received / total);
            emit generationProgress(progress);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, params]() {
        onReplyFinished(reply, params.prompt);
    });
}

void ImagenClient::onReplyFinished(QNetworkReply* reply, const QString& originalPrompt) {
    emit requestFinished();
    emit generationProgress(100);

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject response = doc.object();

    // Extract base64 image data - different format for AI Studio vs Vertex AI
    QString base64Data;

    if (m_provider == GoogleAIProvider::VertexAI) {
        // Vertex AI response format: predictions[0].bytesBase64Encoded
        QJsonArray predictions = response["predictions"].toArray();
        if (!predictions.isEmpty()) {
            base64Data = predictions[0].toObject()["bytesBase64Encoded"].toString();
        }
    } else {
        // AI Studio response format: images[0].bytesBase64Encoded
        QJsonArray images = response["images"].toArray();
        if (!images.isEmpty()) {
            base64Data = images[0].toObject()["bytesBase64Encoded"].toString();
        }
    }

    if (!base64Data.isEmpty()) {
        QByteArray imageData = QByteArray::fromBase64(base64Data.toUtf8());

        QPixmap pixmap;
        if (pixmap.loadFromData(imageData)) {
            emit imageGenerated(pixmap, originalPrompt);
        } else {
            emit errorOccurred("Failed to decode image data");
        }
    } else {
        emit errorOccurred("No images in response");
    }

    reply->deleteLater();
}

} // namespace codex::api
