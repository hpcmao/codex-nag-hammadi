#include "ImageViewerWidget.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QWheelEvent>

namespace codex::ui {

ImageViewerWidget::ImageViewerWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    showPlaceholder();
}

void ImageViewerWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Title
    auto* titleLabel = new QLabel("Image Generee", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #d4d4d4;");
    titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mainLayout->addWidget(titleLabel);

    // Scroll area for image
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setStyleSheet(R"(
        QScrollArea {
            background-color: #1a1a1a;
            border: 1px solid #3d3d3d;
        }
    )");

    // Image label
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(200, 150);
    m_imageLabel->setStyleSheet("background-color: transparent;");
    m_scrollArea->setWidget(m_imageLabel);

    mainLayout->addWidget(m_scrollArea, 1);

    // Zoom controls row
    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(5);

    auto* zoomOutBtn = new QPushButton("-", this);
    zoomOutBtn->setFixedSize(30, 25);
    zoomOutBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3d3d3d;
            color: #d4d4d4;
            border: none;
            border-radius: 3px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #4d4d4d; }
    )");
    connect(zoomOutBtn, &QPushButton::clicked, this, &ImageViewerWidget::zoomOut);
    controlsLayout->addWidget(zoomOutBtn);

    m_zoomSlider = new QSlider(Qt::Horizontal, this);
    m_zoomSlider->setRange(MIN_ZOOM, MAX_ZOOM);
    m_zoomSlider->setValue(100);
    m_zoomSlider->setFixedWidth(100);
    m_zoomSlider->setStyleSheet(R"(
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
    connect(m_zoomSlider, &QSlider::valueChanged, this, &ImageViewerWidget::setZoom);
    controlsLayout->addWidget(m_zoomSlider);

    auto* zoomInBtn = new QPushButton("+", this);
    zoomInBtn->setFixedSize(30, 25);
    zoomInBtn->setStyleSheet(zoomOutBtn->styleSheet());
    connect(zoomInBtn, &QPushButton::clicked, this, &ImageViewerWidget::zoomIn);
    controlsLayout->addWidget(zoomInBtn);

    auto* fitBtn = new QPushButton("Ajuster", this);
    fitBtn->setFixedHeight(25);
    fitBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3d3d3d;
            color: #d4d4d4;
            border: none;
            border-radius: 3px;
            padding: 0 10px;
        }
        QPushButton:hover { background-color: #4d4d4d; }
    )");
    connect(fitBtn, &QPushButton::clicked, this, &ImageViewerWidget::zoomFit);
    controlsLayout->addWidget(fitBtn);

    auto* actualBtn = new QPushButton("100%", this);
    actualBtn->setFixedHeight(25);
    actualBtn->setStyleSheet(fitBtn->styleSheet());
    connect(actualBtn, &QPushButton::clicked, this, &ImageViewerWidget::zoomActual);
    controlsLayout->addWidget(actualBtn);

    controlsLayout->addStretch();

    m_saveBtn = new QPushButton("Sauvegarder", this);
    m_saveBtn->setEnabled(false);
    m_saveBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #094771;
            color: white;
            border: none;
            border-radius: 3px;
            padding: 5px 15px;
        }
        QPushButton:hover { background-color: #0a5a8c; }
        QPushButton:disabled {
            background-color: #3d3d3d;
            color: #666;
        }
    )");
    connect(m_saveBtn, &QPushButton::clicked, this, &ImageViewerWidget::imageSaveRequested);
    controlsLayout->addWidget(m_saveBtn);

    mainLayout->addLayout(controlsLayout);

    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #888; font-size: 10px;");
    m_statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mainLayout->addWidget(m_statusLabel);
}

void ImageViewerWidget::setImage(const QPixmap& pixmap) {
    m_originalPixmap = pixmap;
    m_zoomPercent = 100;
    m_zoomSlider->setValue(100);
    updateZoomedImage();

    m_saveBtn->setEnabled(true);
    m_statusLabel->setText(QString("Image: %1 x %2 | Zoom: %3%")
                           .arg(pixmap.width())
                           .arg(pixmap.height())
                           .arg(m_zoomPercent));

    LOG_INFO(QString("Image displayed: %1x%2").arg(pixmap.width()).arg(pixmap.height()));
}

void ImageViewerWidget::showLoading() {
    m_originalPixmap = QPixmap();
    m_imageLabel->clear();
    m_imageLabel->setText(
        "<div style='color: #888; text-align: center; padding: 50px;'>"
        "<p style='font-size: 24px;'>Generation en cours...</p>"
        "<p>Veuillez patienter</p>"
        "</div>"
    );
    m_saveBtn->setEnabled(false);
    m_statusLabel->setText("Generation en cours...");
}

void ImageViewerWidget::showPlaceholder() {
    m_originalPixmap = QPixmap();
    m_imageLabel->clear();
    m_imageLabel->setText(
        "<div style='color: #666; text-align: center; padding: 50px;'>"
        "<p style='font-size: 48px;'>&#128444;</p>"
        "<p>Selectionnez un passage de texte<br>puis cliquez sur \"Generer Image\"</p>"
        "</div>"
    );
    m_saveBtn->setEnabled(false);
    m_statusLabel->setText("En attente de generation");
}

void ImageViewerWidget::clear() {
    m_originalPixmap = QPixmap();
    m_imageLabel->clear();
    m_saveBtn->setEnabled(false);
    m_statusLabel->clear();
}

void ImageViewerWidget::zoomIn() {
    setZoom(m_zoomPercent + ZOOM_STEP);
}

void ImageViewerWidget::zoomOut() {
    setZoom(m_zoomPercent - ZOOM_STEP);
}

void ImageViewerWidget::zoomFit() {
    if (m_originalPixmap.isNull()) return;

    QSize viewSize = m_scrollArea->viewport()->size();
    QSize imgSize = m_originalPixmap.size();

    double scaleW = static_cast<double>(viewSize.width()) / imgSize.width();
    double scaleH = static_cast<double>(viewSize.height()) / imgSize.height();
    double scale = qMin(scaleW, scaleH);

    setZoom(static_cast<int>(scale * 100));
}

void ImageViewerWidget::zoomActual() {
    setZoom(100);
}

void ImageViewerWidget::setZoom(int percent) {
    percent = qBound(MIN_ZOOM, percent, MAX_ZOOM);

    if (percent != m_zoomPercent) {
        m_zoomPercent = percent;
        m_zoomSlider->blockSignals(true);
        m_zoomSlider->setValue(percent);
        m_zoomSlider->blockSignals(false);

        updateZoomedImage();
        emit zoomChanged(percent);
    }
}

void ImageViewerWidget::updateZoomedImage() {
    if (m_originalPixmap.isNull()) return;

    int newWidth = m_originalPixmap.width() * m_zoomPercent / 100;
    int newHeight = m_originalPixmap.height() * m_zoomPercent / 100;

    QPixmap scaled = m_originalPixmap.scaled(
        newWidth, newHeight,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    m_imageLabel->setPixmap(scaled);
    m_imageLabel->resize(scaled.size());

    m_statusLabel->setText(QString("Image: %1 x %2 | Zoom: %3%")
                           .arg(m_originalPixmap.width())
                           .arg(m_originalPixmap.height())
                           .arg(m_zoomPercent));
}

} // namespace codex::ui
