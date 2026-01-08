#pragma once

#include "ApiClient.h"
#include <QJsonObject>

namespace codex::api {

class ClaudeClient : public ApiClient {
    Q_OBJECT

public:
    explicit ClaudeClient(QObject* parent = nullptr);

    void enrichPassage(const QString& prompt);

signals:
    void enrichmentCompleted(const QJsonObject& response);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QString m_model = "claude-sonnet-4-20250514";
    int m_maxTokens = 1000;
};

} // namespace codex::api
