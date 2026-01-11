#pragma once

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QScrollArea>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QVector>
#include <QDialog>

namespace codex::ui {

class ImageViewerWidget : public QWidget {
    Q_OBJECT

public:
    explicit ImageViewerWidget(QWidget* parent = nullptr);

    void setImage(const QPixmap& pixmap);
    void showLoading();
    void showPlaceholder();
    void clear();

    QPixmap currentImage() const { return m_originalPixmap; }
    bool hasImage() const { return !m_originalPixmap.isNull(); }

    // Plate (grid) functionality
    void addToPlate();
    void createPlate();
    void clearPlate();
    int plateImageCount() const { return m_plateImages.size(); }

    // Progressive plate generation display
    void startPlateGrid(int cols, int rows);
    void setPlateGridImage(int index, const QPixmap& image, const QString& text);
    void finishPlateGrid();

    // Grid accessors
    int gridCols() const { return m_gridCols; }
    int gridRows() const { return m_gridRows; }
    int gridImageCount() const { return m_gridImages.size(); }
    QPixmap gridImage(int index) const { return (index >= 0 && index < m_gridImages.size()) ? m_gridImages[index] : QPixmap(); }
    QString gridText(int index) const { return (index >= 0 && index < m_gridTexts.size()) ? m_gridTexts[index] : QString(); }

public slots:
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void zoomActual();
    void setZoom(int percent);

signals:
    void zoomChanged(int percent);
    void imageSaveRequested();
    void plateCreated(const QPixmap& plate);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void showFullSizeImage();
    void setupUi();
    void updateZoomedImage();

    QScrollArea* m_scrollArea;
    QLabel* m_imageLabel;
    QLabel* m_statusLabel;
    QSlider* m_zoomSlider;
    QPushButton* m_saveBtn;

    QPixmap m_originalPixmap;
    int m_zoomPercent = 100;

    // Plate functionality
    QVector<QPixmap> m_plateImages;
    QComboBox* m_plateSizeCombo = nullptr;
    QPushButton* m_addToPlateBtn = nullptr;
    QPushButton* m_createPlateBtn = nullptr;
    QPushButton* m_clearPlateBtn = nullptr;
    QLabel* m_plateStatusLabel = nullptr;

    void updatePlateStatus();
    QSize parsePlateSize() const;
    void updateGridDisplay();

    // Grid generation state
    int m_gridCols = 0;
    int m_gridRows = 0;
    QVector<QPixmap> m_gridImages;
    QVector<QString> m_gridTexts;
    bool m_gridMode = false;

    static constexpr int MIN_ZOOM = 10;
    static constexpr int MAX_ZOOM = 400;
    static constexpr int ZOOM_STEP = 25;
};

} // namespace codex::ui
