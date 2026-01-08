#include "GeminiClient.h"
#include "utils/Logger.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace codex::api {

GeminiClient::GeminiClient(QObject* parent)
    : ApiClient(parent)
{
    m_baseUrl = "https://generativelanguage.googleapis.com/v1beta/models";
}

void GeminiClient::enrichPassage(const QString& prompt) {
    if (!isConfigured()) {
        emit errorOccurred("Gemini API not configured");
        return;
    }

    emit requestStarted();

    QNetworkRequest request;
    QString url;
    if (m_provider == GoogleAIProvider::VertexAI) {
        // Vertex AI endpoint avec clé API
        url = QString("https://aiplatform.googleapis.com/v1/publishers/google/models/%1:generateContent?key=%2")
            .arg(m_model, m_apiKey);
    } else {
        // AI Studio endpoint
        url = QString("%1/%2:generateContent?key=%3").arg(m_baseUrl, m_model, m_apiKey);
    }
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    QJsonArray contents;
    QJsonObject content;
    QJsonArray parts;
    QJsonObject part;
    part["text"] = prompt;
    parts.append(part);
    content["parts"] = parts;
    contents.append(content);
    body["contents"] = contents;

    // Generation config
    QJsonObject genConfig;
    genConfig["maxOutputTokens"] = m_maxTokens;
    genConfig["temperature"] = 0.7;
    body["generationConfig"] = genConfig;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onEnrichReplyFinished(reply);
    });
}

void GeminiClient::generateImagePrompt(const QString& passage, const QString& style) {
    if (!isConfigured()) {
        emit errorOccurred("Gemini API key not configured");
        return;
    }

    emit requestStarted();

    QString systemPrompt = QString(
        "Tu es un expert en art visuel et en textes gnostiques. "
        "Transforme ce passage gnostique en un prompt detaille pour generer une image photoréaliste. "
        "Style demande: %1. "
        "Le prompt doit decrire une scene visuelle concrete, avec des details sur: "
        "- L'eclairage (divin, mystique, celeste) "
        "- Les personnages et leurs attributs "
        "- L'environnement et l'atmosphere "
        "- Les symboles gnostiques visuels "
        "Reponds UNIQUEMENT avec le prompt en anglais, sans explication."
    ).arg(style);

    QString fullPrompt = systemPrompt + "\n\nPassage:\n" + passage;

    QNetworkRequest request;
    QString url;
    if (m_provider == GoogleAIProvider::VertexAI) {
        // Vertex AI endpoint avec clé API
        url = QString("https://aiplatform.googleapis.com/v1/publishers/google/models/%1:generateContent?key=%2")
            .arg(m_model, m_apiKey);
    } else {
        // AI Studio endpoint
        url = QString("%1/%2:generateContent?key=%3").arg(m_baseUrl, m_model, m_apiKey);
    }
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    QJsonArray contents;
    QJsonObject content;
    QJsonArray parts;
    QJsonObject part;
    part["text"] = fullPrompt;
    parts.append(part);
    content["parts"] = parts;
    contents.append(content);
    body["contents"] = contents;

    QJsonObject genConfig;
    genConfig["maxOutputTokens"] = 500;
    genConfig["temperature"] = 0.8;
    body["generationConfig"] = genConfig;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onPromptReplyFinished(reply);
    });
}

void GeminiClient::onEnrichReplyFinished(QNetworkReply* reply) {
    emit requestFinished();

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject response = doc.object();

    // Extract text from Gemini response
    QJsonArray candidates = response["candidates"].toArray();
    if (!candidates.isEmpty()) {
        QJsonObject candidate = candidates[0].toObject();
        QJsonObject content = candidate["content"].toObject();
        QJsonArray parts = content["parts"].toArray();
        if (!parts.isEmpty()) {
            QString text = parts[0].toObject()["text"].toString();
            QJsonObject result;
            result["text"] = text;
            emit enrichmentCompleted(result);
        }
    } else {
        emit errorOccurred("No response from Gemini");
    }

    reply->deleteLater();
}

void GeminiClient::onPromptReplyFinished(QNetworkReply* reply) {
    emit requestFinished();

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject response = doc.object();

    QJsonArray candidates = response["candidates"].toArray();
    if (!candidates.isEmpty()) {
        QJsonObject candidate = candidates[0].toObject();
        QJsonObject content = candidate["content"].toObject();
        QJsonArray parts = content["parts"].toArray();
        if (!parts.isEmpty()) {
            QString prompt = parts[0].toObject()["text"].toString().trimmed();
            LOG_INFO(QString("Generated image prompt: %1").arg(prompt.left(100)));
            emit imagePromptGenerated(prompt);
        }
    } else {
        emit errorOccurred("No prompt generated from Gemini");
    }

    reply->deleteLater();
}

} // namespace codex::api
