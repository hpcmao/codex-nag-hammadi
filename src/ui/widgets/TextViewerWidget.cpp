#include "TextViewerWidget.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QRegularExpression>

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

void TextViewerWidget::setTextWithVerses(const QString& text, int startPage) {
    // Format text with Page:Paragraph verse numbers
    // Page markers in text: "|" indicates manuscript page change
    // Paragraphs are separated by double newlines or indentation

    QString formatted;
    int currentPage = startPage;
    int paragraphNum = 1;

    // Split by page markers first (| character)
    QStringList pages = text.split(QRegularExpression(R"(\s*\|\s*)"));

    for (int pageIdx = 0; pageIdx < pages.size(); ++pageIdx) {
        QString pageContent = pages[pageIdx].trimmed();
        if (pageContent.isEmpty()) continue;

        // Add page header
        if (pageIdx > 0) {
            currentPage++;
            paragraphNum = 1;
            formatted += QString("\n══════ Page %1 ══════\n\n").arg(currentPage);
        }

        // Split into paragraphs (double newline or significant indentation)
        QStringList paragraphs = pageContent.split(QRegularExpression(R"(\n\s*\n)"));

        for (const QString& para : paragraphs) {
            QString trimmedPara = para.trimmed();
            if (trimmedPara.isEmpty()) continue;

            // Skip footnotes and annotations (lines starting with *)
            if (trimmedPara.startsWith("*") || trimmedPara.startsWith("†")) {
                formatted += QString("    %1\n\n").arg(trimmedPara);
                continue;
            }

            // Add verse number
            QString verseRef = QString("[%1:%2] ").arg(currentPage).arg(paragraphNum);
            formatted += verseRef + trimmedPara + "\n\n";
            paragraphNum++;
        }
    }

    m_textEdit->setPlainText(formatted);
    LOG_INFO(QString("Formatted text with verses starting at page %1").arg(startPage));
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
