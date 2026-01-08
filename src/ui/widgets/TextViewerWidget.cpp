#include "TextViewerWidget.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QFont>

namespace codex::ui {

TextViewerWidget::TextViewerWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setWordWrapMode(QTextOption::WordWrap);

    // Set monospace font for better readability
    QFont font("Consolas", 11);
    font.setStyleHint(QFont::Monospace);
    m_textEdit->setFont(font);

    // Placeholder text
    m_textEdit->setPlaceholderText(
        "Ouvrez un fichier Codex (Fichier > Ouvrir Codex...)\n\n"
        "Sélectionnez ensuite un passage de texte pour le transformer en image."
    );

    layout->addWidget(m_textEdit);

    connect(m_textEdit, &QPlainTextEdit::selectionChanged,
            this, &TextViewerWidget::onSelectionChanged);
}

void TextViewerWidget::loadFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open file: %1").arg(filePath));
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    m_textEdit->setPlainText(content);
    LOG_INFO(QString("Loaded file: %1 (%2 chars)").arg(filePath).arg(content.length()));
}

void TextViewerWidget::setText(const QString& text) {
    m_textEdit->setPlainText(text);
}

QString TextViewerWidget::selectedText() const {
    return m_textEdit->textCursor().selectedText();
}

void TextViewerWidget::onSelectionChanged() {
    QTextCursor cursor = m_textEdit->textCursor();
    if (cursor.hasSelection()) {
        QString text = cursor.selectedText();
        int start = cursor.selectionStart();
        int end = cursor.selectionEnd();

        if (text.length() > 10) { // Minimum selection length
            emit passageSelected(text, start, end);
        }
    }
}

} // namespace codex::ui
