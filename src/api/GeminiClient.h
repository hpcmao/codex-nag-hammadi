#pragma once

#include "ApiClient.h"
#include <QJsonObject>

namespace codex::api {

class GeminiClient : public ApiClient {
    Q_OBJECT

public:
    explicit GeminiClient(QObject* parent = nullptr);

    // Analyze and enrich a passage (similar to Claude)
    void enrichPassage(const QString& prompt);

    // Generate image prompt from passage
    void generateImagePrompt(const QString& passage, const QString& style);

    // Configuration
    void setModel(const QString& model) { m_model = model; }
    QString model() const { return m_model; }

signals:
    void enrichmentCompleted(const QJsonObject& response);
    void imagePromptGenerated(const QString& prompt);

private slots:
    void onEnrichReplyFinished(QNetworkReply* reply);
    void onPromptReplyFinished(QNetworkReply* reply);

private:
    QString m_model = "gemini-2.0-flash";
    int m_maxTokens = 2048;
};

} // namespace codex::api
