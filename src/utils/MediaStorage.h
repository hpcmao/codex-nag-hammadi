#pragma once

#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QByteArray>
#include <QDateTime>

namespace codex::utils {

struct SessionInfo {
    QString treatiseCode;
    QString timestamp;
    int imageCount = 0;
    int audioCount = 0;
    QStringList texts;
};

class MediaStorage {
public:
    static MediaStorage& instance();

    // Base path management
    void updateBasePath();
    QString basePath() const { return m_basePath; }

    // Session management
    QString createSession(const QString& treatiseCode);
    QString currentSessionPath() const { return m_currentSessionPath; }
    void saveSessionMetadata(const QString& treatiseCode, const QString& category, int imageCount, const QStringList& texts);
    SessionInfo loadSessionInfo(const QString& sessionPath) const;
    QStringList listSessions() const;
    bool deleteSession(const QString& sessionPath);

    // Image storage
    bool saveImage(const QPixmap& image, int index, const QString& text = QString());
    bool saveImage(const QString& sessionPath, int index, const QPixmap& image);
    QPixmap loadImage(const QString& sessionPath, int index) const;
    QStringList listImages(const QString& sessionPath) const;
    QString imagePath(const QString& sessionPath, int index) const;

    // Audio storage
    QString saveAudio(const QByteArray& audioData, int index);
    QString saveAudio(const QString& sessionPath, const QByteArray& audioData, int index);
    QByteArray loadAudio(const QString& sessionPath, int index) const;
    QStringList listAudios(const QString& sessionPath) const;
    QString audioPath(const QString& sessionPath, int index) const;

    // Folders
    QString sessionsFolder() const;
    QString videosFolder() const;

    // Utilities
    QString timestampString() const;

private:
    MediaStorage();
    MediaStorage(const MediaStorage&) = delete;
    MediaStorage& operator=(const MediaStorage&) = delete;

    void ensureDirectories();
    QString sessionMetadataPath(const QString& sessionPath) const;

    QString m_basePath;
    QString m_currentSessionPath;
    QString m_currentTreatiseCode;
};

} // namespace codex::utils
