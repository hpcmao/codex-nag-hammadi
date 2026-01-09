#include "SlideshowDialog.h"
#include "core/controllers/PipelineController.h"
#include "api/EdgeTTSClient.h"
#include "utils/Logger.h"
#include "utils/Config.h"
#include "utils/SecureStorage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QApplication>
#include <QScreen>
#include <QScrollArea>

namespace codex::ui {

SlideshowDialog::SlideshowDialog(codex::core::PipelineController* pipelineController, QWidget* parent)
    : QDialog(parent)
    , m_pipelineController(pipelineController)
{
    setWindowTitle("Diaporama - Codex Nag Hammadi");
    setMinimumSize(1200, 800);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);

    // Create temp directory for audio files
    m_tempDir = QDir::tempPath() + "/codex_slideshow_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QDir().mkpath(m_tempDir);

    // Create TTS client
    m_ttsClient = new codex::api::EdgeTTSClient(this);

    // Audio player
    m_audioPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.8f);

    connect(m_audioPlayer, &QMediaPlayer::positionChanged,
            this, &SlideshowDialog::onAudioPositionChanged);
    connect(m_audioPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &SlideshowDialog::onAudioMediaStatusChanged);
    connect(m_audioPlayer, &QMediaPlayer::errorOccurred,
            this, [this](QMediaPlayer::Error error, const QString& errorString) {
                LOG_ERROR(QString("QMediaPlayer error %1: %2").arg(error).arg(errorString));
            });
    connect(m_audioPlayer, &QMediaPlayer::playbackStateChanged,
            this, [this](QMediaPlayer::PlaybackState state) {
                LOG_INFO(QString("QMediaPlayer state changed to: %1").arg(state));
            });

    // Slide timer (for slides without audio)
    m_slideTimer = new QTimer(this);
    m_slideTimer->setSingleShot(true);
    connect(m_slideTimer, &QTimer::timeout, this, &SlideshowDialog::onSlideTimerTimeout);

    // Connect pipeline signals
    connect(m_pipelineController, &codex::core::PipelineController::generationCompleted,
            this, &SlideshowDialog::onImageGenerated);
    connect(m_pipelineController, &codex::core::PipelineController::generationFailed,
            this, &SlideshowDialog::onImageGenerationFailed);

    LOG_INFO("SlideshowDialog: Connected to shared PipelineController");

    // Connect TTS signals
    connect(m_ttsClient, &codex::api::EdgeTTSClient::speechGenerated,
            this, &SlideshowDialog::onAudioGenerated);
    connect(m_ttsClient, &codex::api::EdgeTTSClient::errorOccurred,
            this, &SlideshowDialog::onAudioError);

    setupUi();
}

SlideshowDialog::~SlideshowDialog() {
    // Clean up temp files
    QDir tempDir(m_tempDir);
    tempDir.removeRecursively();
}

void SlideshowDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Top toolbar: Plate size selector and generate button
    auto* topBar = new QWidget(this);
    topBar->setStyleSheet(R"(
        QWidget {
            background-color: #2d2d2d;
            border-bottom: 1px solid #3d3d3d;
        }
        QLabel { color: #d4d4d4; background: transparent; border: none; }
        QComboBox {
            background-color: #3d3d3d;
            color: white;
            border: 1px solid #555;
            border-radius: 3px;
            padding: 5px 10px;
            min-width: 150px;
        }
        QComboBox:hover { background-color: #4d4d4d; }
        QComboBox::drop-down { border: none; }
        QProgressBar {
            background-color: #3d3d3d;
            border: none;
            border-radius: 3px;
            text-align: center;
            color: white;
        }
        QProgressBar::chunk {
            background-color: #094771;
            border-radius: 3px;
        }
    )");
    topBar->setFixedHeight(60);

    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(15, 10, 15, 10);
    topLayout->setSpacing(15);

    auto* titleLabel = new QLabel("DIAPORAMA", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #007acc;");
    topLayout->addWidget(titleLabel);

    topLayout->addStretch();

    m_generationStatus = new QLabel("Pret", this);
    m_generationStatus->setStyleSheet("color: #81c784; font-weight: bold;");
    topLayout->addWidget(m_generationStatus);

    m_generationProgress = new QProgressBar(this);
    m_generationProgress->setRange(0, 100);
    m_generationProgress->setValue(0);
    m_generationProgress->setMinimumWidth(200);
    m_generationProgress->setFixedHeight(20);
    m_generationProgress->setVisible(false);
    topLayout->addWidget(m_generationProgress);

    mainLayout->addWidget(topBar);

    // Content area with margins
    auto* contentWidget = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(10, 10, 10, 10);
    contentLayout->setSpacing(10);

    // Center: Splitter with thumbnails (left) and image (right)
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left: Thumbnails panel
    auto* leftPanel = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPanel);

    auto* thumbLabel = new QLabel("Images:", this);
    thumbLabel->setStyleSheet("font-weight: bold; color: #d4d4d4;");
    leftLayout->addWidget(thumbLabel);

    m_thumbnailList = new QListWidget(this);
    m_thumbnailList->setViewMode(QListView::ListMode);
    m_thumbnailList->setIconSize(QSize(160, 90));
    m_thumbnailList->setSpacing(5);
    m_thumbnailList->setMinimumWidth(200);
    m_thumbnailList->setMaximumWidth(220);
    m_thumbnailList->setStyleSheet(R"(
        QListWidget {
            background-color: #2d2d2d;
            border: 1px solid #3d3d3d;
        }
        QListWidget::item {
            background-color: #3d3d3d;
            border: 2px solid transparent;
            border-radius: 5px;
            padding: 5px;
            min-height: 95px;
        }
        QListWidget::item:selected {
            border-color: #007acc;
            background-color: #264f78;
        }
    )");
    connect(m_thumbnailList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0 && row < m_slides.size() && m_slides[row].imageReady) {
            showSlide(row);
        }
    });
    leftLayout->addWidget(m_thumbnailList, 1);

    splitter->addWidget(leftPanel);

    // Right: Main image display
    auto* imageContainer = new QWidget(this);
    auto* imageLayout = new QVBoxLayout(imageContainer);
    imageLayout->setContentsMargins(0, 0, 0, 0);

    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(640, 480);
    m_imageLabel->setStyleSheet("background-color: #1a1a1a; border: 1px solid #3d3d3d;");
    m_imageLabel->setText("<div style='color: #666; text-align: center; padding: 100px;'>"
                          "<p style='font-size: 24px;'>Generation en cours...</p>"
                          "<p>Les images apparaitront ici</p></div>");
    imageLayout->addWidget(m_imageLabel, 1);

    // Text overlay (at bottom of image)
    m_textOverlay = new QLabel(this);
    m_textOverlay->setAlignment(Qt::AlignCenter);
    m_textOverlay->setWordWrap(true);
    m_textOverlay->setStyleSheet(R"(
        QLabel {
            background-color: rgba(0, 0, 0, 200);
            color: white;
            padding: 20px;
            font-size: 18px;
            font-weight: bold;
            border-top: 2px solid #007acc;
        }
    )");
    m_textOverlay->setMinimumHeight(100);
    m_textOverlay->setMaximumHeight(150);
    imageLayout->addWidget(m_textOverlay);

    splitter->addWidget(imageContainer);
    splitter->setSizes({220, 900});

    contentLayout->addWidget(splitter, 1);

    // Bottom: Transport controls
    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(10);

    // Previous
    m_prevBtn = new QPushButton("<<", this);
    m_prevBtn->setFixedSize(40, 35);
    m_prevBtn->setEnabled(false);
    connect(m_prevBtn, &QPushButton::clicked, this, &SlideshowDialog::onPrevious);
    controlsLayout->addWidget(m_prevBtn);

    // Play/Stop toggle button
    m_playStopBtn = new QPushButton("Play", this);
    m_playStopBtn->setFixedSize(70, 35);
    m_playStopBtn->setEnabled(false);
    m_playStopBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #2d5a27;
            color: white;
            border: none;
            border-radius: 5px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #3d7a37; }
        QPushButton:disabled { background-color: #3d3d3d; color: #666; }
    )");
    connect(m_playStopBtn, &QPushButton::clicked, this, &SlideshowDialog::onPlayStop);
    controlsLayout->addWidget(m_playStopBtn);

    // Next
    m_nextBtn = new QPushButton(">>", this);
    m_nextBtn->setFixedSize(40, 35);
    m_nextBtn->setEnabled(false);
    connect(m_nextBtn, &QPushButton::clicked, this, &SlideshowDialog::onNext);
    controlsLayout->addWidget(m_nextBtn);

    controlsLayout->addSpacing(15);

    // Position slider
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_positionSlider->setRange(0, 100);
    m_positionSlider->setEnabled(false);
    connect(m_positionSlider, &QSlider::sliderMoved, this, &SlideshowDialog::onSliderMoved);
    controlsLayout->addWidget(m_positionSlider, 1);

    // Time label
    m_timeLabel = new QLabel("0:00 / 0:00", this);
    m_timeLabel->setStyleSheet("color: #888; font-family: monospace;");
    m_timeLabel->setMinimumWidth(90);
    controlsLayout->addWidget(m_timeLabel);

    controlsLayout->addSpacing(15);

    // Voice selector
    controlsLayout->addWidget(new QLabel("Voix:", this));
    m_voiceCombo = new QComboBox(this);
    m_voiceCombo->addItem("Henri (homme)", "fr-FR-HenriNeural");
    m_voiceCombo->addItem("Denise (femme)", "fr-FR-DeniseNeural");
    m_voiceCombo->addItem("Eloise (douce)", "fr-FR-EloiseNeural");
    m_voiceCombo->addItem("Vivienne (femme)", "fr-FR-VivienneMultilingualNeural");
    m_voiceCombo->addItem("Remy (homme)", "fr-FR-RemyMultilingualNeural");
    m_voiceCombo->setMinimumWidth(140);
    // Set current voice from config
    QString savedVoice = codex::utils::Config::instance().edgeTtsVoice();
    int voiceIdx = m_voiceCombo->findData(savedVoice);
    if (voiceIdx >= 0) m_voiceCombo->setCurrentIndex(voiceIdx);
    controlsLayout->addWidget(m_voiceCombo);

    controlsLayout->addSpacing(10);

    // Repeat
    m_repeatCheck = new QCheckBox("Boucle", this);
    connect(m_repeatCheck, &QCheckBox::toggled, this, &SlideshowDialog::onRepeatToggled);
    controlsLayout->addWidget(m_repeatCheck);

    controlsLayout->addSpacing(10);

    // Volume
    controlsLayout->addWidget(new QLabel("Vol:", this));
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(80);
    m_volumeSlider->setFixedWidth(70);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &SlideshowDialog::onVolumeChanged);
    controlsLayout->addWidget(m_volumeSlider);

    controlsLayout->addSpacing(10);

    // Fullscreen
    m_fullscreenBtn = new QPushButton("Plein ecran", this);
    m_fullscreenBtn->setFixedHeight(35);
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &SlideshowDialog::onFullscreen);
    controlsLayout->addWidget(m_fullscreenBtn);

    contentLayout->addLayout(controlsLayout);

    mainLayout->addWidget(contentWidget, 1);

    // Status bar
    m_statusLabel = new QLabel("Generation automatique en cours...", this);
    m_statusLabel->setStyleSheet("color: #888; font-size: 11px;");
    contentLayout->addWidget(m_statusLabel);
}

void SlideshowDialog::setContent(const QString& fullText, const QString& treatiseCode,
                                  const QString& category, int cols, int rows) {
    m_fullText = fullText;
    m_treatiseCode = treatiseCode;
    m_category = category;
    m_cols = cols;
    m_rows = rows;

    m_statusLabel->setText(QString("Texte charge: %1 caracteres. Planche %2x%3 = %4 images.")
                           .arg(fullText.length()).arg(cols).arg(rows).arg(cols * rows));
}

void SlideshowDialog::startGeneration() {
    if (m_fullText.isEmpty()) {
        m_statusLabel->setText("Erreur: Aucun texte selectionne.");
        return;
    }

    // Reset state
    m_slides.clear();
    m_thumbnailList->clear();
    m_currentIndex = -1;
    m_nextImageToGenerate = 0;
    m_nextAudioToGenerate = 0;
    m_generationInProgress = true;

    // Split text into segments
    splitTextIntoSegments();

    // Update UI
    m_generationProgress->setVisible(true);
    m_generationProgress->setValue(0);

    // Add placeholder items to thumbnail list
    for (int i = 0; i < m_slides.size(); ++i) {
        auto* item = new QListWidgetItem(QString("Image %1").arg(i + 1));
        item->setIcon(QIcon());  // Placeholder
        m_thumbnailList->addItem(item);
    }

    m_statusLabel->setText(QString("Generation en cours: 0/%1 images...").arg(m_slides.size()));

    // Start generating first image
    generateNextImage();
}

void SlideshowDialog::splitTextIntoSegments() {
    int totalImages = m_cols * m_rows;

    // Split text into sentences first
    QStringList sentences;
    QString currentSentence;

    for (const QChar& c : m_fullText) {
        currentSentence += c;
        if (c == '.' || c == '!' || c == '?' || c == ';') {
            sentences.append(currentSentence.trimmed());
            currentSentence.clear();
        }
    }
    if (!currentSentence.trimmed().isEmpty()) {
        sentences.append(currentSentence.trimmed());
    }

    // Distribute sentences across images
    int sentencesPerImage = qMax(1, sentences.size() / totalImages);

    m_slides.clear();

    for (int i = 0; i < totalImages; ++i) {
        SlideItem slide;

        int startIdx = i * sentencesPerImage;
        int endIdx = (i == totalImages - 1) ? sentences.size() : (i + 1) * sentencesPerImage;

        QStringList segmentSentences;
        for (int j = startIdx; j < endIdx && j < sentences.size(); ++j) {
            segmentSentences.append(sentences[j]);
        }

        if (segmentSentences.isEmpty() && !sentences.isEmpty()) {
            // Use last sentence if we run out
            segmentSentences.append(sentences.last());
        }

        slide.text = segmentSentences.join(" ");
        m_slides.append(slide);
    }

    LOG_INFO(QString("Split text into %1 segments for %2x%3 plate. Total sentences found: %4")
             .arg(m_slides.size()).arg(m_cols).arg(m_rows).arg(sentences.size()));

    for (int i = 0; i < m_slides.size(); ++i) {
        LOG_INFO(QString("Slide %1 text length: %2").arg(i).arg(m_slides[i].text.length()));
    }
}

void SlideshowDialog::generateNextImage() {
    if (m_nextImageToGenerate >= m_slides.size()) {
        // All images generated, start generating audio
        m_generationStatus->setText("Generation audio...");
        LOG_INFO("All images generated, starting audio generation");
        generateNextAudio();
        return;
    }

    int idx = m_nextImageToGenerate;
    m_generationStatus->setText(QString("Image %1/%2...").arg(idx + 1).arg(m_slides.size()));

    LOG_INFO(QString("Generating image %1/%2 for text: %3...")
             .arg(idx + 1).arg(m_slides.size()).arg(m_slides[idx].text.left(50)));

    // Generate image for this segment
    m_pipelineController->startGeneration(
        m_slides[idx].text,
        m_treatiseCode,
        m_category
    );
}

void SlideshowDialog::generateNextAudio() {
    if (m_nextAudioToGenerate >= m_slides.size()) {
        // All audio generated
        m_generationInProgress = false;
        m_generationProgress->setVisible(false);
        m_generationStatus->setText("Termine!");

        // Enable controls
        m_playStopBtn->setEnabled(true);
        m_prevBtn->setEnabled(true);
        m_nextBtn->setEnabled(true);
        m_positionSlider->setEnabled(true);

        // Show first slide and auto-play if not already playing
        if (!m_slides.isEmpty() && m_slides[0].imageReady && !m_isPlaying) {
            showSlide(0);
            // Auto-start playback
            QTimer::singleShot(500, this, &SlideshowDialog::onPlayStop);
        }

        m_statusLabel->setText(QString("Lecture: %1 images avec audio")
                               .arg(m_slides.size()));

        LOG_INFO(QString("Slideshow ready: %1 slides").arg(m_slides.size()));
        emit generationCompleted();
        return;
    }

    int idx = m_nextAudioToGenerate;
    m_generationStatus->setText(QString("Audio %1/%2...").arg(idx + 1).arg(m_slides.size()));

    // Generate TTS for this segment using selected voice
    QString voiceId = m_voiceCombo->currentData().toString();
    if (voiceId.isEmpty()) {
        voiceId = "fr-FR-HenriNeural";
    }

    codex::api::EdgeVoiceSettings settings;
    settings.voiceId = voiceId;
    settings.rate = 0;
    settings.pitch = 0;
    settings.volume = 100;

    LOG_INFO(QString("Generating audio with voice: %1").arg(voiceId));
    m_ttsClient->generateSpeech(m_slides[idx].text, settings);
}

void SlideshowDialog::onImageGenerated(const QPixmap& image, const QString& prompt) {
    Q_UNUSED(prompt)

    LOG_INFO(QString("SlideshowDialog::onImageGenerated called, generationInProgress=%1, nextIdx=%2, slides=%3, image=%4x%5")
             .arg(m_generationInProgress).arg(m_nextImageToGenerate).arg(m_slides.size())
             .arg(image.width()).arg(image.height()));

    // Ignore signals if we're not actively generating for the slideshow
    if (!m_generationInProgress) {
        LOG_INFO("SlideshowDialog: Not in generation mode, ignoring signal");
        return;
    }

    if (image.isNull()) {
        LOG_WARN("SlideshowDialog: Received null image, ignoring");
        return;
    }

    if (m_nextImageToGenerate >= m_slides.size()) {
        LOG_WARN("SlideshowDialog: nextImageToGenerate >= slides.size(), ignoring");
        return;
    }

    int idx = m_nextImageToGenerate;

    // Store the image
    m_slides[idx].image = image;
    m_slides[idx].imageReady = true;

    // Update thumbnail
    if (idx < m_thumbnailList->count()) {
        QPixmap thumb = image.scaled(160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_thumbnailList->item(idx)->setIcon(QIcon(thumb));
        m_thumbnailList->item(idx)->setText(QString("Image %1 (Prete)").arg(idx + 1));
        LOG_INFO(QString("Thumbnail updated for index %1").arg(idx));
    }

    // Update progress
    updateProgress();

    // Show this image if it's the first one
    if (m_currentIndex < 0) {
        showSlide(idx);
    }

    // Generate next image
    m_nextImageToGenerate++;
    generateNextImage();
}

void SlideshowDialog::onImageGenerationFailed(const QString& error) {
    LOG_ERROR(QString("Slideshow image generation failed: %1").arg(error));

    // Ignore if not in generation mode
    if (!m_generationInProgress) {
        LOG_INFO("SlideshowDialog: Not in generation mode, ignoring error signal");
        return;
    }

    // Create placeholder image
    if (m_nextImageToGenerate < m_slides.size()) {
        QPixmap placeholder(1024, 576);
        placeholder.fill(QColor(40, 40, 40));
        QPainter painter(&placeholder);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 16));
        painter.drawText(placeholder.rect(), Qt::AlignCenter,
                        QString("Erreur generation\n%1").arg(error.left(50)));
        painter.end();

        m_slides[m_nextImageToGenerate].image = placeholder;
        m_slides[m_nextImageToGenerate].imageReady = true;

        if (m_nextImageToGenerate < m_thumbnailList->count()) {
            QPixmap thumb = placeholder.scaled(160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_thumbnailList->item(m_nextImageToGenerate)->setIcon(QIcon(thumb));
        }
    }

    updateProgress();
    m_nextImageToGenerate++;
    generateNextImage();
}

void SlideshowDialog::onAudioGenerated(const QByteArray& audioData, int durationMs) {
    // Ignore if not in generation mode
    if (!m_generationInProgress) return;
    if (m_nextAudioToGenerate >= m_slides.size()) return;

    int idx = m_nextAudioToGenerate;

    // Save audio to temp file
    QString audioPath = QString("%1/slide_%2.mp3").arg(m_tempDir).arg(idx);
    QFile file(audioPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(audioData);
        file.close();

        m_slides[idx].audioPath = audioPath;
        m_slides[idx].audioDurationMs = durationMs;
        m_slides[idx].audioReady = true;
    }

    // Update progress
    updateProgress();

    // Auto-start playback as soon as first slide has both image and audio ready
    if (idx == 0 && !m_isPlaying && m_slides[0].imageReady && m_slides[0].audioReady) {
        LOG_INFO("First slide ready with audio, auto-starting playback");
        m_playStopBtn->setEnabled(true);
        m_prevBtn->setEnabled(true);
        m_nextBtn->setEnabled(true);
        m_positionSlider->setEnabled(true);
        showSlide(0);
        QTimer::singleShot(100, this, &SlideshowDialog::onPlayStop);
    }

    // Generate next audio
    m_nextAudioToGenerate++;
    generateNextAudio();
}

void SlideshowDialog::onAudioError(const QString& error) {
    LOG_ERROR(QString("Slideshow audio generation failed: %1").arg(error));

    // Ignore if not in generation mode
    if (!m_generationInProgress) return;

    if (m_nextAudioToGenerate < m_slides.size()) {
        m_slides[m_nextAudioToGenerate].audioDurationMs = 5000;  // Default 5 seconds
        m_slides[m_nextAudioToGenerate].audioReady = true;
    }

    updateProgress();
    m_nextAudioToGenerate++;
    generateNextAudio();
}

void SlideshowDialog::showSlide(int index) {
    LOG_INFO(QString("SlideshowDialog::showSlide(%1) called, slides=%2").arg(index).arg(m_slides.size()));

    if (index < 0 || index >= m_slides.size()) {
        LOG_WARN(QString("showSlide: invalid index %1").arg(index));
        return;
    }
    if (!m_slides[index].imageReady) {
        LOG_WARN(QString("showSlide: image %1 not ready").arg(index));
        return;
    }

    m_currentIndex = index;

    // Display image - use a reasonable minimum size if widget not laid out yet
    QSize targetSize = m_imageLabel->size();
    LOG_INFO(QString("m_imageLabel size: %1x%2").arg(targetSize.width()).arg(targetSize.height()));
    if (targetSize.width() < 100 || targetSize.height() < 100) {
        targetSize = QSize(800, 600);
    }

    QPixmap display = m_slides[index].image.scaled(
        targetSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    m_imageLabel->setPixmap(display);

    LOG_INFO(QString("Showing slide %1, image size: %2x%3, display size: %4x%5")
             .arg(index)
             .arg(m_slides[index].image.width())
             .arg(m_slides[index].image.height())
             .arg(display.width())
             .arg(display.height()));

    // Display text overlay
    m_textOverlay->setText(m_slides[index].text);

    // Update thumbnail selection
    m_thumbnailList->setCurrentRow(index);

    // Update slider
    if (m_slides.size() > 1) {
        m_positionSlider->blockSignals(true);
        m_positionSlider->setValue(index * 100 / (m_slides.size() - 1));
        m_positionSlider->blockSignals(false);
    }

    updateSlideDisplay();
}

void SlideshowDialog::updateProgress() {
    int imagesReady = 0;
    int audioReady = 0;

    for (const auto& slide : m_slides) {
        if (slide.imageReady) imagesReady++;
        if (slide.audioReady) audioReady++;
    }

    int total = m_slides.size() * 2;  // Images + Audio
    int done = imagesReady + audioReady;

    m_generationProgress->setValue(done * 100 / total);
}

void SlideshowDialog::updateThumbnails() {
    // Already updated in onImageGenerated
}

void SlideshowDialog::updateSlideDisplay() {
    if (m_currentIndex < 0) return;

    m_statusLabel->setText(QString("Diapo %1/%2").arg(m_currentIndex + 1).arg(m_slides.size()));
}

void SlideshowDialog::onPlayStop() {
    if (m_slides.isEmpty()) return;

    if (m_isPlaying) {
        // Stop playback
        m_isPlaying = false;
        m_isPaused = false;
        m_audioPlayer->stop();
        m_slideTimer->stop();

        m_playStopBtn->setText("Play");
        m_playStopBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #2d5a27;
                color: white;
                border: none;
                border-radius: 5px;
                font-weight: bold;
            }
            QPushButton:hover { background-color: #3d7a37; }
            QPushButton:disabled { background-color: #3d3d3d; color: #666; }
        )");
    } else {
        // Start playback
        m_isPlaying = true;
        m_isPaused = false;

        if (m_currentIndex < 0) {
            showSlide(0);
        }

        // Start audio or timer for current slide
        if (m_currentIndex >= 0 && m_currentIndex < m_slides.size()) {
            const SlideItem& slide = m_slides[m_currentIndex];
            LOG_INFO(QString("Playing slide %1, audioPath=%2, audioReady=%3")
                     .arg(m_currentIndex).arg(slide.audioPath).arg(slide.audioReady));
            if (!slide.audioPath.isEmpty() && QFile::exists(slide.audioPath)) {
                QFileInfo fi(slide.audioPath);
                LOG_INFO(QString("Audio file exists, size=%1 bytes").arg(fi.size()));
                m_audioPlayer->setSource(QUrl::fromLocalFile(slide.audioPath));
                m_audioPlayer->play();
                LOG_INFO("Called m_audioPlayer->play()");
            } else {
                LOG_WARN(QString("No audio file, using timer. Path=%1, exists=%2")
                         .arg(slide.audioPath).arg(QFile::exists(slide.audioPath)));
                m_slideTimer->start(slide.audioDurationMs > 0 ? slide.audioDurationMs : 5000);
            }
        }

        m_playStopBtn->setText("Stop");
        m_playStopBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #8b0000;
                color: white;
                border: none;
                border-radius: 5px;
                font-weight: bold;
            }
            QPushButton:hover { background-color: #a52a2a; }
            QPushButton:disabled { background-color: #3d3d3d; color: #666; }
        )");
    }
}

void SlideshowDialog::onPrevious() {
    if (m_currentIndex > 0) {
        m_audioPlayer->stop();
        m_slideTimer->stop();
        showSlide(m_currentIndex - 1);

        if (m_isPlaying && !m_isPaused) {
            // Continue playing
            const SlideItem& slide = m_slides[m_currentIndex];
            if (!slide.audioPath.isEmpty() && QFile::exists(slide.audioPath)) {
                m_audioPlayer->setSource(QUrl::fromLocalFile(slide.audioPath));
                m_audioPlayer->play();
            } else {
                m_slideTimer->start(slide.audioDurationMs > 0 ? slide.audioDurationMs : 5000);
            }
        }
    }
}

void SlideshowDialog::onNext() {
    if (m_currentIndex < m_slides.size() - 1) {
        m_audioPlayer->stop();
        m_slideTimer->stop();
        showSlide(m_currentIndex + 1);

        if (m_isPlaying && !m_isPaused) {
            // Continue playing
            const SlideItem& slide = m_slides[m_currentIndex];
            if (!slide.audioPath.isEmpty() && QFile::exists(slide.audioPath)) {
                m_audioPlayer->setSource(QUrl::fromLocalFile(slide.audioPath));
                m_audioPlayer->play();
            } else {
                m_slideTimer->start(slide.audioDurationMs > 0 ? slide.audioDurationMs : 5000);
            }
        }
    } else if (m_repeat && m_isPlaying) {
        showSlide(0);
        const SlideItem& slide = m_slides[0];
        if (!slide.audioPath.isEmpty() && QFile::exists(slide.audioPath)) {
            m_audioPlayer->setSource(QUrl::fromLocalFile(slide.audioPath));
            m_audioPlayer->play();
        } else {
            m_slideTimer->start(slide.audioDurationMs > 0 ? slide.audioDurationMs : 5000);
        }
    }
}

void SlideshowDialog::onSliderMoved(int position) {
    if (m_slides.isEmpty()) return;

    int index = position * (m_slides.size() - 1) / 100;
    if (index != m_currentIndex && m_slides[index].imageReady) {
        m_audioPlayer->stop();
        m_slideTimer->stop();
        showSlide(index);
    }
}

void SlideshowDialog::onVolumeChanged(int value) {
    m_audioOutput->setVolume(value / 100.0f);
}

void SlideshowDialog::onRepeatToggled(bool checked) {
    m_repeat = checked;
}

void SlideshowDialog::onFullscreen() {
    if (m_isFullscreen) {
        showNormal();
        m_fullscreenBtn->setText("Plein ecran");
        m_isFullscreen = false;
    } else {
        showFullScreen();
        m_fullscreenBtn->setText("Fenetre");
        m_isFullscreen = true;
    }
}

void SlideshowDialog::onSlideTimerTimeout() {
    // Timer finished, move to next slide
    if (m_isPlaying && !m_isPaused) {
        onNext();
    }
}

void SlideshowDialog::onAudioPositionChanged(qint64 position) {
    if (m_currentIndex < 0 || m_currentIndex >= m_slides.size()) return;

    int totalDuration = 0;
    for (const auto& slide : m_slides) {
        totalDuration += slide.audioDurationMs > 0 ? slide.audioDurationMs : 5000;
    }

    int currentPosition = 0;
    for (int i = 0; i < m_currentIndex; ++i) {
        currentPosition += m_slides[i].audioDurationMs > 0 ? m_slides[i].audioDurationMs : 5000;
    }
    currentPosition += static_cast<int>(position);

    m_timeLabel->setText(QString("%1 / %2")
                         .arg(formatTime(currentPosition))
                         .arg(formatTime(totalDuration)));
}

void SlideshowDialog::onAudioMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
        // Audio finished, move to next slide
        if (m_isPlaying && !m_isPaused) {
            onNext();
        }
    }
}

void SlideshowDialog::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Space:
            onPlayStop();
            break;

        case Qt::Key_Right:
        case Qt::Key_Down:
            onNext();
            break;

        case Qt::Key_Left:
        case Qt::Key_Up:
            onPrevious();
            break;

        case Qt::Key_Escape:
            if (m_isFullscreen) {
                onFullscreen();
            } else {
                close();
            }
            break;

        case Qt::Key_F11:
        case Qt::Key_F:
            onFullscreen();
            break;

        default:
            QDialog::keyPressEvent(event);
    }
}

void SlideshowDialog::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);

    // Rescale current image
    if (m_currentIndex >= 0 && m_currentIndex < m_slides.size() && m_slides[m_currentIndex].imageReady) {
        QPixmap display = m_slides[m_currentIndex].image.scaled(
            m_imageLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        m_imageLabel->setPixmap(display);
    }
}

void SlideshowDialog::closeEvent(QCloseEvent* event) {
    // Stop playback
    m_isPlaying = false;
    m_isPaused = false;
    m_audioPlayer->stop();
    m_slideTimer->stop();
    m_generationInProgress = false;
    QDialog::closeEvent(event);
}

QString SlideshowDialog::formatTime(int ms) const {
    int seconds = ms / 1000;
    int minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

void SlideshowDialog::drawTextOverlay(QPixmap& image, const QString& text) {
    // Not used - we have a separate QLabel for overlay
    Q_UNUSED(image)
    Q_UNUSED(text)
}

} // namespace codex::ui
