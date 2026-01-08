#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QDateTime>

namespace codex::api {

/**
 * Handles OAuth2 authentication for Google Cloud Vertex AI using Service Account credentials.
 * Manages token refresh automatically.
 */
class VertexAuthenticator : public QObject {
    Q_OBJECT

public:
    explicit VertexAuthenticator(QObject* parent = nullptr);

    // Load service account credentials from JSON file
    bool loadServiceAccount(const QString& jsonFilePath);

    // Get current access token (refreshes if needed)
    QString getAccessToken();

    // Check if credentials are loaded
    bool isConfigured() const { return m_configured; }

    // Get project ID from service account
    QString projectId() const { return m_projectId; }

    // Force token refresh
    void refreshToken();

signals:
    void tokenRefreshed(const QString& token);
    void authenticationFailed(const QString& error);

private:
    QString createJWT();
    void requestAccessToken(const QString& jwt);

    QNetworkAccessManager* m_networkManager = nullptr;

    // Service account credentials
    QString m_clientEmail;
    QString m_privateKey;
    QString m_projectId;
    QString m_tokenUri;

    // Current access token
    QString m_accessToken;
    QDateTime m_tokenExpiry;

    bool m_configured = false;

    static const int TOKEN_EXPIRY_MARGIN_SECONDS = 300;  // Refresh 5 min before expiry
};

} // namespace codex::api
