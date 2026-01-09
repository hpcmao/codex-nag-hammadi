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

namespace codex::core {
class PipelineController;
}

namespace codex::api {
class EdgeTTSClient;
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
    explicit SlideshowDialog(codex::core::PipelineController* pipelineController, QWidget* parent = nullptr);
    ~SlideshowDialog();

    // Set the full text and plate size to generate
    void setContent(const QString& fullText, const QString& treatiseCode,
                    const QString& category, int cols, int rows);

    // Start generation and playback
    void startGeneration();

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

    void onImageGenerated(const QPixmap& image, const QString& prompt);
    void onImageGenerationFailed(const QString& error);
    void onAudioGenerated(const QByteArray& audioData, int durationMs);
    void onAudioError(const QString& error);

    void onSlideTimerTimeout();
    void onAudioPositionChanged(qint64 position);
    void onAudioMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    void setupUi();
    void splitTextIntoSegments();
    void generateNextImage();
    void generateNextAudio();
    void showSlide(int index);
    void updateProgress();
    void updateThumbnails();
    void updateSlideDisplay();
    void drawTextOverlay(QPixmap& image, const QString& text);
    QString formatTime(int ms) const;

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

    // Playback state
    int m_currentIndex = -1;
    bool m_isPlaying = false;
    bool m_isPaused = false;
    bool m_repeat = false;
    bool m_isFullscreen = false;

    // Generation state
    int m_nextImageToGenerate = 0;
    int m_nextAudioToGenerate = 0;
    bool m_generationInProgress = false;

    // Controllers
    codex::core::PipelineController* m_pipelineController = nullptr;
    codex::api::EdgeTTSClient* m_ttsClient = nullptr;

    // Audio
    QMediaPlayer* m_audioPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QTimer* m_slideTimer = nullptr;

    // Temp directory for audio files
    QString m_tempDir;
};

} // namespace codex::ui
