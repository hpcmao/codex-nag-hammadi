#include "SlideshowDialog.h"
#include "SessionPickerDialog.h"
#include "VideoPreviewDialog.h"
#include "api/EdgeTTSClient.h"
#include "api/VeoClient.h"
#include "core/services/NarrationCleaner.h"
#include "utils/Logger.h"
#include "utils/Config.h"
#include "utils/MediaStorage.h"
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
#include <QDateTime>
#include <QApplication>
#include <QScreen>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QPdfWriter>
#include <QTextDocument>
#include <QMenu>
#include <QProcess>
#include <QTemporaryDir>
#include <QBuffer>
#include <QClipboard>
#include <QTextEdit>

namespace codex::ui {

// Helper function to show error dialog with copyable text
static void showCopyableError(QWidget* parent, const QString& title, const QString& message) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setMinimumSize(500, 200);

    auto* layout = new QVBoxLayout(&dialog);

    auto* textEdit = new QTextEdit(&dialog);
    textEdit->setPlainText(message);
    textEdit->setReadOnly(true);
    textEdit->setStyleSheet("QTextEdit { background-color: #2d2d2d; color: #ff6b6b; font-family: monospace; }");
    layout->addWidget(textEdit);

    auto* btnLayout = new QHBoxLayout();
    auto* copyBtn = new QPushButton("Copier", &dialog);
    auto* okBtn = new QPushButton("OK", &dialog);
    okBtn->setDefault(true);
    btnLayout->addStretch();
    btnLayout->addWidget(copyBtn);
    btnLayout->addWidget(okBtn);
    layout->addLayout(btnLayout);

    QObject::connect(copyBtn, &QPushButton::clicked, [message]() {
        QApplication::clipboard()->setText(message);
    });
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

SlideshowDialog::SlideshowDialog(QWidget* parent)
    : QDialog(parent)
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

    // Connect TTS signals
    connect(m_ttsClient, &codex::api::EdgeTTSClient::speechGenerated,
            this, &SlideshowDialog::onAudioGenerated);
    connect(m_ttsClient, &codex::api::EdgeTTSClient::errorOccurred,
            this, &SlideshowDialog::onAudioError);

    // Create Veo client for video generation (uses Gemini API key)
    m_veoClient = new codex::api::VeoClient(this);

    // Use Gemini AI Studio API key (Paid Tier for Veo)
    QString apiKey = codex::utils::SecureStorage::instance().getApiKey(
        codex::utils::SecureStorage::SERVICE_AISTUDIO);
    if (!apiKey.isEmpty()) {
        m_veoClient->setApiKey(apiKey);
        LOG_INFO("Veo: Using Gemini AI Studio API key (Paid Tier)");
    } else {
        LOG_INFO("Veo: No Gemini API key configured (Video IA disabled)");
    }

    // Connect Veo signals
    connect(m_veoClient, &codex::api::VeoClient::videoGenerated,
            this, &SlideshowDialog::onVideoGenerated);
    connect(m_veoClient, &codex::api::VeoClient::generationProgress,
            this, &SlideshowDialog::onVideoProgress);
    connect(m_veoClient, &codex::api::VeoClient::errorOccurred,
            this, &SlideshowDialog::onVideoError);

    setupUi();

    LOG_INFO("SlideshowDialog created (passive mode - receives images from MainWindow)");
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

    // Top toolbar with all controls
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
            padding: 4px 8px;
            min-width: 120px;
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
        QPushButton {
            background-color: #3d3d3d;
            color: white;
            border: 1px solid #555;
            border-radius: 4px;
            padding: 5px 10px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #4d4d4d; }
        QPushButton:disabled { background-color: #2d2d2d; color: #666; border-color: #3d3d3d; }
        QCheckBox { color: #d4d4d4; background: transparent; }
        QSlider::groove:horizontal {
            height: 6px;
            background: #3d3d3d;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #007acc;
            width: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QSlider::sub-page:horizontal {
            background: #094771;
            border-radius: 3px;
        }
    )");
    topBar->setFixedHeight(50);

    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(10, 5, 10, 5);
    topLayout->setSpacing(8);

    // Previous button
    m_prevBtn = new QPushButton("<<", this);
    m_prevBtn->setFixedSize(36, 32);
    m_prevBtn->setEnabled(false);
    connect(m_prevBtn, &QPushButton::clicked, this, &SlideshowDialog::onPrevious);
    topLayout->addWidget(m_prevBtn);

    // Play/Stop toggle button
    m_playStopBtn = new QPushButton("Play", this);
    m_playStopBtn->setFixedSize(60, 32);
    m_playStopBtn->setEnabled(false);
    m_playStopBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #2d5a27;
            color: white;
            border: none;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #3d7a37; }
        QPushButton:disabled { background-color: #3d3d3d; color: #666; }
    )");
    connect(m_playStopBtn, &QPushButton::clicked, this, &SlideshowDialog::onPlayStop);
    topLayout->addWidget(m_playStopBtn);

    // Next button
    m_nextBtn = new QPushButton(">>", this);
    m_nextBtn->setFixedSize(36, 32);
    m_nextBtn->setEnabled(false);
    connect(m_nextBtn, &QPushButton::clicked, this, &SlideshowDialog::onNext);
    topLayout->addWidget(m_nextBtn);

    topLayout->addSpacing(5);

    // Position slider
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_positionSlider->setRange(0, 100);
    m_positionSlider->setEnabled(false);
    m_positionSlider->setMinimumWidth(150);
    m_positionSlider->setMaximumWidth(300);
    connect(m_positionSlider, &QSlider::sliderMoved, this, &SlideshowDialog::onSliderMoved);
    topLayout->addWidget(m_positionSlider);

    // Time label
    m_timeLabel = new QLabel("0:00 / 0:00", this);
    m_timeLabel->setStyleSheet("color: #888; font-family: monospace; font-size: 11px;");
    m_timeLabel->setMinimumWidth(75);
    topLayout->addWidget(m_timeLabel);

    topLayout->addSpacing(10);

    // Repeat checkbox
    m_repeatCheck = new QCheckBox("Boucle", this);
    connect(m_repeatCheck, &QCheckBox::toggled, this, &SlideshowDialog::onRepeatToggled);
    topLayout->addWidget(m_repeatCheck);

    topLayout->addSpacing(10);

    // Voice selector
    auto* voixLabel = new QLabel("Voix:", this);
    voixLabel->setStyleSheet("font-size: 11px;");
    topLayout->addWidget(voixLabel);
    m_voiceCombo = new QComboBox(this);
    m_voiceCombo->addItem("Henri", "fr-FR-HenriNeural");
    m_voiceCombo->addItem("Denise", "fr-FR-DeniseNeural");
    m_voiceCombo->addItem("Eloise", "fr-FR-EloiseNeural");
    m_voiceCombo->addItem("Vivienne", "fr-FR-VivienneMultilingualNeural");
    m_voiceCombo->addItem("Remy", "fr-FR-RemyMultilingualNeural");
    m_voiceCombo->setMinimumWidth(90);
    QString savedVoice = codex::utils::Config::instance().edgeTtsVoice();
    int voiceIdx = m_voiceCombo->findData(savedVoice);
    if (voiceIdx >= 0) m_voiceCombo->setCurrentIndex(voiceIdx);
    topLayout->addWidget(m_voiceCombo);

    topLayout->addSpacing(10);

    // Volume
    auto* volLabel = new QLabel("Vol:", this);
    volLabel->setStyleSheet("font-size: 11px;");
    topLayout->addWidget(volLabel);
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(80);
    m_volumeSlider->setFixedWidth(60);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &SlideshowDialog::onVolumeChanged);
    topLayout->addWidget(m_volumeSlider);

    topLayout->addStretch();

    // Generation status and progress
    m_generationStatus = new QLabel("En attente...", this);
    m_generationStatus->setStyleSheet("color: #81c784; font-weight: bold; font-size: 11px;");
    topLayout->addWidget(m_generationStatus);

    m_generationProgress = new QProgressBar(this);
    m_generationProgress->setRange(0, 100);
    m_generationProgress->setValue(0);
    m_generationProgress->setMinimumWidth(200);
    m_generationProgress->setMaximumWidth(300);
    m_generationProgress->setFixedHeight(22);
    m_generationProgress->setVisible(true);
    m_generationProgress->setStyleSheet(R"(
        QProgressBar {
            border: 1px solid #3d3d3d;
            border-radius: 4px;
            background-color: #2d2d2d;
            text-align: center;
            color: #fff;
            font-weight: bold;
        }
        QProgressBar::chunk {
            background-color: #1e5a1e;
            border-radius: 3px;
        }
    )");
    topLayout->addWidget(m_generationProgress);

    topLayout->addSpacing(10);

    // Load media button
    m_loadBtn = new QPushButton("Charger", this);
    m_loadBtn->setFixedHeight(32);
    m_loadBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #1e4a5a;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 5px 12px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #2d6a7a; }
    )");
    connect(m_loadBtn, &QPushButton::clicked, this, &SlideshowDialog::onLoadMedia);
    topLayout->addWidget(m_loadBtn);

    // Export button
    m_exportBtn = new QPushButton("Exporter", this);
    m_exportBtn->setFixedHeight(32);
    m_exportBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #1e5a1e;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 5px 12px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #2d7a2d; }
    )");
    connect(m_exportBtn, &QPushButton::clicked, this, &SlideshowDialog::onExport);
    topLayout->addWidget(m_exportBtn);

    // Video export button (FFmpeg)
    m_videoBtn = new QPushButton("Exporter Video", this);
    m_videoBtn->setFixedHeight(32);
    m_videoBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #5a1e5a;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 5px 12px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #7a2d7a; }
        QPushButton:disabled { background-color: #3a3a3a; color: #888; }
    )");
    m_videoBtn->setToolTip("Exporter le diaporama en video MP4 (images fixes + audio) - Gratuit");
    connect(m_videoBtn, &QPushButton::clicked, this, &SlideshowDialog::onExportVideo);
    topLayout->addWidget(m_videoBtn);

    // AI Video generation button (Veo)
    m_videoAIBtn = new QPushButton("Video IA", this);
    m_videoAIBtn->setFixedHeight(32);
    m_videoAIBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #1e3a5a;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 5px 12px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #2d5a7a; }
        QPushButton:disabled { background-color: #3a3a3a; color: #888; }
    )");
    m_videoAIBtn->setToolTip("Generer une video IA animee (Veo 3.1) - Choisir l'image - ~$3.20 pour 8s");
    connect(m_videoAIBtn, &QPushButton::clicked, this, &SlideshowDialog::onGenerateVideo);
    topLayout->addWidget(m_videoAIBtn);

    // Fullscreen button
    m_fullscreenBtn = new QPushButton("Plein ecran", this);
    m_fullscreenBtn->setFixedHeight(32);
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &SlideshowDialog::onFullscreen);
    topLayout->addWidget(m_fullscreenBtn);

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
                          "<p style='font-size: 24px;'>En attente des images...</p>"
                          "<p>Les images apparaitront au fur et a mesure de leur generation</p></div>");
    imageLayout->addWidget(m_imageLabel, 1);

    // Text overlay removed - text is embedded in exported images only

    splitter->addWidget(imageContainer);
    splitter->setSizes({220, 900});

    contentLayout->addWidget(splitter, 1);

    mainLayout->addWidget(contentWidget, 1);

    // Status bar
    m_statusLabel = new QLabel("En attente des images de la generation...", this);
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
    m_totalExpectedImages = cols * rows;

    m_statusLabel->setText(QString("Texte charge: %1 caracteres. Planche %2x%3 = %4 images attendues.")
                           .arg(fullText.length()).arg(cols).arg(rows).arg(m_totalExpectedImages));
}

void SlideshowDialog::prepareSlideshow() {
    // Reset state
    m_slides.clear();
    m_thumbnailList->clear();
    m_currentIndex = -1;
    m_nextAudioToGenerate = 0;
    m_imagesReceived = 0;
    m_allImagesReceived = false;
    m_autoStarted = false;

    // Split text into segments to match expected images
    splitTextIntoSegments();

    // Create placeholder items in thumbnail list
    for (int i = 0; i < m_slides.size(); ++i) {
        auto* item = new QListWidgetItem(QString("Image %1 (en attente)").arg(i + 1));
        item->setIcon(QIcon());  // Placeholder
        m_thumbnailList->addItem(item);
    }

    // Update UI
    m_generationProgress->setVisible(true);
    m_generationProgress->setValue(0);
    m_generationStatus->setText(QString("0/%1 images").arg(m_totalExpectedImages));

    LOG_INFO(QString("Slideshow prepared: expecting %1 images").arg(m_totalExpectedImages));
}

void SlideshowDialog::splitTextIntoSegments() {
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

    // Distribute sentences across expected images
    int sentencesPerImage = qMax(1, sentences.size() / m_totalExpectedImages);

    m_slides.clear();

    for (int i = 0; i < m_totalExpectedImages; ++i) {
        SlideItem slide;

        int startIdx = i * sentencesPerImage;
        int endIdx = (i == m_totalExpectedImages - 1) ? sentences.size() : (i + 1) * sentencesPerImage;

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

    LOG_INFO(QString("Split text into %1 segments. Total sentences found: %2")
             .arg(m_slides.size()).arg(sentences.size()));
}

void SlideshowDialog::addImage(const QPixmap& image, const QString& text, int index) {
    LOG_INFO(QString("SlideshowDialog::addImage called: index=%1, image=%2x%3")
             .arg(index).arg(image.width()).arg(image.height()));

    if (index < 0 || index >= m_slides.size()) {
        LOG_WARN(QString("addImage: invalid index %1 (slides size: %2)").arg(index).arg(m_slides.size()));
        return;
    }

    if (image.isNull()) {
        LOG_WARN(QString("addImage: received null image for index %1").arg(index));
        return;
    }

    // Store the image
    m_slides[index].image = image;
    m_slides[index].imageReady = true;
    if (!text.isEmpty()) {
        m_slides[index].text = text;
    }
    m_imagesReceived++;

    // Update thumbnail
    if (index < m_thumbnailList->count()) {
        QPixmap thumb = image.scaled(160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_thumbnailList->item(index)->setIcon(QIcon(thumb));
        m_thumbnailList->item(index)->setText(QString("Image %1 (prete)").arg(index + 1));
    }

    // Update progress
    m_generationStatus->setText(QString("%1/%2 images").arg(m_imagesReceived).arg(m_totalExpectedImages));
    updateProgress();

    // Show this image if it's the first one received
    if (m_currentIndex < 0) {
        showSlide(index);
    }

    // Start audio generation for this image if not already generating
    // We generate audio in order, so check if this is the next one needed
    if (index == m_nextAudioToGenerate) {
        generateNextAudio();
    }

    LOG_INFO(QString("Image %1 added. Total received: %2/%3")
             .arg(index).arg(m_imagesReceived).arg(m_totalExpectedImages));
}

void SlideshowDialog::finishAddingImages() {
    m_allImagesReceived = true;
    LOG_INFO(QString("All images received: %1 total").arg(m_imagesReceived));

    // Update status
    m_generationStatus->setText(QString("%1 images").arg(m_imagesReceived));

    // If we haven't started audio generation yet, start now
    if (m_nextAudioToGenerate == 0 && !m_slides.isEmpty() && m_slides[0].imageReady) {
        generateNextAudio();
    }
}

void SlideshowDialog::generateNextAudio() {
    // Find next slide that has image but no audio yet
    while (m_nextAudioToGenerate < m_slides.size()) {
        if (!m_slides[m_nextAudioToGenerate].imageReady) {
            // Image not ready yet, wait
            LOG_INFO(QString("Audio gen: waiting for image %1").arg(m_nextAudioToGenerate));
            return;
        }
        if (!m_slides[m_nextAudioToGenerate].audioReady) {
            // Found one that needs audio
            break;
        }
        m_nextAudioToGenerate++;
    }

    if (m_nextAudioToGenerate >= m_slides.size()) {
        // All audio generated
        LOG_INFO("All audio generated");
        m_generationProgress->setVisible(false);
        m_generationStatus->setText("Termine!");

        // Enable all controls
        m_playStopBtn->setEnabled(true);
        m_prevBtn->setEnabled(true);
        m_nextBtn->setEnabled(true);
        m_positionSlider->setEnabled(true);

        m_statusLabel->setText(QString("Lecture: %1 images avec audio").arg(m_slides.size()));

        // Auto-start if not already playing
        if (!m_isPlaying && !m_autoStarted) {
            m_autoStarted = true;
            if (m_currentIndex < 0 && !m_slides.isEmpty()) {
                showSlide(0);
            }
            QTimer::singleShot(300, this, &SlideshowDialog::onPlayStop);
        }

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

    // Nettoyer le texte pour la narration
    codex::core::NarrationCleaner cleaner;
    QString cleanedText = cleaner.clean(m_slides[idx].text);

    LOG_INFO(QString("Generating audio %1/%2 with voice: %3 (cleaned: %4 -> %5 chars)")
             .arg(idx + 1).arg(m_slides.size()).arg(voiceId)
             .arg(m_slides[idx].text.length()).arg(cleanedText.length()));
    m_ttsClient->generateSpeech(cleanedText, settings);
}

void SlideshowDialog::onAudioGenerated(const QByteArray& audioData, int durationMs) {
    if (m_nextAudioToGenerate >= m_slides.size()) {
        LOG_WARN("onAudioGenerated: nextAudioToGenerate out of range");
        return;
    }

    int idx = m_nextAudioToGenerate;

    // Save audio to temp file for immediate playback
    QString audioPath = QString("%1/slide_%2.mp3").arg(m_tempDir).arg(idx);
    QFile file(audioPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(audioData);
        file.close();

        m_slides[idx].audioPath = audioPath;
        m_slides[idx].audioDurationMs = durationMs;
        m_slides[idx].audioReady = true;

        LOG_INFO(QString("Audio %1 saved: %2, duration=%3ms")
                 .arg(idx).arg(audioPath).arg(durationMs));

        // Also save to permanent MediaStorage session
        auto& storage = codex::utils::MediaStorage::instance();
        QString permanentPath = storage.saveAudio(audioData, idx);
        if (!permanentPath.isEmpty()) {
            LOG_INFO(QString("Audio %1 also saved to MediaStorage: %2").arg(idx).arg(permanentPath));
        }
    } else {
        LOG_ERROR(QString("Failed to save audio file: %1").arg(audioPath));
    }

    // Update progress
    updateProgress();

    // Try auto-start playback when first slide is fully ready
    tryAutoStartPlayback();

    // Generate next audio
    m_nextAudioToGenerate++;
    generateNextAudio();
}

void SlideshowDialog::onAudioError(const QString& error) {
    LOG_ERROR(QString("Audio generation failed: %1").arg(error));

    if (m_nextAudioToGenerate < m_slides.size()) {
        // Mark as ready with default duration (will use timer instead of audio)
        m_slides[m_nextAudioToGenerate].audioDurationMs = 5000;
        m_slides[m_nextAudioToGenerate].audioReady = true;
    }

    updateProgress();
    tryAutoStartPlayback();

    m_nextAudioToGenerate++;
    generateNextAudio();
}

void SlideshowDialog::tryAutoStartPlayback() {
    // Auto-start when first slide has both image and audio ready
    if (!m_autoStarted && !m_isPlaying &&
        !m_slides.isEmpty() && m_slides[0].imageReady && m_slides[0].audioReady) {

        LOG_INFO("First slide ready with audio, auto-starting playback");
        m_autoStarted = true;

        // Enable controls
        m_playStopBtn->setEnabled(true);
        m_prevBtn->setEnabled(true);
        m_nextBtn->setEnabled(true);
        m_positionSlider->setEnabled(true);

        // Show first slide and start playing
        showSlide(0);
        QTimer::singleShot(200, this, &SlideshowDialog::onPlayStop);
    }
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

    // Display image with text embedded
    QSize targetSize = m_imageLabel->size();
    if (targetSize.width() < 100 || targetSize.height() < 100) {
        targetSize = QSize(800, 600);
    }

    // Create image with text overlay
    QPixmap imageWithText = createImageWithText(index);
    QPixmap display = imageWithText.scaled(
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

    if (total > 0) {
        m_generationProgress->setValue(done * 100 / total);
    }
}

void SlideshowDialog::updateThumbnails() {
    // Already updated in addImage
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
        // Check if next slide is ready
        if (!m_slides[m_currentIndex + 1].imageReady) {
            LOG_INFO("Next slide not ready yet, waiting...");
            return;
        }

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
    if (index != m_currentIndex && index < m_slides.size() && m_slides[index].imageReady) {
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

    // Rescale current image with text
    if (m_currentIndex >= 0 && m_currentIndex < m_slides.size() && m_slides[m_currentIndex].imageReady) {
        QPixmap imageWithText = createImageWithText(m_currentIndex);
        QPixmap display = imageWithText.scaled(
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
    QDialog::closeEvent(event);
}

QString SlideshowDialog::formatTime(int ms) const {
    int seconds = ms / 1000;
    int minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

QPixmap SlideshowDialog::createImageWithText(int index) const {
    if (index < 0 || index >= m_slides.size() || !m_slides[index].imageReady) {
        return QPixmap();
    }

    const SlideItem& slide = m_slides[index];
    QPixmap result = slide.image.copy();

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Calculate text area at bottom (20% of image height)
    int textHeight = result.height() / 5;
    QRect textRect(0, result.height() - textHeight, result.width(), textHeight);

    // Draw semi-transparent background
    painter.fillRect(textRect, QColor(0, 0, 0, 200));

    // Draw blue top border
    painter.setPen(QPen(QColor(0, 122, 204), 3));
    painter.drawLine(textRect.topLeft(), textRect.topRight());

    // Draw text
    painter.setPen(Qt::white);
    QFont font("Arial", 16, QFont::Bold);
    painter.setFont(font);

    QRect textPadded = textRect.adjusted(20, 15, -20, -15);
    painter.drawText(textPadded, Qt::AlignCenter | Qt::TextWordWrap, slide.text);

    painter.end();
    return result;
}

void SlideshowDialog::onExport() {
    if (m_slides.isEmpty()) {
        QMessageBox::warning(this, "Export", "Aucune image a exporter.");
        return;
    }

    // Check if any images are ready
    bool hasImages = false;
    for (const auto& slide : m_slides) {
        if (slide.imageReady) {
            hasImages = true;
            break;
        }
    }
    if (!hasImages) {
        QMessageBox::warning(this, "Export", "Aucune image prete pour l'export.");
        return;
    }

    // Show menu with export options
    QMenu menu(this);
    QAction* pdfAction = menu.addAction("Exporter tout en PDF (texte searchable)");
    QAction* pngAllAction = menu.addAction("Exporter tout en PNG (images avec texte)");
    QAction* pngCurrentAction = menu.addAction("Exporter image courante en PNG");

    QAction* selected = menu.exec(QCursor::pos());

    if (selected == pdfAction) {
        QString filePath = QFileDialog::getSaveFileName(this, "Exporter en PDF",
            QDir::homePath() + "/" + m_treatiseCode + "_diaporama.pdf",
            "PDF (*.pdf)");
        if (!filePath.isEmpty()) {
            exportToPdf(filePath);
        }
    } else if (selected == pngAllAction) {
        QString folderPath = QFileDialog::getExistingDirectory(this,
            "Choisir le dossier d'export", QDir::homePath());
        if (!folderPath.isEmpty()) {
            exportToPng(folderPath);
        }
    } else if (selected == pngCurrentAction) {
        if (m_currentIndex < 0 || !m_slides[m_currentIndex].imageReady) {
            QMessageBox::warning(this, "Export", "Aucune image courante a exporter.");
            return;
        }
        QString filePath = QFileDialog::getSaveFileName(this, "Exporter en PNG",
            QDir::homePath() + "/" + m_treatiseCode + "_slide_" + QString::number(m_currentIndex + 1) + ".png",
            "PNG (*.png)");
        if (!filePath.isEmpty()) {
            exportCurrentToPng(filePath);
        }
    }
}

void SlideshowDialog::exportToPdf(const QString& filePath) {
    QPdfWriter pdfWriter(filePath);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setPageOrientation(QPageLayout::Landscape);
    pdfWriter.setResolution(150);
    pdfWriter.setTitle(m_treatiseCode + " - Diaporama");
    pdfWriter.setCreator("Codex Nag Hammadi");

    QPainter painter(&pdfWriter);

    int pageWidth = pdfWriter.width();
    int pageHeight = pdfWriter.height();

    bool firstPage = true;
    int exportedCount = 0;

    for (int i = 0; i < m_slides.size(); ++i) {
        if (!m_slides[i].imageReady) continue;

        if (!firstPage) {
            pdfWriter.newPage();
        }
        firstPage = false;

        const SlideItem& slide = m_slides[i];

        // Calculate image area (top 75% of page)
        int imageAreaHeight = pageHeight * 3 / 4;
        QRect imageRect(0, 0, pageWidth, imageAreaHeight);

        // Scale image to fit
        QPixmap scaledImage = slide.image.scaled(imageRect.size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Center image
        int x = (pageWidth - scaledImage.width()) / 2;
        int y = (imageAreaHeight - scaledImage.height()) / 2;
        painter.drawPixmap(x, y, scaledImage);

        // Draw text area (bottom 25%)
        QRect textRect(50, imageAreaHeight + 20, pageWidth - 100, pageHeight - imageAreaHeight - 40);

        // Draw text (searchable in PDF)
        painter.setPen(Qt::black);
        QFont font("Arial", 12);
        painter.setFont(font);
        painter.drawText(textRect, Qt::AlignLeft | Qt::TextWordWrap, slide.text);

        // Draw page number
        painter.setPen(Qt::gray);
        QFont smallFont("Arial", 9);
        painter.setFont(smallFont);
        painter.drawText(pageWidth - 100, pageHeight - 20,
            QString("Page %1/%2").arg(i + 1).arg(m_slides.size()));

        exportedCount++;
    }

    painter.end();

    QMessageBox::information(this, "Export PDF",
        QString("PDF exporte avec succes!\n%1 pages\n\nFichier: %2")
        .arg(exportedCount).arg(filePath));

    LOG_INFO(QString("Exported %1 slides to PDF: %2").arg(exportedCount).arg(filePath));
}

void SlideshowDialog::exportToPng(const QString& folderPath) {
    int exportedCount = 0;

    for (int i = 0; i < m_slides.size(); ++i) {
        if (!m_slides[i].imageReady) continue;

        QPixmap imageWithText = createImageWithText(i);
        if (imageWithText.isNull()) continue;

        QString fileName = QString("%1/%2_slide_%3.png")
            .arg(folderPath)
            .arg(m_treatiseCode)
            .arg(i + 1, 2, 10, QChar('0'));

        if (imageWithText.save(fileName, "PNG")) {
            exportedCount++;
        }
    }

    QMessageBox::information(this, "Export PNG",
        QString("Images exportees avec succes!\n%1 images PNG avec texte incruste\n\nDossier: %2")
        .arg(exportedCount).arg(folderPath));

    LOG_INFO(QString("Exported %1 slides to PNG in: %2").arg(exportedCount).arg(folderPath));
}

void SlideshowDialog::exportCurrentToPng(const QString& filePath) {
    if (m_currentIndex < 0 || !m_slides[m_currentIndex].imageReady) {
        return;
    }

    QPixmap imageWithText = createImageWithText(m_currentIndex);
    if (imageWithText.save(filePath, "PNG")) {
        QMessageBox::information(this, "Export PNG",
            QString("Image exportee avec succes!\n\nFichier: %1").arg(filePath));
        LOG_INFO(QString("Exported slide %1 to PNG: %2").arg(m_currentIndex + 1).arg(filePath));
    } else {
        QMessageBox::warning(this, "Erreur", "Impossible de sauvegarder l'image.");
    }
}

void SlideshowDialog::onLoadMedia() {
    // Show session picker dialog with preview
    SessionPickerDialog picker(this);
    if (picker.exec() != QDialog::Accepted) {
        return;
    }

    // Only handle existing session (ignore "New Generation" in this context)
    if (picker.resultType() == SessionPickerDialog::ExistingSession) {
        QString sessionPath = picker.selectedSessionPath();
        loadMediaSession(sessionPath);
    }
}

void SlideshowDialog::loadMediaSession(const QString& sessionPath) {
    auto& storage = codex::utils::MediaStorage::instance();
    auto info = storage.loadSessionInfo(sessionPath);

    if (info.imageCount == 0) {
        QMessageBox::warning(this, "Erreur",
            QString("Aucune image trouvee dans:\n%1").arg(sessionPath));
        return;
    }

    // Reset state
    m_slides.clear();
    m_thumbnailList->clear();
    m_currentIndex = -1;
    m_isPlaying = false;
    m_isPaused = false;
    m_autoStarted = false;

    // Stop any current playback
    m_audioPlayer->stop();
    m_slideTimer->stop();

    // Update UI text
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

    // Update metadata
    m_treatiseCode = info.treatiseCode;
    m_totalExpectedImages = info.imageCount;

    // Load images
    QStringList images = storage.listImages(sessionPath);
    QStringList audios = storage.listAudios(sessionPath);

    LOG_INFO(QString("Loading session: %1 images, %2 audios from %3")
             .arg(images.size()).arg(audios.size()).arg(sessionPath));

    for (int i = 0; i < images.size(); ++i) {
        SlideItem slide;

        // Load image
        QPixmap pixmap;
        if (pixmap.load(images[i])) {
            slide.image = pixmap;
            slide.imageReady = true;
        }

        // Get text from info
        if (i < info.texts.size()) {
            slide.text = info.texts[i];
        }

        // Check for audio
        QString audioPath = storage.audioPath(sessionPath, i);
        if (!audioPath.isEmpty()) {
            slide.audioPath = audioPath;
            slide.audioReady = true;
            slide.audioDurationMs = 5000; // Default duration
        }

        m_slides.append(slide);

        // Add thumbnail
        auto* item = new QListWidgetItem(QString("Image %1").arg(i + 1));
        if (slide.imageReady) {
            QPixmap thumb = slide.image.scaled(160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            item->setIcon(QIcon(thumb));
        }
        m_thumbnailList->addItem(item);
    }

    // Update UI
    m_generationProgress->setVisible(false);
    m_generationStatus->setText(QString("Charge: %1 images").arg(m_slides.size()));
    m_imagesReceived = m_slides.size();

    // Enable controls
    m_playStopBtn->setEnabled(!m_slides.isEmpty());
    m_prevBtn->setEnabled(!m_slides.isEmpty());
    m_nextBtn->setEnabled(!m_slides.isEmpty());
    m_positionSlider->setEnabled(!m_slides.isEmpty());

    // Show first slide
    if (!m_slides.isEmpty()) {
        showSlide(0);
    }

    m_statusLabel->setText(QString("Session chargee: %1 - %2 images")
                            .arg(QFileInfo(sessionPath).fileName())
                            .arg(m_slides.size()));

    QMessageBox::information(this, "Session chargee",
        QString("Session chargee avec succes!\n\n%1 images\n%2 audios\n\nDossier: %3")
        .arg(images.size())
        .arg(audios.size())
        .arg(sessionPath));
}

void SlideshowDialog::addSlide(const QPixmap& image, const QString& text) {
    SlideItem slide;
    slide.image = image;
    slide.text = text;
    slide.imageReady = true;
    slide.audioReady = false;  // Will be generated on demand

    m_slides.append(slide);

    // Add thumbnail to list
    int index = m_slides.size();
    auto* item = new QListWidgetItem(QString("Image %1").arg(index));
    if (!image.isNull()) {
        QPixmap thumb = image.scaled(160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        item->setIcon(QIcon(thumb));
    }
    m_thumbnailList->addItem(item);

    // Show first slide if this is the first
    if (m_slides.size() == 1) {
        showSlide(0);
    }

    LOG_INFO(QString("Added slide %1 with text: %2 chars")
             .arg(m_slides.size()).arg(text.length()));
}

void SlideshowDialog::startAutoPlay() {
    if (m_slides.isEmpty()) {
        LOG_WARN("Cannot start auto-play: no slides");
        return;
    }

    // Show first slide
    if (m_currentIndex < 0) {
        showSlide(0);
    }

    // Start audio generation for all slides
    if (m_nextAudioToGenerate < m_slides.size()) {
        m_allImagesReceived = true;
        generateNextAudio();
    }

    // Start playing once first audio is ready
    if (m_slides[0].audioReady) {
        m_isPlaying = true;
        m_playStopBtn->setText("Stop");
        m_audioPlayer->play();
    } else {
        // Will auto-start when first audio is generated
        m_autoStarted = false;
    }

    LOG_INFO("Started auto-play with " + QString::number(m_slides.size()) + " slides");
}

void SlideshowDialog::onGenerateVideo() {
    if (m_slides.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Aucune image pour generer une video.");
        return;
    }

    // Count ready images
    QVector<int> readySlides;
    for (int i = 0; i < m_slides.size(); ++i) {
        if (m_slides[i].imageReady) {
            readySlides.append(i);
        }
    }

    if (readySlides.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Aucune image generee. Veuillez d'abord generer des images.");
        return;
    }

    // Create selection dialog
    QDialog selectDialog(this);
    selectDialog.setWindowTitle("Choisir l'image de reference");
    selectDialog.setMinimumSize(600, 400);

    auto* layout = new QVBoxLayout(&selectDialog);

    auto* label = new QLabel("Selectionnez l'image pour la video Veo (8 secondes):", &selectDialog);
    label->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(label);

    // Create list with thumbnails
    auto* imageList = new QListWidget(&selectDialog);
    imageList->setViewMode(QListView::IconMode);
    imageList->setIconSize(QSize(200, 120));
    imageList->setSpacing(10);
    imageList->setSelectionMode(QAbstractItemView::SingleSelection);
    imageList->setStyleSheet(R"(
        QListWidget { background-color: #2d2d2d; }
        QListWidget::item {
            background-color: #3d3d3d;
            border: 2px solid transparent;
            border-radius: 5px;
            padding: 5px;
        }
        QListWidget::item:selected { border-color: #007acc; background-color: #264f78; }
    )");

    for (int idx : readySlides) {
        QPixmap thumb = m_slides[idx].image.scaled(200, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        auto* item = new QListWidgetItem(QIcon(thumb), QString("Image %1").arg(idx + 1));
        item->setData(Qt::UserRole, idx);
        imageList->addItem(item);
    }

    // Pre-select current slide if available
    for (int i = 0; i < imageList->count(); ++i) {
        if (imageList->item(i)->data(Qt::UserRole).toInt() == m_currentIndex) {
            imageList->setCurrentRow(i);
            break;
        }
    }
    if (imageList->currentRow() < 0 && imageList->count() > 0) {
        imageList->setCurrentRow(0);
    }

    layout->addWidget(imageList, 1);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* cancelBtn = new QPushButton("Annuler", &selectDialog);
    auto* generateBtn = new QPushButton("Generer Video IA", &selectDialog);
    generateBtn->setStyleSheet("background-color: #1e5a1e; color: white; font-weight: bold; padding: 8px 20px;");
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(generateBtn);
    layout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, &selectDialog, &QDialog::reject);
    connect(generateBtn, &QPushButton::clicked, &selectDialog, &QDialog::accept);
    connect(imageList, &QListWidget::itemDoubleClicked, &selectDialog, &QDialog::accept);

    if (selectDialog.exec() != QDialog::Accepted) {
        return;
    }

    // Get selected slide
    auto* selectedItem = imageList->currentItem();
    if (!selectedItem) {
        return;
    }

    int slideIndex = selectedItem->data(Qt::UserRole).toInt();
    generateVideoForSlide(slideIndex);
}

void SlideshowDialog::generateVideoForSlide(int slideIndex) {
    if (slideIndex < 0 || slideIndex >= m_slides.size() || !m_slides[slideIndex].imageReady) {
        QMessageBox::warning(this, "Erreur", "Image invalide.");
        return;
    }

    const SlideItem& slide = m_slides[slideIndex];
    m_currentVideoSlide = slideIndex;

    // Get text for prompt
    QString textForPrompt = slide.text;
    if (textForPrompt.isEmpty()) {
        textForPrompt = "Scene mystique gnostique dans un style cinematographique.";
    }

    // Disable button and show progress
    m_videoAIBtn->setEnabled(false);
    m_videoAIBtn->setText("IA...");
    m_statusLabel->setText(QString("Generation video IA en cours (image %1)...").arg(slideIndex + 1));

    // Create video prompt combining text and image context
    QString videoPrompt = QString(
        "Create a mystical, cinematic video in the style of Denis Villeneuve's Dune. "
        "The scene should be ethereal and otherworldly, with dramatic lighting. "
        "Animate the provided image with subtle movements: gentle camera pan, floating particles, "
        "atmospheric fog, and soft light rays. Keep the scene contemplative and spiritual. "
        "Content: %1"
    ).arg(textForPrompt.left(400));  // Limit text length

    // Generate video with image reference (image-to-video)
    codex::api::VideoGenerationParams params;
    params.prompt = videoPrompt;
    params.durationSeconds = 8;
    params.aspectRatio = "16:9";
    params.generateAudio = true;

    // Convert current slide image to bytes for Veo
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    slide.image.save(&buffer, "PNG");
    buffer.close();

    params.referenceImage = imageData;
    params.referenceImageMimeType = "image/png";

    m_veoClient->generateVideo(params);

    LOG_INFO(QString("AI Video generation started from slide %1 with image (%2x%3). Prompt: %4 chars")
             .arg(slideIndex + 1)
             .arg(slide.image.width())
             .arg(slide.image.height())
             .arg(videoPrompt.length()));
}

void SlideshowDialog::onVideoGenerated(const QByteArray& videoData, const QString& prompt) {
    Q_UNUSED(prompt);

    if (videoData.isEmpty()) {
        LOG_WARN("Video data empty");
        if (!m_generatingAllVideos) {
            m_videoAIBtn->setEnabled(true);
            m_videoAIBtn->setText("Video IA");
            QMessageBox::warning(this, "Erreur", "Les donnees video recues sont vides.");
        }
        // Continue with next in queue
        processNextVideoInQueue();
        return;
    }

    // Save video with slide number in filename
    auto& storage = codex::utils::MediaStorage::instance();
    QString timestamp = storage.timestampString();
    QString slideNum = (m_currentVideoSlide >= 0)
        ? QString("_slide%1").arg(m_currentVideoSlide + 1, 2, 10, QChar('0'))
        : "";
    QString fileName = QString("veo_%1%2_%3.mp4")
        .arg(m_treatiseCode.isEmpty() ? "codex" : m_treatiseCode)
        .arg(slideNum)
        .arg(timestamp);

    QString filePath = storage.videosFolder() + "/" + fileName;
    QDir().mkpath(storage.videosFolder());

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(videoData);
        file.close();

        m_videosGenerated++;
        LOG_INFO(QString("Video saved: %1 (%2 bytes)").arg(filePath).arg(videoData.size()));

        if (m_generatingAllVideos) {
            m_statusLabel->setText(QString("Video %1 generee: %2").arg(m_videosGenerated).arg(fileName));
            // Process next video in queue
            processNextVideoInQueue();
        } else {
            m_videoAIBtn->setEnabled(true);
            m_videoAIBtn->setText("Video IA");
            m_statusLabel->setText(QString("Video generee: %1").arg(fileName));

            // Open preview dialog with video
            VideoPreviewDialog previewDialog(this);
            previewDialog.setVideoFile(filePath);

            // Pass TTS audio from the slide if available
            if (m_currentVideoSlide >= 0 && m_currentVideoSlide < m_slides.size()) {
                const auto& slide = m_slides[m_currentVideoSlide];
                if (!slide.audioPath.isEmpty() && QFile::exists(slide.audioPath)) {
                    previewDialog.setTtsAudio(slide.audioPath, slide.audioDurationMs);
                }
            }

            previewDialog.exec();
        }
    } else {
        LOG_ERROR(QString("Failed to save video: %1").arg(filePath));
        if (!m_generatingAllVideos) {
            m_videoAIBtn->setEnabled(true);
            m_videoAIBtn->setText("Video IA");
            QMessageBox::critical(this, "Erreur",
                QString("Impossible de sauvegarder la video:\n%1").arg(filePath));
        }
        processNextVideoInQueue();
    }
}

void SlideshowDialog::onVideoProgress(int percent) {
    // Update progress bar
    m_generationProgress->setVisible(true);
    m_generationProgress->setValue(percent);

    if (m_generatingAllVideos) {
        int totalSlides = m_videoQueue.size() + m_videosGenerated + 1;
        m_videoAIBtn->setText(QString("IA %1/%2").arg(m_videosGenerated + 1).arg(totalSlides));
        m_generationProgress->setFormat(QString("Video %1/%2: %p%")
            .arg(m_videosGenerated + 1).arg(totalSlides));
        m_statusLabel->setText(QString("Generation video %1/%2 en cours...")
            .arg(m_videosGenerated + 1).arg(totalSlides));
    } else {
        m_videoAIBtn->setText(QString("IA %1%").arg(percent));
        m_generationProgress->setFormat("Generation video Veo: %p%");

        // More detailed status based on progress
        QString status;
        if (percent < 10) {
            status = "Envoi de l'image a Veo...";
        } else if (percent < 30) {
            status = "Veo analyse l'image...";
        } else if (percent < 70) {
            status = "Generation de la video en cours...";
        } else if (percent < 95) {
            status = "Finalisation de la video...";
        } else {
            status = "Telechargement de la video...";
        }
        m_statusLabel->setText(QString("%1 (%2%)").arg(status).arg(percent));
    }

    // Hide progress bar when done
    if (percent >= 100) {
        m_generationProgress->setVisible(false);
    }
}

void SlideshowDialog::onVideoError(const QString& error) {
    LOG_ERROR(QString("AI Video generation failed: %1").arg(error));

    if (m_generatingAllVideos) {
        m_statusLabel->setText(QString("Erreur slide %1: %2").arg(m_currentVideoSlide + 1).arg(error));
        // Continue with next in queue
        processNextVideoInQueue();
    } else {
        m_videoAIBtn->setEnabled(true);
        m_videoAIBtn->setText("Video IA");
        m_statusLabel->setText(QString("Erreur video IA: %1").arg(error));
        showCopyableError(this, "Erreur de generation video IA", error);
    }
}

void SlideshowDialog::onGenerateAllVideos() {
    // Count ready images
    m_videoQueue.clear();
    for (int i = 0; i < m_slides.size(); ++i) {
        if (m_slides[i].imageReady) {
            m_videoQueue.append(i);
        }
    }

    if (m_videoQueue.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Aucune image disponible.");
        return;
    }

    // Confirm with user (expensive operation)
    int count = m_videoQueue.size();
    double cost = count * 3.20;  // ~$3.20 per 8s video
    auto reply = QMessageBox::question(this, "Generer toutes les videos",
        QString("Generer %1 videos Veo de 8 secondes?\n\n"
                "Cout estime: ~$%2\n"
                "Temps estime: %3-%4 minutes\n\n"
                "Continuer?")
        .arg(count)
        .arg(cost, 0, 'f', 2)
        .arg(count * 2)
        .arg(count * 5),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        m_videoQueue.clear();
        return;
    }

    m_generatingAllVideos = true;
    m_videosGenerated = 0;
    m_videoAIBtn->setEnabled(false);

    processNextVideoInQueue();
}

void SlideshowDialog::processNextVideoInQueue() {
    if (m_videoQueue.isEmpty()) {
        // All done
        m_generatingAllVideos = false;
        m_videoAIBtn->setEnabled(true);
        m_videoAIBtn->setText("Video IA");

        if (m_videosGenerated > 0) {
            m_statusLabel->setText(QString("%1 videos generees!").arg(m_videosGenerated));
            QMessageBox::information(this, "Generation terminee",
                QString("%1 videos Veo ont ete generees avec succes!").arg(m_videosGenerated));
        }
        return;
    }

    int nextSlide = m_videoQueue.takeFirst();
    generateVideoForSlide(nextSlide);
}

void SlideshowDialog::onExportVideo() {
    if (m_slides.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Aucune image a exporter.");
        return;
    }

    // Check for images
    int imageCount = 0;
    for (const auto& slide : m_slides) {
        if (slide.imageReady) imageCount++;
    }

    if (imageCount == 0) {
        QMessageBox::warning(this, "Erreur", "Aucune image generee.");
        return;
    }

    // Find FFmpeg executable
    QString ffmpegPath = "ffmpeg";  // Default: try PATH

    // Check common installation paths if not in PATH
    QStringList searchPaths = {
        "ffmpeg",
        QDir::homePath() + "/AppData/Local/Microsoft/WinGet/Links/ffmpeg.exe",
        "C:/ffmpeg/bin/ffmpeg.exe",
        "C:/Program Files/ffmpeg/bin/ffmpeg.exe",
        "C:/Program Files (x86)/ffmpeg/bin/ffmpeg.exe"
    };

    bool ffmpegFound = false;
    for (const QString& path : searchPaths) {
        QProcess testProcess;
        testProcess.start(path, QStringList() << "-version");
        testProcess.waitForFinished(3000);

        if (testProcess.exitCode() == 0 && testProcess.error() != QProcess::FailedToStart) {
            ffmpegPath = path;
            ffmpegFound = true;
            LOG_INFO(QString("FFmpeg found at: %1").arg(ffmpegPath));
            break;
        }
    }

    if (!ffmpegFound) {
        QMessageBox::warning(this, "FFmpeg requis",
            "FFmpeg n'est pas installe ou introuvable.\n\n"
            "Pour installer FFmpeg sur Windows:\n"
            "1. Telecharger depuis https://ffmpeg.org/download.html\n"
            "2. Extraire et ajouter le dossier 'bin' au PATH\n"
            "3. Ou utiliser: winget install ffmpeg\n\n"
            "Redemarrez l'application apres l'installation.");
        return;
    }

    // Ask for output file
    auto& storage = codex::utils::MediaStorage::instance();
    QString timestamp = storage.timestampString();
    QString defaultName = QString("%1_%2.mp4")
        .arg(m_treatiseCode.isEmpty() ? "diaporama" : m_treatiseCode)
        .arg(timestamp);

    QString filePath = QFileDialog::getSaveFileName(
        this, "Exporter video",
        storage.videosFolder() + "/" + defaultName,
        "Video MP4 (*.mp4)");

    if (filePath.isEmpty()) return;

    // Create temp directory for frames
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        QMessageBox::critical(this, "Erreur", "Impossible de creer le dossier temporaire.");
        return;
    }

    m_videoBtn->setEnabled(false);
    m_videoBtn->setText("Export...");
    m_statusLabel->setText("Export video en cours...");
    QApplication::processEvents();

    // Export frames with text overlay
    QStringList frameFiles;
    QStringList audioFiles;
    double totalDuration = 0;

    for (int i = 0; i < m_slides.size(); ++i) {
        if (!m_slides[i].imageReady) continue;

        // Create image with text overlay
        QPixmap frameImage = createImageWithText(i);
        QString framePath = QString("%1/frame_%2.png")
            .arg(tempDir.path())
            .arg(i, 4, 10, QChar('0'));

        frameImage.save(framePath, "PNG");
        frameFiles.append(framePath);

        // Get audio duration or use default
        double duration = 5.0;  // Default 5 seconds per slide
        if (m_slides[i].audioReady && m_slides[i].audioDurationMs > 0) {
            duration = m_slides[i].audioDurationMs / 1000.0;
        }
        totalDuration += duration;

        // Add audio file if available
        if (m_slides[i].audioReady && !m_slides[i].audioPath.isEmpty()) {
            audioFiles.append(m_slides[i].audioPath);
        }
    }

    if (frameFiles.isEmpty()) {
        m_videoBtn->setEnabled(true);
        m_videoBtn->setText("Exporter Video");
        QMessageBox::warning(this, "Erreur", "Aucune image a exporter.");
        return;
    }

    // Create FFmpeg concat file for images
    QString concatPath = tempDir.path() + "/concat.txt";
    QFile concatFile(concatPath);
    if (concatFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&concatFile);
        for (int i = 0; i < frameFiles.size(); ++i) {
            double duration = 5.0;
            if (i < m_slides.size() && m_slides[i].audioDurationMs > 0) {
                duration = m_slides[i].audioDurationMs / 1000.0;
            }
            stream << "file '" << frameFiles[i].replace("\\", "/") << "'\n";
            stream << "duration " << duration << "\n";
        }
        // Add last frame again (FFmpeg concat requirement)
        stream << "file '" << frameFiles.last().replace("\\", "/") << "'\n";
        concatFile.close();
    }

    m_statusLabel->setText("Encodage video...");
    QApplication::processEvents();

    // Build FFmpeg command
    QStringList ffmpegArgs;
    ffmpegArgs << "-y";  // Overwrite
    ffmpegArgs << "-f" << "concat";
    ffmpegArgs << "-safe" << "0";
    ffmpegArgs << "-i" << concatPath;

    // If we have audio files, merge them
    if (!audioFiles.isEmpty()) {
        // Create audio concat file
        QString audioConcatPath = tempDir.path() + "/audio_concat.txt";
        QFile audioConcatFile(audioConcatPath);
        if (audioConcatFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&audioConcatFile);
            for (const QString& audioPath : audioFiles) {
                QString path = audioPath;
                path.replace("\\", "/");
                stream << "file '" << path << "'\n";
            }
            audioConcatFile.close();
        }

        // First merge audio files
        QString mergedAudioPath = tempDir.path() + "/merged_audio.mp3";
        QProcess audioMerge;
        QStringList audioMergeArgs;
        audioMergeArgs << "-y" << "-f" << "concat" << "-safe" << "0";
        audioMergeArgs << "-i" << audioConcatPath;
        audioMergeArgs << "-c" << "copy" << mergedAudioPath;

        audioMerge.start(ffmpegPath, audioMergeArgs);
        audioMerge.waitForFinished(60000);

        if (QFile::exists(mergedAudioPath)) {
            ffmpegArgs << "-i" << mergedAudioPath;
            ffmpegArgs << "-c:a" << "aac" << "-b:a" << "192k";
        }
    }

    ffmpegArgs << "-c:v" << "libx264";
    ffmpegArgs << "-pix_fmt" << "yuv420p";
    ffmpegArgs << "-preset" << "medium";
    ffmpegArgs << "-crf" << "23";
    ffmpegArgs << "-shortest";
    ffmpegArgs << filePath;

    // Run FFmpeg
    QProcess ffmpeg;
    ffmpeg.start(ffmpegPath, ffmpegArgs);

    // Wait with timeout (max 5 minutes)
    if (!ffmpeg.waitForFinished(300000)) {
        ffmpeg.kill();
        m_videoBtn->setEnabled(true);
        m_videoBtn->setText("Exporter Video");
        QMessageBox::critical(this, "Erreur", "L'export video a depasse le delai maximum.");
        return;
    }

    m_videoBtn->setEnabled(true);
    m_videoBtn->setText("Exporter Video");

    if (ffmpeg.exitCode() == 0 && QFile::exists(filePath)) {
        QFileInfo fi(filePath);
        m_statusLabel->setText(QString("Video exportee: %1").arg(fi.fileName()));
        QMessageBox::information(this, "Export reussi",
            QString("Video exportee avec succes!\n\n"
                    "Fichier: %1\n"
                    "Taille: %2 MB\n"
                    "Duree: ~%3 secondes\n"
                    "Images: %4")
            .arg(filePath)
            .arg(fi.size() / (1024.0 * 1024.0), 0, 'f', 2)
            .arg(totalDuration, 0, 'f', 1)
            .arg(frameFiles.size()));

        LOG_INFO(QString("Video exported: %1 (%2 frames, %3 sec)")
                 .arg(filePath).arg(frameFiles.size()).arg(totalDuration));
    } else {
        QString errorOutput = ffmpeg.readAllStandardError();
        m_statusLabel->setText("Erreur export video");
        QMessageBox::critical(this, "Erreur FFmpeg",
            QString("L'export video a echoue.\n\nErreur: %1").arg(errorOutput.left(500)));
        LOG_ERROR(QString("FFmpeg failed: %1").arg(errorOutput));
    }
}

} // namespace codex::ui
