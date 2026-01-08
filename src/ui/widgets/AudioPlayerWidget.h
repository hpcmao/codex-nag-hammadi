#pragma once

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QUrl>

namespace codex::ui {

class AudioPlayerWidget : public QWidget {
    Q_OBJECT

public:
    explicit AudioPlayerWidget(QWidget* parent = nullptr);
    ~AudioPlayerWidget();

    // Load audio from file or data
    void loadFromFile(const QString& filePath);
    void loadFromData(const QByteArray& audioData);

    // Playback controls
    void play();
    void pause();
    void stop();

    // State
    bool isPlaying() const;
    bool hasAudio() const { return m_hasAudio; }

    // Volume (0-100)
    int volume() const;
    void setVolume(int volume);

signals:
    void playbackStarted();
    void playbackPaused();
    void playbackStopped();
    void playbackFinished();
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onVolumeSliderChanged(int value);
    void onPositionSliderPressed();
    void onPositionSliderReleased();

private:
    void setupUi();
    void updatePlayPauseButton();
    QString formatTime(qint64 ms) const;

    QMediaPlayer* m_player;
    QAudioOutput* m_audioOutput;

    QPushButton* m_playPauseBtn;
    QPushButton* m_stopBtn;
    QSlider* m_positionSlider;
    QSlider* m_volumeSlider;
    QLabel* m_timeLabel;
    QLabel* m_statusLabel;

    bool m_hasAudio = false;
    bool m_sliderPressed = false;
    QString m_tempFilePath;
};

} // namespace codex::ui
