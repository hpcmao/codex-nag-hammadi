#include "VertexAuthenticator.h"
#include "utils/Logger.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcess>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>

namespace codex::api {

VertexAuthenticator::VertexAuthenticator(QObject* parent)
    : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

bool VertexAuthenticator::loadServiceAccount(const QString& jsonFilePath) {
    QFile file(jsonFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("VertexAuth: Cannot open service account file: %1").arg(jsonFilePath));
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject json = doc.object();

    m_clientEmail = json["client_email"].toString();
    m_privateKey = json["private_key"].toString();
    m_projectId = json["project_id"].toString();
    m_tokenUri = json["token_uri"].toString();

    if (m_tokenUri.isEmpty()) {
        m_tokenUri = "https://oauth2.googleapis.com/token";
    }

    if (m_clientEmail.isEmpty() || m_privateKey.isEmpty() || m_projectId.isEmpty()) {
        LOG_ERROR("VertexAuth: Invalid service account file - missing required fields");
        return false;
    }

    m_configured = true;
    LOG_INFO(QString("VertexAuth: Loaded service account for project: %1, email: %2")
             .arg(m_projectId, m_clientEmail));

    return true;
}

QString VertexAuthenticator::getAccessToken() {
    // Check if we have a valid token
    if (!m_accessToken.isEmpty() && m_tokenExpiry.isValid()) {
        if (QDateTime::currentDateTimeUtc().secsTo(m_tokenExpiry) > TOKEN_EXPIRY_MARGIN_SECONDS) {
            return m_accessToken;
        }
    }

    // Try to get token using gcloud CLI (simplest method)
    QProcess process;
    process.start("gcloud", QStringList() << "auth" << "application-default" << "print-access-token");

    if (process.waitForFinished(10000)) {
        if (process.exitCode() == 0) {
            m_accessToken = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            m_tokenExpiry = QDateTime::currentDateTimeUtc().addSecs(3600);  // Assume 1 hour
            LOG_INFO("VertexAuth: Got access token via gcloud CLI");
            return m_accessToken;
        }
    }

    // If gcloud failed, try using service account directly
    if (m_configured) {
        // For now, return empty and log instruction
        LOG_WARN("VertexAuth: gcloud CLI not available. Please run: gcloud auth application-default login");
        emit authenticationFailed(
            "Authentification Vertex AI requise.\n\n"
            "Executez cette commande dans un terminal:\n"
            "gcloud auth application-default login\n\n"
            "Ou utilisez Google AI Studio avec une cle API."
        );
    }

    return QString();
}

void VertexAuthenticator::refreshToken() {
    m_accessToken.clear();
    m_tokenExpiry = QDateTime();
    getAccessToken();
}

QString VertexAuthenticator::createJWT() {
    // JWT Header
    QJsonObject header;
    header["alg"] = "RS256";
    header["typ"] = "JWT";

    // JWT Payload
    qint64 now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    QJsonObject payload;
    payload["iss"] = m_clientEmail;
    payload["scope"] = "https://www.googleapis.com/auth/cloud-platform";
    payload["aud"] = m_tokenUri;
    payload["iat"] = now;
    payload["exp"] = now + 3600;

    // Base64url encode
    auto base64url = [](const QByteArray& data) -> QString {
        return QString::fromUtf8(data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    };

    QString headerB64 = base64url(QJsonDocument(header).toJson(QJsonDocument::Compact));
    QString payloadB64 = base64url(QJsonDocument(payload).toJson(QJsonDocument::Compact));

    QString unsignedToken = headerB64 + "." + payloadB64;

    // Note: RS256 signing requires OpenSSL or similar library
    // For now, we rely on gcloud CLI for authentication
    LOG_WARN("VertexAuth: JWT signing not implemented, using gcloud CLI instead");

    return QString();
}

void VertexAuthenticator::requestAccessToken(const QString& jwt) {
    Q_UNUSED(jwt)
    // This would exchange JWT for access token
    // Not implemented - using gcloud CLI instead
}

} // namespace codex::api
