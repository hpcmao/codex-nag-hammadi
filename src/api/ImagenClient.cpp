#include "ImagenClient.h"
#include "utils/Logger.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QBuffer>

namespace codex::api {

ImagenClient::ImagenClient(QObject* parent)
    : ApiClient(parent)
{
    m_baseUrl = "https://generativelanguage.googleapis.com/v1beta/models";
}

void ImagenClient::generateImage(const ImageGenerationParams& params) {
    if (!isConfigured()) {
        emit errorOccurred("Imagen API key not configured");
        return;
    }

    emit requestStarted();
    emit generationProgress(10);

    QString endpoint = QString("/%1:generateImages?key=%2").arg(m_model, m_apiKey);
    QNetworkRequest request = createRequest(endpoint);

    QJsonObject body;
    body["prompt"] = params.prompt;

    QJsonObject imageParams;
    imageParams["aspectRatio"] = params.aspectRatio;
    imageParams["numberOfImages"] = params.numberOfImages;
    body["imageGenerationConfig"] = imageParams;

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

    QJsonArray images = response["images"].toArray();
    if (!images.isEmpty()) {
        QString base64Data = images[0].toObject()["bytesBase64Encoded"].toString();
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
