#pragma once

#include "ApiClient.h"
#include <QPixmap>

namespace codex::api {

struct ImageGenerationParams {
    QString prompt;
    QString aspectRatio = "16:9";
    int numberOfImages = 1;
};

class ImagenClient : public ApiClient {
    Q_OBJECT

public:
    explicit ImagenClient(QObject* parent = nullptr);

    void generateImage(const ImageGenerationParams& params);

signals:
    void imageGenerated(const QPixmap& image, const QString& prompt);
    void generationProgress(int percent);

private slots:
    void onReplyFinished(QNetworkReply* reply, const QString& originalPrompt);

private:
    QString m_model = "imagen-3.0-generate-001";
};

} // namespace codex::api
