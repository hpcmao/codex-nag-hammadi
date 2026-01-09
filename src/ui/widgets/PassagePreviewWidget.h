#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>

namespace codex::ui {

class PassagePreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit PassagePreviewWidget(QWidget* parent = nullptr);

    // Set the selected passage
    void setPassage(const QString& text, int startPos, int endPos);

    // Set current treatise code (for favorites)
    void setTreatiseCode(const QString& code);

    // Clear the preview
    void clear();

    // Get current passage
    QString passage() const { return m_passage; }
    bool hasPassage() const { return !m_passage.isEmpty(); }

signals:
    void favoriteChanged();

private slots:
    void onToggleStar();
    void onToggleHeart();

private:
    void setupUi();
    void updateStats();
    void updateFavoriteButtons();

    QTextEdit* m_previewEdit;
    QLabel* m_statsLabel;
    QLabel* m_positionLabel;
    QPushButton* m_starBtn;
    QPushButton* m_heartBtn;

    QString m_passage;
    QString m_treatiseCode;
    int m_startPos = 0;
    int m_endPos = 0;
};

} // namespace codex::ui
