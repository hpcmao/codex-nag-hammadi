#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>

namespace codex::ui {

class PassagePreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit PassagePreviewWidget(QWidget* parent = nullptr);

    // Set the selected passage
    void setPassage(const QString& text, int startPos, int endPos);

    // Clear the preview
    void clear();

    // Get current passage
    QString passage() const { return m_passage; }
    bool hasPassage() const { return !m_passage.isEmpty(); }

signals:
    void generateImageRequested(const QString& passage);
    void generateAudioRequested(const QString& passage);

private slots:
    void onGenerateImage();
    void onGenerateAudio();

private:
    void setupUi();
    void updateStats();

    QTextEdit* m_previewEdit;
    QLabel* m_statsLabel;
    QLabel* m_positionLabel;
    QPushButton* m_generateImageBtn;
    QPushButton* m_generateAudioBtn;

    QString m_passage;
    int m_startPos = 0;
    int m_endPos = 0;
};

} // namespace codex::ui
