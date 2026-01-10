#pragma once

#include <QDialog>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QProgressBar>

namespace codex::ui {

class VideoPreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit VideoPreviewDialog(QWidget* parent = nullptr);
    ~VideoPreviewDialog();

    // Set the video file to preview
    void setVideoFile(const QString& filePath);

    // Set optional TTS audio to merge
    void setTtsAudio(const QString& ttsPath, int durationMs);

    // Get the final output path (after merge if applicable)
    QString outputFilePath() const { return m_outputFilePath; }

private slots:
    void onPlayPause();
    void onStop();
    void onOpenExternal();
    void onMergeAudio();
    void onSaveAs();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    void setupUi();
    QString formatTime(qint64 ms) const;
    QString findFFmpeg() const;

    // Video player
    QMediaPlayer* m_mediaPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QVideoWidget* m_videoWidget = nullptr;

    // Controls
    QPushButton* m_playPauseBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QPushButton* m_openExternalBtn = nullptr;
    QPushButton* m_mergeAudioBtn = nullptr;
    QPushButton* m_saveAsBtn = nullptr;
    QSlider* m_positionSlider = nullptr;
    QLabel* m_timeLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_mergeProgress = nullptr;

    // Data
    QString m_videoFilePath;
    QString m_ttsAudioPath;
    int m_ttsDurationMs = 0;
    QString m_outputFilePath;
    bool m_isPlaying = false;
};

} // namespace codex::ui
