#pragma once

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPropertyAnimation>
#include <QVector>

namespace codex::ui {

struct SlideData {
    QPixmap image;
    QString audioPath;
    int audioDurationMs = 5000;  // Default 5 seconds if no audio
    QString passageText;
};

class SlideshowWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal fadeOpacity READ fadeOpacity WRITE setFadeOpacity)

public:
    explicit SlideshowWidget(QWidget* parent = nullptr);
    ~SlideshowWidget();

    // Load slideshow content
    void setSlides(const QVector<SlideData>& slides);
    void addSlide(const SlideData& slide);
    void clear();

    // Playback control
    void start();
    void pause();
    void resume();
    void stop();
    void nextSlide();
    void previousSlide();
    void goToSlide(int index);

    // State
    bool isPlaying() const { return m_isPlaying; }
    bool isPaused() const { return m_isPaused; }
    int currentSlideIndex() const { return m_currentIndex; }
    int slideCount() const { return m_slides.size(); }

    // Settings
    void setTransitionDuration(int ms) { m_transitionDurationMs = ms; }
    int transitionDuration() const { return m_transitionDurationMs; }
    void setDefaultSlideDuration(int ms) { m_defaultSlideDurationMs = ms; }
    int defaultSlideDuration() const { return m_defaultSlideDurationMs; }
    void setAutoPlay(bool autoPlay) { m_autoPlay = autoPlay; }
    bool autoPlay() const { return m_autoPlay; }

    // Volume
    void setVolume(int volume);
    int volume() const;

    // Fade animation property
    qreal fadeOpacity() const { return m_fadeOpacity; }
    void setFadeOpacity(qreal opacity);

signals:
    void slideChanged(int index, int total);
    void playbackStarted();
    void playbackPaused();
    void playbackStopped();
    void playbackFinished();
    void progressChanged(int currentMs, int totalMs);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onAudioPositionChanged(qint64 position);
    void onAudioMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onSlideTimerTimeout();
    void onFadeAnimationFinished();
    void onControlsTimeout();

private:
    void showSlide(int index);
    void startSlideTimer();
    void startFadeTransition();
    void playAudioForCurrentSlide();
    void updateScaledImage();
    void showControls();
    void hideControls();
    void toggleFullscreen();
    int calculateTotalDuration() const;

    QVector<SlideData> m_slides;
    int m_currentIndex = 0;
    int m_nextIndex = 0;

    // Playback state
    bool m_isPlaying = false;
    bool m_isPaused = false;
    bool m_inTransition = false;

    // Settings
    int m_transitionDurationMs = 800;
    int m_defaultSlideDurationMs = 5000;
    bool m_autoPlay = true;

    // Display
    QPixmap m_currentImage;
    QPixmap m_nextImage;
    QPixmap m_scaledCurrentImage;
    QPixmap m_scaledNextImage;
    qreal m_fadeOpacity = 1.0;

    // Timers
    QTimer* m_slideTimer;
    QTimer* m_controlsTimer;

    // Animation
    QPropertyAnimation* m_fadeAnimation;

    // Audio
    QMediaPlayer* m_audioPlayer;
    QAudioOutput* m_audioOutput;

    // Controls visibility
    bool m_controlsVisible = false;
    bool m_isFullscreen = false;
};

} // namespace codex::ui
