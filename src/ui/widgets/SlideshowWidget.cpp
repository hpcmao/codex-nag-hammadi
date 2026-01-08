#include "SlideshowWidget.h"
#include "utils/Logger.h"

#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QScreen>

namespace codex::ui {

SlideshowWidget::SlideshowWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Codex Nag Hammadi - Diaporama");
    setMinimumSize(800, 600);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // Black background
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);

    // Slide timer
    m_slideTimer = new QTimer(this);
    m_slideTimer->setSingleShot(true);
    connect(m_slideTimer, &QTimer::timeout, this, &SlideshowWidget::onSlideTimerTimeout);

    // Controls auto-hide timer
    m_controlsTimer = new QTimer(this);
    m_controlsTimer->setSingleShot(true);
    m_controlsTimer->setInterval(3000);
    connect(m_controlsTimer, &QTimer::timeout, this, &SlideshowWidget::onControlsTimeout);

    // Fade animation
    m_fadeAnimation = new QPropertyAnimation(this, "fadeOpacity", this);
    m_fadeAnimation->setDuration(m_transitionDurationMs);
    m_fadeAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, &SlideshowWidget::onFadeAnimationFinished);

    // Audio player
    m_audioPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.8f);

    connect(m_audioPlayer, &QMediaPlayer::positionChanged,
            this, &SlideshowWidget::onAudioPositionChanged);
    connect(m_audioPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &SlideshowWidget::onAudioMediaStatusChanged);
}

SlideshowWidget::~SlideshowWidget() {
    stop();
}

void SlideshowWidget::setSlides(const QVector<SlideData>& slides) {
    stop();
    m_slides = slides;
    m_currentIndex = 0;

    if (!m_slides.isEmpty()) {
        m_currentImage = m_slides[0].image;
        updateScaledImage();
    }

    update();
}

void SlideshowWidget::addSlide(const SlideData& slide) {
    m_slides.append(slide);
}

void SlideshowWidget::clear() {
    stop();
    m_slides.clear();
    m_currentIndex = 0;
    m_currentImage = QPixmap();
    m_scaledCurrentImage = QPixmap();
    update();
}

void SlideshowWidget::start() {
    if (m_slides.isEmpty()) {
        LOG_WARN("Cannot start slideshow: no slides");
        return;
    }

    m_isPlaying = true;
    m_isPaused = false;
    m_currentIndex = 0;

    showSlide(0);
    emit playbackStarted();

    LOG_INFO(QString("Slideshow started with %1 slides").arg(m_slides.size()));
}

void SlideshowWidget::pause() {
    if (!m_isPlaying || m_isPaused) return;

    m_isPaused = true;
    m_slideTimer->stop();
    m_audioPlayer->pause();

    emit playbackPaused();
    LOG_INFO("Slideshow paused");
}

void SlideshowWidget::resume() {
    if (!m_isPlaying || !m_isPaused) return;

    m_isPaused = false;
    m_audioPlayer->play();

    // Resume timer with remaining time (approximation)
    if (!m_inTransition) {
        startSlideTimer();
    }

    LOG_INFO("Slideshow resumed");
}

void SlideshowWidget::stop() {
    m_isPlaying = false;
    m_isPaused = false;
    m_inTransition = false;

    m_slideTimer->stop();
    m_fadeAnimation->stop();
    m_audioPlayer->stop();

    emit playbackStopped();
    LOG_INFO("Slideshow stopped");
}

void SlideshowWidget::nextSlide() {
    if (m_slides.isEmpty()) return;

    if (m_currentIndex < m_slides.size() - 1) {
        m_nextIndex = m_currentIndex + 1;
        startFadeTransition();
    } else if (m_autoPlay) {
        // End of slideshow
        stop();
        emit playbackFinished();
    }
}

void SlideshowWidget::previousSlide() {
    if (m_slides.isEmpty()) return;

    if (m_currentIndex > 0) {
        m_nextIndex = m_currentIndex - 1;
        startFadeTransition();
    }
}

void SlideshowWidget::goToSlide(int index) {
    if (index < 0 || index >= m_slides.size()) return;

    m_nextIndex = index;
    startFadeTransition();
}

void SlideshowWidget::setVolume(int volume) {
    m_audioOutput->setVolume(volume / 100.0f);
}

int SlideshowWidget::volume() const {
    return static_cast<int>(m_audioOutput->volume() * 100);
}

void SlideshowWidget::setFadeOpacity(qreal opacity) {
    m_fadeOpacity = opacity;
    update();
}

void SlideshowWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Fill background with black
    painter.fillRect(rect(), Qt::black);

    if (m_scaledCurrentImage.isNull() && m_scaledNextImage.isNull()) {
        // No image - draw placeholder text
        painter.setPen(QColor(100, 100, 100));
        painter.setFont(QFont("Arial", 24));
        painter.drawText(rect(), Qt::AlignCenter, "Aucune image");
        return;
    }

    // Calculate centered position for images
    QRect imageRect;
    if (!m_scaledCurrentImage.isNull()) {
        imageRect = QRect(
            (width() - m_scaledCurrentImage.width()) / 2,
            (height() - m_scaledCurrentImage.height()) / 2,
            m_scaledCurrentImage.width(),
            m_scaledCurrentImage.height()
        );
    }

    if (m_inTransition && !m_scaledNextImage.isNull()) {
        // Crossfade transition
        // Draw current image fading out
        painter.setOpacity(m_fadeOpacity);
        if (!m_scaledCurrentImage.isNull()) {
            painter.drawPixmap(imageRect, m_scaledCurrentImage);
        }

        // Draw next image fading in
        QRect nextRect(
            (width() - m_scaledNextImage.width()) / 2,
            (height() - m_scaledNextImage.height()) / 2,
            m_scaledNextImage.width(),
            m_scaledNextImage.height()
        );
        painter.setOpacity(1.0 - m_fadeOpacity);
        painter.drawPixmap(nextRect, m_scaledNextImage);
    } else {
        // Normal display
        painter.setOpacity(1.0);
        if (!m_scaledCurrentImage.isNull()) {
            painter.drawPixmap(imageRect, m_scaledCurrentImage);
        }
    }

    // Draw slide counter (bottom right)
    if (m_controlsVisible && !m_slides.isEmpty()) {
        painter.setOpacity(0.8);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 14));
        QString counter = QString("%1 / %2").arg(m_currentIndex + 1).arg(m_slides.size());
        QRect counterRect(width() - 100, height() - 40, 90, 30);
        painter.drawText(counterRect, Qt::AlignRight | Qt::AlignVCenter, counter);
    }

    // Draw controls hint (center bottom)
    if (m_controlsVisible) {
        painter.setOpacity(0.6);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 12));
        QString hint = "Espace: Pause | Fleches: Navigation | Echap: Quitter | F11: Plein ecran";
        QRect hintRect(0, height() - 40, width(), 30);
        painter.drawText(hintRect, Qt::AlignCenter, hint);
    }
}

void SlideshowWidget::keyPressEvent(QKeyEvent* event) {
    showControls();

    switch (event->key()) {
        case Qt::Key_Space:
            if (m_isPaused) {
                resume();
            } else if (m_isPlaying) {
                pause();
            } else {
                start();
            }
            break;

        case Qt::Key_Right:
        case Qt::Key_Down:
        case Qt::Key_PageDown:
            nextSlide();
            break;

        case Qt::Key_Left:
        case Qt::Key_Up:
        case Qt::Key_PageUp:
            previousSlide();
            break;

        case Qt::Key_Home:
            goToSlide(0);
            break;

        case Qt::Key_End:
            goToSlide(m_slides.size() - 1);
            break;

        case Qt::Key_Escape:
            if (m_isFullscreen) {
                toggleFullscreen();
            } else {
                stop();
                close();
            }
            break;

        case Qt::Key_F11:
        case Qt::Key_F:
            toggleFullscreen();
            break;

        case Qt::Key_Plus:
        case Qt::Key_Equal:
            setVolume(qMin(100, volume() + 10));
            break;

        case Qt::Key_Minus:
            setVolume(qMax(0, volume() - 10));
            break;

        case Qt::Key_M:
            setVolume(volume() > 0 ? 0 : 80);
            break;

        default:
            QWidget::keyPressEvent(event);
    }
}

void SlideshowWidget::mouseMoveEvent(QMouseEvent* event) {
    Q_UNUSED(event)
    showControls();
}

void SlideshowWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    Q_UNUSED(event)
    toggleFullscreen();
}

void SlideshowWidget::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event)
    updateScaledImage();
}

void SlideshowWidget::onAudioPositionChanged(qint64 position) {
    if (m_slides.isEmpty()) return;

    // Calculate total progress
    int currentSlideStart = 0;
    for (int i = 0; i < m_currentIndex; ++i) {
        currentSlideStart += m_slides[i].audioDurationMs > 0
            ? m_slides[i].audioDurationMs
            : m_defaultSlideDurationMs;
    }

    int currentMs = currentSlideStart + static_cast<int>(position);
    int totalMs = calculateTotalDuration();

    emit progressChanged(currentMs, totalMs);
}

void SlideshowWidget::onAudioMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
        // Audio finished, move to next slide
        if (m_isPlaying && !m_isPaused) {
            nextSlide();
        }
    }
}

void SlideshowWidget::onSlideTimerTimeout() {
    // Timer finished (for slides without audio)
    if (m_isPlaying && !m_isPaused) {
        nextSlide();
    }
}

void SlideshowWidget::onFadeAnimationFinished() {
    m_inTransition = false;
    m_currentIndex = m_nextIndex;
    m_currentImage = m_nextImage;
    m_scaledCurrentImage = m_scaledNextImage;
    m_fadeOpacity = 1.0;

    emit slideChanged(m_currentIndex, m_slides.size());

    // Start audio or timer for new slide
    if (m_isPlaying && !m_isPaused) {
        playAudioForCurrentSlide();
    }

    update();
}

void SlideshowWidget::onControlsTimeout() {
    hideControls();
}

void SlideshowWidget::showSlide(int index) {
    if (index < 0 || index >= m_slides.size()) return;

    m_currentIndex = index;
    m_currentImage = m_slides[index].image;
    updateScaledImage();

    emit slideChanged(m_currentIndex, m_slides.size());

    // Start audio or timer
    playAudioForCurrentSlide();

    update();
}

void SlideshowWidget::startSlideTimer() {
    int duration = m_defaultSlideDurationMs;
    if (m_currentIndex < m_slides.size() && m_slides[m_currentIndex].audioDurationMs > 0) {
        duration = m_slides[m_currentIndex].audioDurationMs;
    }
    m_slideTimer->start(duration);
}

void SlideshowWidget::startFadeTransition() {
    if (m_nextIndex < 0 || m_nextIndex >= m_slides.size()) return;

    m_inTransition = true;
    m_slideTimer->stop();
    m_audioPlayer->stop();

    // Prepare next image
    m_nextImage = m_slides[m_nextIndex].image;

    // Scale next image
    if (!m_nextImage.isNull()) {
        m_scaledNextImage = m_nextImage.scaled(
            size(), Qt::KeepAspectRatio, Qt::SmoothTransformation
        );
    }

    // Start fade animation
    m_fadeAnimation->setDuration(m_transitionDurationMs);
    m_fadeAnimation->setStartValue(1.0);
    m_fadeAnimation->setEndValue(0.0);
    m_fadeAnimation->start();
}

void SlideshowWidget::playAudioForCurrentSlide() {
    if (m_currentIndex < 0 || m_currentIndex >= m_slides.size()) return;

    const SlideData& slide = m_slides[m_currentIndex];

    if (!slide.audioPath.isEmpty() && QFile::exists(slide.audioPath)) {
        // Play audio
        m_audioPlayer->setSource(QUrl::fromLocalFile(slide.audioPath));
        m_audioPlayer->play();
    } else {
        // No audio - use timer
        startSlideTimer();
    }
}

void SlideshowWidget::updateScaledImage() {
    if (!m_currentImage.isNull()) {
        m_scaledCurrentImage = m_currentImage.scaled(
            size(), Qt::KeepAspectRatio, Qt::SmoothTransformation
        );
    }
    update();
}

void SlideshowWidget::showControls() {
    m_controlsVisible = true;
    m_controlsTimer->start();
    update();
}

void SlideshowWidget::hideControls() {
    m_controlsVisible = false;
    update();
}

void SlideshowWidget::toggleFullscreen() {
    if (m_isFullscreen) {
        showNormal();
        m_isFullscreen = false;
    } else {
        showFullScreen();
        m_isFullscreen = true;
    }
}

int SlideshowWidget::calculateTotalDuration() const {
    int total = 0;
    for (const auto& slide : m_slides) {
        total += slide.audioDurationMs > 0 ? slide.audioDurationMs : m_defaultSlideDurationMs;
    }
    return total;
}

} // namespace codex::ui
