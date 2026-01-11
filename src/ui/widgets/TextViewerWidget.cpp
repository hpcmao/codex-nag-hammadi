#include "TextViewerWidget.h"
#include "utils/Logger.h"
#include "utils/ThemeManager.h"
#include "core/services/NarrationCleaner.h"

#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBlock>
#include <QPaintEvent>
#include <QPalette>

namespace codex::ui {

// ============================================================================
// AlternatingTextEdit implementation
// ============================================================================

AlternatingTextEdit::AlternatingTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
    , m_evenColor("#1e1e1e")
    , m_oddColor("#262626")
{
}

void AlternatingTextEdit::setAlternatingColors(const QColor& even, const QColor& odd) {
    m_evenColor = even;
    m_oddColor = odd;
    viewport()->update();
}

void AlternatingTextEdit::paintEvent(QPaintEvent* event) {
    // First, paint the alternating row backgrounds
    QPainter painter(viewport());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            // Determine if even or odd line
            QColor bgColor = (blockNumber % 2 == 0) ? m_evenColor : m_oddColor;

            // Draw the background rectangle for this line
            QRectF lineRect(0, top, viewport()->width(), blockBoundingRect(block).height());
            painter.fillRect(lineRect, bgColor);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }

    painter.end();

    // Now call the base class to paint the text on top
    QPlainTextEdit::paintEvent(event);
}

// ============================================================================
// TextViewerWidget implementation
// ============================================================================

TextViewerWidget::TextViewerWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_textEdit = new AlternatingTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setWordWrapMode(QTextOption::WordWrap);

    // Set monospace font for better readability
    auto& theme = codex::utils::ThemeManager::instance();
    QFont font(theme.fontSettings().textFamily, theme.fontSettings().textSize);
    font.setStyleHint(QFont::Monospace);
    m_textEdit->setFont(font);

    // Set alternating colors from theme
    updateColors();

    // Placeholder text
    m_textEdit->setPlaceholderText(
        "Ouvrez un fichier Codex (Fichier > Ouvrir Codex...)\n\n"
        "Selectionnez ensuite un passage de texte pour le transformer en image."
    );

    layout->addWidget(m_textEdit);

    connect(m_textEdit, &QPlainTextEdit::selectionChanged,
            this, &TextViewerWidget::onSelectionChanged);

    // Connect to theme changes
    connect(&codex::utils::ThemeManager::instance(), &codex::utils::ThemeManager::themeChanged,
            this, &TextViewerWidget::onThemeChanged);
}

void TextViewerWidget::updateColors() {
    auto& theme = codex::utils::ThemeManager::instance();
    auto colors = theme.colors();

    m_textEdit->setAlternatingColors(
        QColor(colors.rowEven),
        QColor(colors.rowOdd)
    );

    // Also update font
    QFont font(theme.fontSettings().textFamily, theme.fontSettings().textSize);
    font.setStyleHint(QFont::Monospace);
    m_textEdit->setFont(font);

    // Selection colors from theme
    QPalette palette = m_textEdit->palette();
    palette.setColor(QPalette::Highlight, QColor(colors.selection));
    palette.setColor(QPalette::HighlightedText, QColor(colors.selectionText));
    m_textEdit->setPalette(palette);
}

void TextViewerWidget::onThemeChanged() {
    updateColors();
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

    // Clean the text first (remove annotations, notes, etc.)
    codex::core::NarrationCleaner cleaner;
    cleaner.setRemovePipes(false);  // Keep pipes for page splitting
    QString cleanedText = cleaner.clean(text);

    QString formatted;
    int currentPage = startPage;
    int paragraphNum = 1;

    // Split by page markers first (| character)
    QStringList pages = cleanedText.split(QRegularExpression(R"(\s*\|\s*)"));

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

void TextViewerWidget::selectRange(int start, int end) {
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    m_textEdit->setTextCursor(cursor);
    m_textEdit->ensureCursorVisible();
}

} // namespace codex::ui
