#include "MediaStorage.h"
#include "Config.h"
#include "Logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace codex::utils {

MediaStorage& MediaStorage::instance() {
    static MediaStorage instance;
    return instance;
}

MediaStorage::MediaStorage() {
    updateBasePath();
}

void MediaStorage::updateBasePath() {
    // Use configured output path, or codex file's parent directory, or fall back to app data
    auto& config = Config::instance();
    QString imagesPath = config.outputImagesPath();

    if (!imagesPath.isEmpty()) {
        m_basePath = imagesPath;
    } else {
        QString codexPath = config.codexFilePath();
        if (!codexPath.isEmpty()) {
            QFileInfo fi(codexPath);
            m_basePath = fi.absolutePath();
        } else {
            m_basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        }
    }
    ensureDirectories();
}

void MediaStorage::ensureDirectories() {
    QDir().mkpath(sessionsFolder());
    QDir().mkpath(videosFolder());
}

QString MediaStorage::sessionsFolder() const {
    return m_basePath + "/sessions";
}

QString MediaStorage::videosFolder() const {
    // Use configured videos path if set, otherwise default to m_basePath/videos
    QString videosPath = Config::instance().outputVideosPath();
    if (!videosPath.isEmpty()) {
        return videosPath;
    }
    return m_basePath + "/videos";
}

QString MediaStorage::timestampString() const {
    return QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
}

QString MediaStorage::createSession(const QString& treatiseCode) {
    m_currentTreatiseCode = treatiseCode;
    QString timestamp = timestampString();
    QString sessionName = treatiseCode.isEmpty() ? timestamp : treatiseCode + "_" + timestamp;
    m_currentSessionPath = sessionsFolder() + "/" + sessionName;

    QDir().mkpath(m_currentSessionPath);
    QDir().mkpath(m_currentSessionPath + "/images");
    QDir().mkpath(m_currentSessionPath + "/audio");

    LOG_INFO(QString("Created session: %1").arg(m_currentSessionPath));
    return m_currentSessionPath;
}

void MediaStorage::saveSessionMetadata(const QString& treatiseCode, const QString& category, int imageCount, const QStringList& texts) {
    if (m_currentSessionPath.isEmpty()) {
        return;
    }

    QJsonObject metadata;
    metadata["treatiseCode"] = treatiseCode;
    metadata["category"] = category;
    metadata["timestamp"] = timestampString();
    metadata["imageCount"] = imageCount;

    QJsonArray textsArray;
    for (const QString& text : texts) {
        textsArray.append(text);
    }
    metadata["texts"] = textsArray;

    QString metaPath = sessionMetadataPath(m_currentSessionPath);
    QFile file(metaPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(metadata).toJson());
        file.close();
    }
}

SessionInfo MediaStorage::loadSessionInfo(const QString& sessionPath) const {
    SessionInfo info;

    QString metaPath = sessionMetadataPath(sessionPath);
    QFile file(metaPath);
    if (!file.open(QIODevice::ReadOnly)) {
        // Try to infer from directory name
        QFileInfo fi(sessionPath);
        QString dirName = fi.fileName();
        int underscoreIdx = dirName.indexOf('_');
        if (underscoreIdx > 0) {
            info.treatiseCode = dirName.left(underscoreIdx);
            info.timestamp = dirName.mid(underscoreIdx + 1);
        } else {
            info.timestamp = dirName;
        }
        info.imageCount = listImages(sessionPath).size();
        info.audioCount = listAudios(sessionPath).size();
        return info;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject obj = doc.object();
    info.treatiseCode = obj["treatiseCode"].toString();
    info.timestamp = obj["timestamp"].toString();
    info.imageCount = obj["imageCount"].toInt();

    QJsonArray textsArray = obj["texts"].toArray();
    for (const QJsonValue& v : textsArray) {
        info.texts.append(v.toString());
    }

    // Update counts from actual files if metadata is outdated
    info.imageCount = qMax(info.imageCount, listImages(sessionPath).size());
    info.audioCount = listAudios(sessionPath).size();

    return info;
}

QStringList MediaStorage::listSessions() const {
    QDir sessionsDir(sessionsFolder());
    return sessionsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
}

bool MediaStorage::deleteSession(const QString& sessionPath) {
    QDir dir(sessionPath);
    return dir.removeRecursively();
}

QString MediaStorage::sessionMetadataPath(const QString& sessionPath) const {
    return sessionPath + "/metadata.json";
}

// Image storage

bool MediaStorage::saveImage(const QPixmap& image, int index, const QString& text) {
    if (m_currentSessionPath.isEmpty()) {
        LOG_WARN("No current session, cannot save image");
        return false;
    }
    return saveImage(m_currentSessionPath, index, image);
}

bool MediaStorage::saveImage(const QString& sessionPath, int index, const QPixmap& image) {
    QString path = imagePath(sessionPath, index);
    QDir().mkpath(QFileInfo(path).absolutePath());
    bool success = image.save(path, "PNG");
    if (success) {
        LOG_INFO(QString("Saved image %1 to %2").arg(index).arg(path));
    } else {
        LOG_ERROR(QString("Failed to save image %1").arg(index));
    }
    return success;
}

QPixmap MediaStorage::loadImage(const QString& sessionPath, int index) const {
    QString path = imagePath(sessionPath, index);
    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        LOG_WARN(QString("Failed to load image: %1").arg(path));
    }
    return pixmap;
}

QStringList MediaStorage::listImages(const QString& sessionPath) const {
    QDir imagesDir(sessionPath + "/images");
    return imagesDir.entryList(QStringList() << "*.png" << "*.jpg" << "*.jpeg",
                                QDir::Files, QDir::Name);
}

QString MediaStorage::imagePath(const QString& sessionPath, int index) const {
    return sessionPath + QString("/images/image_%1.png").arg(index, 4, 10, QChar('0'));
}

// Audio storage

QString MediaStorage::saveAudio(const QByteArray& audioData, int index) {
    if (m_currentSessionPath.isEmpty()) {
        LOG_WARN("No current session, cannot save audio");
        return QString();
    }
    return saveAudio(m_currentSessionPath, audioData, index);
}

QString MediaStorage::saveAudio(const QString& sessionPath, const QByteArray& audioData, int index) {
    QString path = audioPath(sessionPath, index);
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open audio file for writing: %1").arg(path));
        return QString();
    }

    file.write(audioData);
    file.close();

    LOG_INFO(QString("Saved audio %1 to %2").arg(index).arg(path));
    return path;
}

QByteArray MediaStorage::loadAudio(const QString& sessionPath, int index) const {
    QString path = audioPath(sessionPath, index);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARN(QString("Failed to load audio: %1").arg(path));
        return QByteArray();
    }
    return file.readAll();
}

QStringList MediaStorage::listAudios(const QString& sessionPath) const {
    QDir audioDir(sessionPath + "/audio");
    return audioDir.entryList(QStringList() << "*.mp3" << "*.wav" << "*.ogg",
                               QDir::Files, QDir::Name);
}

QString MediaStorage::audioPath(const QString& sessionPath, int index) const {
    return sessionPath + QString("/audio/audio_%1.mp3").arg(index, 4, 10, QChar('0'));
}

} // namespace codex::utils
