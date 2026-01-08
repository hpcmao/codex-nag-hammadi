#pragma once

#include <QWidget>
#include <QPlainTextEdit>

namespace codex::ui {

class TextViewerWidget : public QWidget {
    Q_OBJECT

public:
    explicit TextViewerWidget(QWidget* parent = nullptr);

    void loadFile(const QString& filePath);
    void setText(const QString& text);
    QString selectedText() const;

signals:
    void passageSelected(const QString& text, int start, int end);

private slots:
    void onSelectionChanged();

private:
    QPlainTextEdit* m_textEdit;
};

} // namespace codex::ui
