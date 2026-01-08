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

bool ApiClient::isConfigured() const {
    return !m_apiKey.isEmpty();
}

QNetworkRequest ApiClient::createRequest(const QString& endpoint) {
    QNetworkRequest request;
    request.setUrl(QUrl(m_baseUrl + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return request;
}

void ApiClient::handleNetworkError(QNetworkReply* reply) {
    QString errorMsg = reply->errorString();
    LOG_ERROR(QString("API Error: %1").arg(errorMsg));
    emit errorOccurred(errorMsg);
}

} // namespace codex::api
