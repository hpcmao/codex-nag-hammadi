#pragma once

#include <QDialog>
#include <QVector>
#include <QPixmap>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QProgressBar>
#include <QListWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QCheckBox>

namespace codex::api {
class EdgeTTSClient;
class VeoClient;
}

namespace codex::core {
class PipelineController;
}

namespace codex::ui {

struct SlideItem {
    QString text;           // Passage text
    QPixmap image;          // Generated image
    QString audioPath;      // TTS audio file path
    int audioDurationMs = 0;
    bool imageReady = false;
    bool audioReady = false;
};

class SlideshowDialog : public QDialog {
    Q_OBJECT

public:
    // Constructor - images come from MainWindow, not generated here
    explicit SlideshowDialog(QWidget* parent = nullptr);
    ~SlideshowDialog();

    // Set the full text and plate size (for splitting text into segments)
    void setContent(const QString& fullText, const QString& treatiseCode,
                    const QString& category, int cols, int rows);

    // Receive an image from MainWindow and start audio generation for it
    void addImage(const QPixmap& image, const QString& text, int index);

    // Called when all images from MainWindow are done
    void finishAddingImages();

    // Prepare the slideshow (create placeholders based on text segments)
    void prepareSlideshow();

    // Add a slide directly (for loading existing sessions)
    void addSlide(const QPixmap& image, const QString& text);

    // Start playback automatically
    void startAutoPlay();

    // Set pipeline controller (for generation mode)
    void setPipelineController(codex::core::PipelineController* controller) { Q_UNUSED(controller); }

signals:
    void generationCompleted();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onPlayStop();
    void onPrevious();
    void onNext();
    void onSliderMoved(int position);
    void onVolumeChanged(int value);
    void onRepeatToggled(bool checked);
    void onFullscreen();
    void onExport();
    void onLoadMedia();

    void onAudioGenerated(const QByteArray& audioData, int durationMs);
    void onAudioError(const QString& error);

    void onGenerateVideo();
    void onGenerateAllVideos();
    void onVideoGenerated(const QByteArray& videoData, const QString& prompt);
    void onVideoProgress(int percent);
    void onVideoError(const QString& error);
    void onExportVideo();  // Export slideshow as MP4 using FFmpeg

    void onSlideTimerTimeout();
    void onAudioPositionChanged(qint64 position);
    void onAudioMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    void setupUi();
    void splitTextIntoSegments();
    void generateNextAudio();
    void showSlide(int index);
    void updateProgress();
    void updateThumbnails();
    void updateSlideDisplay();
    void tryAutoStartPlayback();
    QString formatTime(int ms) const;
    QPixmap createImageWithText(int index) const;
    void exportToPdf(const QString& filePath);
    void exportToPng(const QString& folderPath);
    void exportCurrentToPng(const QString& filePath);
    void loadMediaSession(const QString& sessionPath);
    void generateVideoForSlide(int index);
    void processNextVideoInQueue();

    // UI Elements
    QLabel* m_imageLabel = nullptr;
    QLabel* m_textOverlay = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_timeLabel = nullptr;

    // Transport controls
    QPushButton* m_playStopBtn = nullptr;
    QPushButton* m_prevBtn = nullptr;
    QPushButton* m_nextBtn = nullptr;
    QPushButton* m_fullscreenBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_videoBtn = nullptr;
    QPushButton* m_videoAIBtn = nullptr;  // AI video generation with Veo
    QCheckBox* m_repeatCheck = nullptr;
    QSlider* m_positionSlider = nullptr;
    QSlider* m_volumeSlider = nullptr;

    // Progress panel
    QListWidget* m_thumbnailList = nullptr;
    QProgressBar* m_generationProgress = nullptr;
    QLabel* m_generationStatus = nullptr;

    // Voice selector
    QComboBox* m_voiceCombo = nullptr;

    // Data
    QVector<SlideItem> m_slides;
    QString m_fullText;
    QString m_treatiseCode;
    QString m_category;
    int m_cols = 2;
    int m_rows = 2;
    int m_totalExpectedImages = 0;
    int m_imagesReceived = 0;

    // Playback state
    int m_currentIndex = -1;
    bool m_isPlaying = false;
    bool m_isPaused = false;
    bool m_repeat = false;
    bool m_isFullscreen = false;
    bool m_autoStarted = false;

    // Audio generation state
    int m_nextAudioToGenerate = 0;
    bool m_allImagesReceived = false;

    // Controllers
    codex::api::EdgeTTSClient* m_ttsClient = nullptr;
    codex::api::VeoClient* m_veoClient = nullptr;

    // Audio
    QMediaPlayer* m_audioPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QTimer* m_slideTimer = nullptr;

    // Temp directory for audio files
    QString m_tempDir;

    // Video generation queue
    QVector<int> m_videoQueue;          // Slides to generate videos for
    int m_currentVideoSlide = -1;       // Currently generating slide
    bool m_generatingAllVideos = false; // Batch generation mode
    int m_videosGenerated = 0;          // Count for progress
};

} // namespace codex::ui
