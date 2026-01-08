#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

namespace codex::api {

class ApiClient : public QObject {
    Q_OBJECT

public:
    explicit ApiClient(QObject* parent = nullptr);
    virtual ~ApiClient() = default;

    void setApiKey(const QString& key);
    bool isConfigured() const;

signals:
    void requestStarted();
    void requestFinished();
    void errorOccurred(const QString& error);

protected:
    QNetworkRequest createRequest(const QString& endpoint);
    void handleNetworkError(QNetworkReply* reply);

    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    QString m_baseUrl;
};

} // namespace codex::api
