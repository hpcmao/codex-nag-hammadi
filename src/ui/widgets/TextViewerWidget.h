#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QPainter>

namespace codex::ui {

// Custom QPlainTextEdit with alternating line backgrounds
class AlternatingTextEdit : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit AlternatingTextEdit(QWidget* parent = nullptr);

    void setAlternatingColors(const QColor& even, const QColor& odd);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_evenColor;
    QColor m_oddColor;
};

class TextViewerWidget : public QWidget {
    Q_OBJECT

public:
    explicit TextViewerWidget(QWidget* parent = nullptr);

    void loadFile(const QString& filePath);
    void setText(const QString& text);
    void setTextWithVerses(const QString& text, int startPage);
    QString selectedText() const;
    void selectRange(int start, int end);

    // Update alternating colors from theme
    void updateColors();

signals:
    void passageSelected(const QString& text, int start, int end);

private slots:
    void onSelectionChanged();
    void onThemeChanged();

private:
    AlternatingTextEdit* m_textEdit;
};

} // namespace codex::ui
