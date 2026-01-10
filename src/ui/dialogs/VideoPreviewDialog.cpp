#include "VideoPreviewDialog.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include <QStyle>

namespace codex::ui {

VideoPreviewDialog::VideoPreviewDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Apercu Video Veo");
    setMinimumSize(800, 600);
    resize(900, 700);

    setupUi();

    // Setup media player
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_mediaPlayer->setVideoOutput(m_videoWidget);
    m_audioOutput->setVolume(0.8);

    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &VideoPreviewDialog::onPositionChanged);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &VideoPreviewDialog::onDurationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &VideoPreviewDialog::onMediaStatusChanged);
}

VideoPreviewDialog::~VideoPreviewDialog() {
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }
}

void VideoPreviewDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Video widget
    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setMinimumSize(640, 360);
    m_videoWidget->setStyleSheet("background-color: black;");
    mainLayout->addWidget(m_videoWidget, 1);

    // Position slider
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_positionSlider->setRange(0, 0);
    connect(m_positionSlider, &QSlider::sliderMoved, m_mediaPlayer, &QMediaPlayer::setPosition);
    mainLayout->addWidget(m_positionSlider);

    // Time label
    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setStyleSheet("font-family: monospace; font-size: 12px;");
    mainLayout->addWidget(m_timeLabel);

    // Playback controls
    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(10);

    m_playPauseBtn = new QPushButton("Lecture", this);
    m_playPauseBtn->setFixedHeight(36);
    m_playPauseBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #1e5a1e;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 20px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #2d7a2d; }
    )");
    connect(m_playPauseBtn, &QPushButton::clicked, this, &VideoPreviewDialog::onPlayPause);
    controlsLayout->addWidget(m_playPauseBtn);

    m_stopBtn = new QPushButton("Stop", this);
    m_stopBtn->setFixedHeight(36);
    connect(m_stopBtn, &QPushButton::clicked, this, &VideoPreviewDialog::onStop);
    controlsLayout->addWidget(m_stopBtn);

    controlsLayout->addStretch();

    m_mergeAudioBtn = new QPushButton("Fusionner Audio TTS", this);
    m_mergeAudioBtn->setFixedHeight(36);
    m_mergeAudioBtn->setEnabled(false);
    m_mergeAudioBtn->setToolTip("Ajouter l'audio TTS comme piste supplementaire");
    m_mergeAudioBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #5a3a1e;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 15px;
        }
        QPushButton:hover { background-color: #7a5a2e; }
        QPushButton:disabled { background-color: #3a3a3a; color: #888; }
    )");
    connect(m_mergeAudioBtn, &QPushButton::clicked, this, &VideoPreviewDialog::onMergeAudio);
    controlsLayout->addWidget(m_mergeAudioBtn);

    m_openExternalBtn = new QPushButton("Ouvrir dans VLC", this);
    m_openExternalBtn->setFixedHeight(36);
    m_openExternalBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #1e3a5a;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 15px;
        }
        QPushButton:hover { background-color: #2d5a7a; }
    )");
    connect(m_openExternalBtn, &QPushButton::clicked, this, &VideoPreviewDialog::onOpenExternal);
    controlsLayout->addWidget(m_openExternalBtn);

    m_saveAsBtn = new QPushButton("Enregistrer sous...", this);
    m_saveAsBtn->setFixedHeight(36);
    connect(m_saveAsBtn, &QPushButton::clicked, this, &VideoPreviewDialog::onSaveAs);
    controlsLayout->addWidget(m_saveAsBtn);

    mainLayout->addLayout(controlsLayout);

    // Merge progress bar (hidden by default)
    m_mergeProgress = new QProgressBar(this);
    m_mergeProgress->setVisible(false);
    m_mergeProgress->setTextVisible(true);
    m_mergeProgress->setFormat("Fusion audio: %p%");
    mainLayout->addWidget(m_mergeProgress);

    // Status label
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("color: #888; font-size: 11px;");
    mainLayout->addWidget(m_statusLabel);

    // Close button
    auto* closeLayout = new QHBoxLayout();
    closeLayout->addStretch();
    auto* closeBtn = new QPushButton("Fermer", this);
    closeBtn->setFixedHeight(36);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeLayout->addWidget(closeBtn);
    mainLayout->addLayout(closeLayout);
}

void VideoPreviewDialog::setVideoFile(const QString& filePath) {
    m_videoFilePath = filePath;
    m_outputFilePath = filePath;

    QFileInfo fi(filePath);
    m_statusLabel->setText(QString("Video: %1 (%2 KB)")
        .arg(fi.fileName())
        .arg(fi.size() / 1024));

    m_mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
}

void VideoPreviewDialog::setTtsAudio(const QString& ttsPath, int durationMs) {
    m_ttsAudioPath = ttsPath;
    m_ttsDurationMs = durationMs;

    if (!ttsPath.isEmpty() && QFile::exists(ttsPath)) {
        m_mergeAudioBtn->setEnabled(true);
        m_mergeAudioBtn->setToolTip(QString("Fusionner avec: %1").arg(QFileInfo(ttsPath).fileName()));
    }
}

void VideoPreviewDialog::onPlayPause() {
    if (m_isPlaying) {
        m_mediaPlayer->pause();
        m_playPauseBtn->setText("Lecture");
        m_isPlaying = false;
    } else {
        m_mediaPlayer->play();
        m_playPauseBtn->setText("Pause");
        m_isPlaying = true;
    }
}

void VideoPreviewDialog::onStop() {
    m_mediaPlayer->stop();
    m_playPauseBtn->setText("Lecture");
    m_isPlaying = false;
}

void VideoPreviewDialog::onOpenExternal() {
    QString fileToOpen = m_outputFilePath.isEmpty() ? m_videoFilePath : m_outputFilePath;
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileToOpen));
}

void VideoPreviewDialog::onMergeAudio() {
    if (m_ttsAudioPath.isEmpty() || !QFile::exists(m_ttsAudioPath)) {
        QMessageBox::warning(this, "Erreur", "Aucun fichier audio TTS disponible.");
        return;
    }

    QString ffmpegPath = findFFmpeg();
    if (ffmpegPath.isEmpty()) {
        QMessageBox::warning(this, "FFmpeg non trouve",
            "FFmpeg est requis pour fusionner les pistes audio.\n\n"
            "Installez FFmpeg et assurez-vous qu'il est dans le PATH.");
        return;
    }

    // Create output filename
    QFileInfo fi(m_videoFilePath);
    QString outputPath = fi.absolutePath() + "/" + fi.baseName() + "_merged.mp4";

    m_mergeProgress->setVisible(true);
    m_mergeProgress->setValue(10);
    m_mergeAudioBtn->setEnabled(false);
    m_statusLabel->setText("Fusion des pistes audio en cours...");

    QApplication::processEvents();

    // FFmpeg command to add TTS as second audio track
    // -map 0:v = video from first input
    // -map 0:a? = audio from first input (if exists)
    // -map 1:a = audio from second input (TTS)
    QStringList args;
    args << "-y"  // Overwrite
         << "-i" << m_videoFilePath
         << "-i" << m_ttsAudioPath
         << "-map" << "0:v"      // Video from Veo
         << "-map" << "0:a?"     // Audio from Veo (optional)
         << "-map" << "1:a"      // Audio from TTS
         << "-c:v" << "copy"     // Copy video without re-encoding
         << "-c:a" << "aac"      // Encode audio to AAC
         << "-shortest"          // Stop when shortest input ends
         << outputPath;

    m_mergeProgress->setValue(30);
    QApplication::processEvents();

    QProcess ffmpeg;
    ffmpeg.start(ffmpegPath, args);

    if (!ffmpeg.waitForFinished(60000)) {  // 60 sec timeout
        m_mergeProgress->setVisible(false);
        m_mergeAudioBtn->setEnabled(true);
        m_statusLabel->setText("Erreur: FFmpeg timeout");
        QMessageBox::critical(this, "Erreur", "FFmpeg a pris trop de temps.");
        return;
    }

    m_mergeProgress->setValue(90);
    QApplication::processEvents();

    if (ffmpeg.exitCode() != 0) {
        QString error = QString::fromUtf8(ffmpeg.readAllStandardError());
        m_mergeProgress->setVisible(false);
        m_mergeAudioBtn->setEnabled(true);
        m_statusLabel->setText("Erreur FFmpeg");
        QMessageBox::critical(this, "Erreur FFmpeg",
            QString("La fusion a echoue:\n%1").arg(error.left(500)));
        LOG_ERROR(QString("FFmpeg merge failed: %1").arg(error));
        return;
    }

    m_mergeProgress->setValue(100);

    // Success - load the merged file
    m_outputFilePath = outputPath;
    QFileInfo outFi(outputPath);

    m_mediaPlayer->stop();
    m_mediaPlayer->setSource(QUrl::fromLocalFile(outputPath));

    m_mergeProgress->setVisible(false);
    m_mergeAudioBtn->setText("Audio fusionne!");
    m_mergeAudioBtn->setEnabled(false);
    m_statusLabel->setText(QString("Video fusionnee: %1 (%2 KB) - 2 pistes audio")
        .arg(outFi.fileName())
        .arg(outFi.size() / 1024));

    LOG_INFO(QString("Audio merged successfully: %1").arg(outputPath));

    QMessageBox::information(this, "Fusion reussie",
        QString("La video avec les 2 pistes audio a ete creee:\n%1\n\n"
                "Piste 1: Audio Veo (ambiance IA)\n"
                "Piste 2: Audio TTS (narration)")
        .arg(outputPath));
}

void VideoPreviewDialog::onSaveAs() {
    QString sourceFile = m_outputFilePath.isEmpty() ? m_videoFilePath : m_outputFilePath;

    QString savePath = QFileDialog::getSaveFileName(this,
        "Enregistrer la video",
        QDir::homePath() + "/Videos/veo_video.mp4",
        "Video MP4 (*.mp4)");

    if (savePath.isEmpty()) return;

    if (QFile::copy(sourceFile, savePath)) {
        m_statusLabel->setText(QString("Video sauvegardee: %1").arg(savePath));
        QMessageBox::information(this, "Sauvegarde", "Video sauvegardee avec succes!");
    } else {
        QMessageBox::warning(this, "Erreur", "Impossible de sauvegarder la video.");
    }
}

void VideoPreviewDialog::onPositionChanged(qint64 position) {
    m_positionSlider->setValue(static_cast<int>(position));

    qint64 duration = m_mediaPlayer->duration();
    m_timeLabel->setText(QString("%1 / %2")
        .arg(formatTime(position))
        .arg(formatTime(duration)));
}

void VideoPreviewDialog::onDurationChanged(qint64 duration) {
    m_positionSlider->setRange(0, static_cast<int>(duration));
}

void VideoPreviewDialog::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
        m_playPauseBtn->setText("Lecture");
        m_isPlaying = false;
        m_mediaPlayer->setPosition(0);
    } else if (status == QMediaPlayer::LoadedMedia) {
        // Auto-play when loaded
        m_mediaPlayer->play();
        m_playPauseBtn->setText("Pause");
        m_isPlaying = true;
    }
}

QString VideoPreviewDialog::formatTime(qint64 ms) const {
    int secs = static_cast<int>(ms / 1000);
    int mins = secs / 60;
    secs = secs % 60;
    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

QString VideoPreviewDialog::findFFmpeg() const {
    QStringList searchPaths = {
        "ffmpeg",
        QDir::homePath() + "/AppData/Local/Microsoft/WinGet/Links/ffmpeg.exe",
        "C:/ffmpeg/bin/ffmpeg.exe",
        "C:/Program Files/ffmpeg/bin/ffmpeg.exe",
        "C:/Program Files (x86)/ffmpeg/bin/ffmpeg.exe",
        "/usr/bin/ffmpeg",
        "/usr/local/bin/ffmpeg"
    };

    for (const QString& path : searchPaths) {
        QProcess test;
        test.start(path, QStringList() << "-version");
        if (test.waitForFinished(3000) && test.exitCode() == 0) {
            return path;
        }
    }

    return QString();
}

} // namespace codex::ui
