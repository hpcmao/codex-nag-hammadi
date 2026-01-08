#include "PassagePreviewWidget.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QRegularExpression>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFont>

namespace codex::ui {

PassagePreviewWidget::PassagePreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void PassagePreviewWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Title
    auto* titleLabel = new QLabel("Passage Selectionne", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #d4d4d4;");
    mainLayout->addWidget(titleLabel);

    // Preview text area
    m_previewEdit = new QTextEdit(this);
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setMaximumHeight(150);
    m_previewEdit->setPlaceholderText("Selectionnez un passage de texte dans le panneau central...");
    m_previewEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: #252525;
            color: #d4d4d4;
            border: 1px solid #3d3d3d;
            border-radius: 3px;
            padding: 5px;
            font-size: 11px;
        }
    )");
    mainLayout->addWidget(m_previewEdit);

    // Stats row
    auto* statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(15);

    m_statsLabel = new QLabel("0 caracteres | 0 mots", this);
    m_statsLabel->setStyleSheet("color: #888; font-size: 10px;");
    statsLayout->addWidget(m_statsLabel);

    m_positionLabel = new QLabel("", this);
    m_positionLabel->setStyleSheet("color: #666; font-size: 10px;");
    statsLayout->addWidget(m_positionLabel);

    statsLayout->addStretch();
    mainLayout->addLayout(statsLayout);

    // Buttons row
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    m_generateImageBtn = new QPushButton("Generer Image", this);
    m_generateImageBtn->setEnabled(false);
    m_generateImageBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #094771;
            color: white;
            border: none;
            padding: 8px 15px;
            border-radius: 3px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #0a5a8c;
        }
        QPushButton:disabled {
            background-color: #3d3d3d;
            color: #666;
        }
    )");
    connect(m_generateImageBtn, &QPushButton::clicked, this, &PassagePreviewWidget::onGenerateImage);
    buttonLayout->addWidget(m_generateImageBtn);

    m_generateAudioBtn = new QPushButton("Generer Audio", this);
    m_generateAudioBtn->setEnabled(false);
    m_generateAudioBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #4a4a4a;
            color: white;
            border: none;
            padding: 8px 15px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: #5a5a5a;
        }
        QPushButton:disabled {
            background-color: #3d3d3d;
            color: #666;
        }
    )");
    connect(m_generateAudioBtn, &QPushButton::clicked, this, &PassagePreviewWidget::onGenerateAudio);
    buttonLayout->addWidget(m_generateAudioBtn);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Set fixed height for the entire widget
    setMaximumHeight(250);
}

void PassagePreviewWidget::setPassage(const QString& text, int startPos, int endPos) {
    m_passage = text;
    m_startPos = startPos;
    m_endPos = endPos;

    // Display passage (truncate if very long for preview)
    QString displayText = text;
    if (displayText.length() > 500) {
        displayText = displayText.left(500) + "...";
    }
    m_previewEdit->setPlainText(displayText);

    updateStats();

    // Enable buttons if passage is valid (minimum 20 characters)
    bool valid = text.length() >= 20;
    m_generateImageBtn->setEnabled(valid);
    m_generateAudioBtn->setEnabled(valid);

    if (valid) {
        LOG_INFO(QString("Passage selected: %1 chars, %2 words")
                 .arg(text.length())
                 .arg(text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count()));
    }
}

void PassagePreviewWidget::clear() {
    m_passage.clear();
    m_startPos = 0;
    m_endPos = 0;
    m_previewEdit->clear();
    m_statsLabel->setText("0 caracteres | 0 mots");
    m_positionLabel->clear();
    m_generateImageBtn->setEnabled(false);
    m_generateAudioBtn->setEnabled(false);
}

void PassagePreviewWidget::updateStats() {
    int charCount = m_passage.length();
    int wordCount = m_passage.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();

    m_statsLabel->setText(QString("%1 caracteres | %2 mots").arg(charCount).arg(wordCount));
    m_positionLabel->setText(QString("Position: %1-%2").arg(m_startPos).arg(m_endPos));
}

void PassagePreviewWidget::onGenerateImage() {
    if (!m_passage.isEmpty()) {
        emit generateImageRequested(m_passage);
    }
}

void PassagePreviewWidget::onGenerateAudio() {
    if (!m_passage.isEmpty()) {
        emit generateAudioRequested(m_passage);
    }
}

} // namespace codex::ui
