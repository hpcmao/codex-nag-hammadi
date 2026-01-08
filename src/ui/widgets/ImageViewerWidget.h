#pragma once

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QScrollArea>
#include <QPushButton>
#include <QSlider>

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

public slots:
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void zoomActual();
    void setZoom(int percent);

signals:
    void zoomChanged(int percent);
    void imageSaveRequested();

private:
    void setupUi();
    void updateZoomedImage();

    QScrollArea* m_scrollArea;
    QLabel* m_imageLabel;
    QLabel* m_statusLabel;
    QSlider* m_zoomSlider;
    QPushButton* m_saveBtn;

    QPixmap m_originalPixmap;
    int m_zoomPercent = 100;

    static constexpr int MIN_ZOOM = 10;
    static constexpr int MAX_ZOOM = 400;
    static constexpr int ZOOM_STEP = 25;
};

} // namespace codex::ui
