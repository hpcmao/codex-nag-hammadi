#include "ClaudeClient.h"
#include "utils/Logger.h"

#include <QJsonDocument>
#include <QJsonArray>

namespace codex::api {

ClaudeClient::ClaudeClient(QObject* parent)
    : ApiClient(parent)
{
    m_baseUrl = "https://api.anthropic.com";
}

void ClaudeClient::enrichPassage(const QString& prompt) {
    if (!isConfigured()) {
        emit errorOccurred("Claude API key not configured");
        return;
    }

    emit requestStarted();

    QNetworkRequest request = createRequest("/v1/messages");
    request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");

    QJsonObject body;
    body["model"] = m_model;
    body["max_tokens"] = m_maxTokens;

    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    message["content"] = prompt;
    messages.append(message);
    body["messages"] = messages;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void ClaudeClient::onReplyFinished(QNetworkReply* reply) {
    emit requestFinished();

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject response = doc.object();

    // Extract content from Claude response
    QJsonArray content = response["content"].toArray();
    if (!content.isEmpty()) {
        QString text = content[0].toObject()["text"].toString();

        // Try to parse as JSON (for structured responses)
        QJsonDocument contentDoc = QJsonDocument::fromJson(text.toUtf8());
        if (!contentDoc.isNull()) {
            emit enrichmentCompleted(contentDoc.object());
        } else {
            // Return as plain text wrapped in JSON
            QJsonObject result;
            result["text"] = text;
            emit enrichmentCompleted(result);
        }
    }

    reply->deleteLater();
}

} // namespace codex::api
