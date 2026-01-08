#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

namespace codex::api {

enum class GoogleAIProvider {
    AIStudio,   // Google AI Studio with API key
    VertexAI    // Vertex AI with OAuth2 token
};

class ApiClient : public QObject {
    Q_OBJECT

public:
    explicit ApiClient(QObject* parent = nullptr);
    virtual ~ApiClient() = default;

    // API Key authentication (Google AI Studio)
    void setApiKey(const QString& key);

    // OAuth2 token authentication (Vertex AI)
    void setAccessToken(const QString& token);

    // Vertex AI configuration
    void setVertexConfig(const QString& projectId, const QString& region);

    // Set provider type
    void setProvider(GoogleAIProvider provider);
    GoogleAIProvider provider() const { return m_provider; }

    virtual bool isConfigured() const;

signals:
    void requestStarted();
    void requestFinished();
    void errorOccurred(const QString& error);

protected:
    QNetworkRequest createRequest(const QString& endpoint);
    QNetworkRequest createVertexRequest(const QString& model, const QString& method);
    void handleNetworkError(QNetworkReply* reply);

    QString getVertexBaseUrl() const;

    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    QString m_accessToken;
    QString m_baseUrl;

    // Vertex AI settings
    GoogleAIProvider m_provider = GoogleAIProvider::AIStudio;
    QString m_vertexProjectId;
    QString m_vertexRegion = "us-central1";
};

} // namespace codex::api
