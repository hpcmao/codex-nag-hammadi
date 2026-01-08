#include "AudioPlayerWidget.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTemporaryFile>
#include <QDir>
#include <QStandardPaths>

namespace codex::ui {

AudioPlayerWidget::AudioPlayerWidget(QWidget* parent)
    : QWidget(parent)
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    setupUi();

    // Connect player signals
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &AudioPlayerWidget::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &AudioPlayerWidget::onDurationChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, &AudioPlayerWidget::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, &AudioPlayerWidget::onPlaybackStateChanged);
}

AudioPlayerWidget::~AudioPlayerWidget() {
    // Clean up temp file if exists
    if (!m_tempFilePath.isEmpty() && QFile::exists(m_tempFilePath)) {
        QFile::remove(m_tempFilePath);
    }
}

void AudioPlayerWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Title
    auto* titleLabel = new QLabel("Lecteur Audio", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #d4d4d4;");
    mainLayout->addWidget(titleLabel);

    // Controls row
    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(5);

    m_playPauseBtn = new QPushButton("Play", this);
    m_playPauseBtn->setFixedSize(60, 30);
    m_playPauseBtn->setEnabled(false);
    m_playPauseBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #094771;
            color: white;
            border: none;
            border-radius: 3px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #0a5a8c; }
        QPushButton:disabled {
            background-color: #3d3d3d;
            color: #666;
        }
    )");
    connect(m_playPauseBtn, &QPushButton::clicked, this, &AudioPlayerWidget::onPlayPauseClicked);
    controlsLayout->addWidget(m_playPauseBtn);

    m_stopBtn = new QPushButton("Stop", this);
    m_stopBtn->setFixedSize(50, 30);
    m_stopBtn->setEnabled(false);
    m_stopBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3d3d3d;
            color: #d4d4d4;
            border: none;
            border-radius: 3px;
        }
        QPushButton:hover { background-color: #4d4d4d; }
        QPushButton:disabled {
            background-color: #2d2d2d;
            color: #555;
        }
    )");
    connect(m_stopBtn, &QPushButton::clicked, this, &AudioPlayerWidget::onStopClicked);
    controlsLayout->addWidget(m_stopBtn);

    // Time label
    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setStyleSheet("color: #888; font-size: 11px; min-width: 80px;");
    controlsLayout->addWidget(m_timeLabel);

    controlsLayout->addStretch();

    // Volume icon and slider
    auto* volumeLabel = new QLabel("Vol", this);
    volumeLabel->setStyleSheet("color: #888; font-size: 10px;");
    controlsLayout->addWidget(volumeLabel);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(70);
    m_volumeSlider->setFixedWidth(80);
    m_volumeSlider->setStyleSheet(R"(
        QSlider::groove:horizontal {
            height: 4px;
            background: #3d3d3d;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            width: 12px;
            margin: -4px 0;
            background: #007acc;
            border-radius: 6px;
        }
    )");
    connect(m_volumeSlider, &QSlider::valueChanged, this, &AudioPlayerWidget::onVolumeSliderChanged);
    controlsLayout->addWidget(m_volumeSlider);

    mainLayout->addLayout(controlsLayout);

    // Position slider
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_positionSlider->setRange(0, 0);
    m_positionSlider->setEnabled(false);
    m_positionSlider->setStyleSheet(R"(
        QSlider::groove:horizontal {
            height: 6px;
            background: #3d3d3d;
            border-radius: 3px;
        }
        QSlider::sub-page:horizontal {
            background: #007acc;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            width: 14px;
            margin: -4px 0;
            background: #d4d4d4;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:disabled {
            background: #555;
        }
    )");
    connect(m_positionSlider, &QSlider::sliderPressed, this, &AudioPlayerWidget::onPositionSliderPressed);
    connect(m_positionSlider, &QSlider::sliderReleased, this, &AudioPlayerWidget::onPositionSliderReleased);
    mainLayout->addWidget(m_positionSlider);

    // Status label
    m_statusLabel = new QLabel("Aucun audio charge", this);
    m_statusLabel->setStyleSheet("color: #666; font-size: 10px;");
    mainLayout->addWidget(m_statusLabel);

    // Initialize volume
    m_audioOutput->setVolume(0.7f);
}

void AudioPlayerWidget::loadFromFile(const QString& filePath) {
    if (!QFile::exists(filePath)) {
        LOG_ERROR(QString("Audio file not found: %1").arg(filePath));
        m_statusLabel->setText("Fichier audio introuvable");
        return;
    }

    m_player->setSource(QUrl::fromLocalFile(filePath));
    m_hasAudio = true;
    m_playPauseBtn->setEnabled(true);
    m_stopBtn->setEnabled(true);
    m_positionSlider->setEnabled(true);
    m_statusLabel->setText(QString("Charge: %1").arg(QFileInfo(filePath).fileName()));

    LOG_INFO(QString("Loaded audio file: %1").arg(filePath));
}

void AudioPlayerWidget::loadFromData(const QByteArray& audioData) {
    // Save to temp file (QMediaPlayer needs a file)
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_tempFilePath = tempDir + "/codex_audio_temp.mp3";

    QFile tempFile(m_tempFilePath);
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(audioData);
        tempFile.close();

        loadFromFile(m_tempFilePath);
        m_statusLabel->setText("Audio genere charge");
    } else {
        LOG_ERROR("Failed to write temp audio file");
        m_statusLabel->setText("Erreur: impossible de charger l'audio");
    }
}

void AudioPlayerWidget::play() {
    if (m_hasAudio) {
        m_player->play();
    }
}

void AudioPlayerWidget::pause() {
    m_player->pause();
}

void AudioPlayerWidget::stop() {
    m_player->stop();
    m_player->setPosition(0);
}

bool AudioPlayerWidget::isPlaying() const {
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

int AudioPlayerWidget::volume() const {
    return static_cast<int>(m_audioOutput->volume() * 100);
}

void AudioPlayerWidget::setVolume(int volume) {
    m_audioOutput->setVolume(volume / 100.0f);
    m_volumeSlider->setValue(volume);
}

void AudioPlayerWidget::onPlayPauseClicked() {
    if (isPlaying()) {
        pause();
    } else {
        play();
    }
}

void AudioPlayerWidget::onStopClicked() {
    stop();
}

void AudioPlayerWidget::onPositionChanged(qint64 position) {
    if (!m_sliderPressed) {
        m_positionSlider->setValue(static_cast<int>(position));
    }

    qint64 duration = m_player->duration();
    m_timeLabel->setText(QString("%1 / %2")
                         .arg(formatTime(position))
                         .arg(formatTime(duration)));

    emit positionChanged(position);
}

void AudioPlayerWidget::onDurationChanged(qint64 duration) {
    m_positionSlider->setRange(0, static_cast<int>(duration));
    emit durationChanged(duration);
}

void AudioPlayerWidget::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    switch (status) {
        case QMediaPlayer::EndOfMedia:
            stop();
            emit playbackFinished();
            m_statusLabel->setText("Lecture terminee");
            break;
        case QMediaPlayer::InvalidMedia:
            m_statusLabel->setText("Format audio invalide");
            break;
        case QMediaPlayer::LoadedMedia:
            m_statusLabel->setText("Pret a jouer");
            break;
        default:
            break;
    }
}

void AudioPlayerWidget::onPlaybackStateChanged(QMediaPlayer::PlaybackState state) {
    updatePlayPauseButton();

    switch (state) {
        case QMediaPlayer::PlayingState:
            m_statusLabel->setText("Lecture en cours...");
            emit playbackStarted();
            break;
        case QMediaPlayer::PausedState:
            m_statusLabel->setText("En pause");
            emit playbackPaused();
            break;
        case QMediaPlayer::StoppedState:
            m_statusLabel->setText("Arrete");
            emit playbackStopped();
            break;
    }
}

void AudioPlayerWidget::onVolumeSliderChanged(int value) {
    m_audioOutput->setVolume(value / 100.0f);
}

void AudioPlayerWidget::onPositionSliderPressed() {
    m_sliderPressed = true;
}

void AudioPlayerWidget::onPositionSliderReleased() {
    m_sliderPressed = false;
    m_player->setPosition(m_positionSlider->value());
}

void AudioPlayerWidget::updatePlayPauseButton() {
    if (isPlaying()) {
        m_playPauseBtn->setText("Pause");
    } else {
        m_playPauseBtn->setText("Play");
    }
}

QString AudioPlayerWidget::formatTime(qint64 ms) const {
    int seconds = static_cast<int>(ms / 1000);
    int minutes = seconds / 60;
    seconds = seconds % 60;
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

} // namespace codex::ui
