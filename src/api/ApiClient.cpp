#include "ApiClient.h"
#include "utils/Logger.h"

#include <QNetworkRequest>

namespace codex::api {

ApiClient::ApiClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

void ApiClient::setApiKey(const QString& key) {
    m_apiKey = key;
}

void ApiClient::setAccessToken(const QString& token) {
    m_accessToken = token;
}

void ApiClient::setVertexConfig(const QString& projectId, const QString& region) {
    m_vertexProjectId = projectId;
    m_vertexRegion = region.isEmpty() ? "us-central1" : region;
    LOG_INFO(QString("ApiClient: Vertex AI config set - project: %1, region: %2")
             .arg(m_vertexProjectId, m_vertexRegion));
}

void ApiClient::setProvider(GoogleAIProvider provider) {
    m_provider = provider;
    LOG_INFO(QString("ApiClient: Provider set to %1")
             .arg(provider == GoogleAIProvider::VertexAI ? "Vertex AI" : "AI Studio"));
}

bool ApiClient::isConfigured() const {
    if (m_provider == GoogleAIProvider::VertexAI) {
        return !m_accessToken.isEmpty() && !m_vertexProjectId.isEmpty();
    }
    return !m_apiKey.isEmpty();
}

QString ApiClient::getVertexBaseUrl() const {
    return QString("https://%1-aiplatform.googleapis.com/v1").arg(m_vertexRegion);
}

QNetworkRequest ApiClient::createRequest(const QString& endpoint) {
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseUrl + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return request;
}

QNetworkRequest ApiClient::createVertexRequest(const QString& model, const QString& method) {
    // Vertex AI endpoint format:
    // https://{REGION}-aiplatform.googleapis.com/v1/projects/{PROJECT}/locations/{REGION}/publishers/google/models/{MODEL}:{METHOD}

    QString url = QString("%1/projects/%2/locations/%3/publishers/google/models/%4:%5")
        .arg(getVertexBaseUrl())
        .arg(m_vertexProjectId)
        .arg(m_vertexRegion)
        .arg(model)
        .arg(method);

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());

    LOG_INFO(QString("ApiClient: Vertex AI request to %1").arg(url));

    return request;
}

void ApiClient::handleNetworkError(QNetworkReply* reply) {
    QString errorMsg = reply->errorString();
    QByteArray responseData = reply->readAll();

    if (!responseData.isEmpty()) {
        errorMsg += "\nResponse: " + QString::fromUtf8(responseData.left(500));
    }

    LOG_ERROR(QString("API Error: %1").arg(errorMsg));
    emit errorOccurred(errorMsg);
}

} // namespace codex::api
