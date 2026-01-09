#include "ImageViewerWidget.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QGroupBox>
#include <QDialog>
#include <QScreen>
#include <QGuiApplication>

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
    m_imageLabel->installEventFilter(this);  // For double-click
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

void ImageViewerWidget::addToPlate() {
    if (m_originalPixmap.isNull()) return;

    m_plateImages.append(m_originalPixmap);
    updatePlateStatus();

    LOG_INFO(QString("Added image to plate. Total: %1").arg(m_plateImages.size()));
}

void ImageViewerWidget::createPlate() {
    if (m_plateImages.isEmpty()) return;

    QSize gridSize = parsePlateSize();
    int cols = gridSize.width();
    int rows = gridSize.height();
    int totalCells = cols * rows;

    // Determine cell size based on first image
    int cellWidth = 512;  // Default cell width
    int cellHeight = 288; // 16:9 ratio

    if (!m_plateImages.isEmpty()) {
        // Use first image's aspect ratio
        QPixmap first = m_plateImages.first();
        cellWidth = first.width();
        cellHeight = first.height();
    }

    // Create the composite image
    int plateWidth = cols * cellWidth;
    int plateHeight = rows * cellHeight;

    QPixmap plate(plateWidth, plateHeight);
    plate.fill(Qt::black);

    QPainter painter(&plate);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    for (int i = 0; i < qMin(m_plateImages.size(), totalCells); ++i) {
        int row = i / cols;
        int col = i % cols;
        int x = col * cellWidth;
        int y = row * cellHeight;

        QPixmap scaled = m_plateImages[i].scaled(
            cellWidth, cellHeight,
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation
        );

        // Center crop if needed
        int srcX = (scaled.width() - cellWidth) / 2;
        int srcY = (scaled.height() - cellHeight) / 2;

        painter.drawPixmap(x, y, scaled, srcX, srcY, cellWidth, cellHeight);
    }

    painter.end();

    // Display the plate
    setImage(plate);
    emit plateCreated(plate);

    LOG_INFO(QString("Created plate %1x%2 with %3 images").arg(cols).arg(rows).arg(m_plateImages.size()));
}

void ImageViewerWidget::clearPlate() {
    m_plateImages.clear();
    updatePlateStatus();
    LOG_INFO("Plate cleared");
}

void ImageViewerWidget::updatePlateStatus() {
    // No longer used - plate controls removed
}

QSize ImageViewerWidget::parsePlateSize() const {
    // Return current grid size since combo was removed
    if (m_gridCols > 0 && m_gridRows > 0) {
        return QSize(m_gridCols, m_gridRows);
    }
    return QSize(2, 2);
}

void ImageViewerWidget::startPlateGrid(int cols, int rows) {
    m_gridCols = cols;
    m_gridRows = rows;
    m_gridMode = true;
    m_gridImages.clear();
    m_gridTexts.clear();

    int total = cols * rows;
    m_gridImages.resize(total);
    m_gridTexts.resize(total);

    m_saveBtn->setEnabled(false);

    m_statusLabel->setText(QString("Planche %1x%2 : selectionnez un passage et cliquez Generer")
                           .arg(cols).arg(rows));

    // Show initial empty grid
    updateGridDisplay();

    LOG_INFO(QString("Started plate grid generation: %1x%2").arg(cols).arg(rows));
}

void ImageViewerWidget::setPlateGridImage(int index, const QPixmap& image, const QString& text) {
    if (index < 0 || index >= m_gridImages.size()) return;

    m_gridImages[index] = image;
    m_gridTexts[index] = text;

    // Count completed images
    int completed = 0;
    for (const QPixmap& img : m_gridImages) {
        if (!img.isNull()) completed++;
    }

    m_statusLabel->setText(QString("Generation de planche %1x%2 : %3/%4 images")
                           .arg(m_gridCols).arg(m_gridRows)
                           .arg(completed).arg(m_gridImages.size()));

    updateGridDisplay();

    LOG_INFO(QString("Plate grid image %1 set").arg(index));
}

void ImageViewerWidget::finishPlateGrid() {
    m_gridMode = false;

    // Count completed images
    int completed = 0;
    for (const QPixmap& img : m_gridImages) {
        if (!img.isNull()) completed++;
    }

    m_statusLabel->setText(QString("Planche %1x%2 terminee : %3 images")
                           .arg(m_gridCols).arg(m_gridRows).arg(completed));

    m_saveBtn->setEnabled(true);

    // Copy grid images to plate collection for potential re-use
    m_plateImages = m_gridImages;
    updatePlateStatus();

    LOG_INFO(QString("Plate grid finished: %1 images").arg(completed));
}

void ImageViewerWidget::updateGridDisplay() {
    if (m_gridCols <= 0 || m_gridRows <= 0) return;

    // Determine cell size (use standard dimensions)
    int cellWidth = 512;
    int cellHeight = 288;  // 16:9

    // Check if we have any images to get dimensions from
    for (const QPixmap& img : m_gridImages) {
        if (!img.isNull()) {
            cellWidth = img.width();
            cellHeight = img.height();
            break;
        }
    }

    // Create the composite plate image
    int plateWidth = m_gridCols * cellWidth;
    int plateHeight = m_gridRows * cellHeight;

    QPixmap plate(plateWidth, plateHeight);
    plate.fill(QColor(30, 30, 30));  // Dark background

    QPainter painter(&plate);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    // Font for text overlay and placeholders
    QFont font("Segoe UI", 10);
    painter.setFont(font);

    int total = m_gridCols * m_gridRows;
    for (int i = 0; i < total; ++i) {
        int row = i / m_gridCols;
        int col = i % m_gridCols;
        int x = col * cellWidth;
        int y = row * cellHeight;

        if (i < m_gridImages.size() && !m_gridImages[i].isNull()) {
            // Draw the image
            QPixmap scaled = m_gridImages[i].scaled(
                cellWidth, cellHeight,
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation
            );

            // Center crop if needed
            int srcX = (scaled.width() - cellWidth) / 2;
            int srcY = (scaled.height() - cellHeight) / 2;

            painter.drawPixmap(x, y, scaled, srcX, srcY, cellWidth, cellHeight);

            // Draw text overlay at bottom
            if (i < m_gridTexts.size() && !m_gridTexts[i].isEmpty()) {
                QString text = m_gridTexts[i];
                if (text.length() > 100) {
                    text = text.left(97) + "...";
                }

                // Semi-transparent background for text
                QRect textBgRect(x, y + cellHeight - 40, cellWidth, 40);
                painter.fillRect(textBgRect, QColor(0, 0, 0, 180));

                // Draw text
                painter.setPen(QColor(220, 220, 220));
                QRect textRect(x + 5, y + cellHeight - 38, cellWidth - 10, 36);
                painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, text);
            }
        } else {
            // Draw placeholder cell
            QRect cellRect(x, y, cellWidth, cellHeight);
            painter.fillRect(cellRect, QColor(40, 40, 40));
            painter.setPen(QColor(80, 80, 80));
            painter.drawRect(cellRect.adjusted(0, 0, -1, -1));

            // Draw "loading" indicator or index number
            painter.setPen(QColor(100, 100, 100));
            QString indexText = QString("%1").arg(i + 1);
            painter.drawText(cellRect, Qt::AlignCenter, indexText);
        }

        // Draw cell border
        painter.setPen(QColor(60, 60, 60));
        painter.drawRect(x, y, cellWidth - 1, cellHeight - 1);
    }

    painter.end();

    // Display the composite plate
    m_originalPixmap = plate;
    m_zoomPercent = 100;

    // Auto-fit to view
    QSize viewSize = m_scrollArea->viewport()->size();
    if (viewSize.width() > 0 && viewSize.height() > 0) {
        double scaleW = static_cast<double>(viewSize.width() - 20) / plateWidth;
        double scaleH = static_cast<double>(viewSize.height() - 20) / plateHeight;
        double scale = qMin(scaleW, scaleH);
        if (scale < 1.0) {
            m_zoomPercent = static_cast<int>(scale * 100);
        }
    }

    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(m_zoomPercent);
    m_zoomSlider->blockSignals(false);

    updateZoomedImage();
}

bool ImageViewerWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_imageLabel && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && !m_originalPixmap.isNull()) {
            showFullSizeImage();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ImageViewerWidget::showFullSizeImage() {
    if (m_originalPixmap.isNull()) return;

    // Create fullscreen dialog
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Image - Taille Originale");
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Get screen size
    QScreen* screen = QGuiApplication::primaryScreen();
    QSize screenSize = screen->availableSize();

    // Scale image to fit screen if larger
    QPixmap displayPixmap = m_originalPixmap;
    if (displayPixmap.width() > screenSize.width() * 0.9 ||
        displayPixmap.height() > screenSize.height() * 0.9) {
        displayPixmap = displayPixmap.scaled(
            screenSize.width() * 0.9,
            screenSize.height() * 0.9,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
    }

    // Layout with scrollable image
    auto* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(dialog);
    scrollArea->setWidgetResizable(false);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setStyleSheet("background-color: #1a1a1a; border: none;");

    auto* imageLabel = new QLabel(dialog);
    imageLabel->setPixmap(displayPixmap);
    imageLabel->setAlignment(Qt::AlignCenter);
    scrollArea->setWidget(imageLabel);

    layout->addWidget(scrollArea);

    // Size dialog to fit image
    dialog->resize(displayPixmap.width() + 20, displayPixmap.height() + 40);
    dialog->exec();

    LOG_INFO("Displayed image in full size dialog");
}

} // namespace codex::ui
