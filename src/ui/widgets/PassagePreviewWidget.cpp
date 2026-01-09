#include "PassagePreviewWidget.h"
#include "utils/Logger.h"
#include "db/repositories/FavoriteRepository.h"

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

    // Title row with favorite buttons
    auto* titleLayout = new QHBoxLayout();

    auto* titleLabel = new QLabel("Passage Selectionne", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #d4d4d4;");
    titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    titleLayout->addWidget(titleLabel);

    titleLayout->addStretch();

    // Star button
    m_starBtn = new QPushButton(this);
    m_starBtn->setText(QString::fromUtf8("\u2606"));  // Empty star
    m_starBtn->setToolTip("Marquer avec une etoile");
    m_starBtn->setFixedSize(30, 30);
    m_starBtn->setEnabled(false);
    m_starBtn->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: 1px solid #3d3d3d;
            border-radius: 15px;
            font-size: 16px;
            color: #888;
        }
        QPushButton:hover {
            background-color: #3d3d3d;
            color: #ffd700;
        }
        QPushButton:disabled {
            color: #444;
            border-color: #2d2d2d;
        }
    )");
    connect(m_starBtn, &QPushButton::clicked, this, &PassagePreviewWidget::onToggleStar);
    titleLayout->addWidget(m_starBtn);

    // Heart button
    m_heartBtn = new QPushButton(this);
    m_heartBtn->setText(QString::fromUtf8("\u2661"));  // Empty heart
    m_heartBtn->setToolTip("Marquer comme favori (coeur)");
    m_heartBtn->setFixedSize(30, 30);
    m_heartBtn->setEnabled(false);
    m_heartBtn->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: 1px solid #3d3d3d;
            border-radius: 15px;
            font-size: 16px;
            color: #888;
        }
        QPushButton:hover {
            background-color: #3d3d3d;
            color: #ff4444;
        }
        QPushButton:disabled {
            color: #444;
            border-color: #2d2d2d;
        }
    )");
    connect(m_heartBtn, &QPushButton::clicked, this, &PassagePreviewWidget::onToggleHeart);
    titleLayout->addWidget(m_heartBtn);

    mainLayout->addLayout(titleLayout);

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
    m_statsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statsLayout->addWidget(m_statsLabel);

    m_positionLabel = new QLabel("", this);
    m_positionLabel->setStyleSheet("color: #666; font-size: 10px;");
    m_positionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statsLayout->addWidget(m_positionLabel);

    statsLayout->addStretch();
    mainLayout->addLayout(statsLayout);

    // Favorites buttons row (generation buttons moved to menu)
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Set fixed height for the entire widget (reduced since generation buttons removed)
    setMaximumHeight(180);
}

void PassagePreviewWidget::setTreatiseCode(const QString& code) {
    m_treatiseCode = code;
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

    // Enable favorite buttons if passage is valid (minimum 20 characters)
    // Generation buttons are now in the menu
    bool valid = text.length() >= 20;
    m_starBtn->setEnabled(valid && !m_treatiseCode.isEmpty());
    m_heartBtn->setEnabled(valid && !m_treatiseCode.isEmpty());

    // Update favorite button states
    updateFavoriteButtons();

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
    m_starBtn->setEnabled(false);
    m_heartBtn->setEnabled(false);

    // Reset button icons
    m_starBtn->setText(QString::fromUtf8("\u2606"));  // Empty star
    m_heartBtn->setText(QString::fromUtf8("\u2661"));  // Empty heart
}

void PassagePreviewWidget::updateStats() {
    int charCount = m_passage.length();
    int wordCount = m_passage.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();

    m_statsLabel->setText(QString("%1 caracteres | %2 mots").arg(charCount).arg(wordCount));
    m_positionLabel->setText(QString("Position: %1-%2").arg(m_startPos).arg(m_endPos));
}

void PassagePreviewWidget::updateFavoriteButtons() {
    if (m_treatiseCode.isEmpty() || m_passage.isEmpty()) {
        m_starBtn->setText(QString::fromUtf8("\u2606"));  // Empty star
        m_heartBtn->setText(QString::fromUtf8("\u2661"));  // Empty heart
        return;
    }

    codex::db::FavoriteRepository repo;
    auto favorite = repo.getFavorite(m_treatiseCode, m_startPos, m_endPos);

    if (favorite.has_value()) {
        if (favorite->type == codex::db::FavoriteType::Star) {
            m_starBtn->setText(QString::fromUtf8("\u2605"));  // Filled star
            m_starBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: transparent;
                    border: 1px solid #ffd700;
                    border-radius: 15px;
                    font-size: 16px;
                    color: #ffd700;
                }
                QPushButton:hover {
                    background-color: #3d3d3d;
                }
            )");
            m_heartBtn->setText(QString::fromUtf8("\u2661"));  // Empty heart
            m_heartBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: transparent;
                    border: 1px solid #3d3d3d;
                    border-radius: 15px;
                    font-size: 16px;
                    color: #888;
                }
                QPushButton:hover {
                    background-color: #3d3d3d;
                    color: #ff4444;
                }
            )");
        } else {
            m_heartBtn->setText(QString::fromUtf8("\u2665"));  // Filled heart
            m_heartBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: transparent;
                    border: 1px solid #ff4444;
                    border-radius: 15px;
                    font-size: 16px;
                    color: #ff4444;
                }
                QPushButton:hover {
                    background-color: #3d3d3d;
                }
            )");
            m_starBtn->setText(QString::fromUtf8("\u2606"));  // Empty star
            m_starBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: transparent;
                    border: 1px solid #3d3d3d;
                    border-radius: 15px;
                    font-size: 16px;
                    color: #888;
                }
                QPushButton:hover {
                    background-color: #3d3d3d;
                    color: #ffd700;
                }
            )");
        }
    } else {
        // Not favorited - reset to default empty state
        m_starBtn->setText(QString::fromUtf8("\u2606"));
        m_starBtn->setStyleSheet(R"(
            QPushButton {
                background-color: transparent;
                border: 1px solid #3d3d3d;
                border-radius: 15px;
                font-size: 16px;
                color: #888;
            }
            QPushButton:hover {
                background-color: #3d3d3d;
                color: #ffd700;
            }
            QPushButton:disabled {
                color: #444;
                border-color: #2d2d2d;
            }
        )");
        m_heartBtn->setText(QString::fromUtf8("\u2661"));
        m_heartBtn->setStyleSheet(R"(
            QPushButton {
                background-color: transparent;
                border: 1px solid #3d3d3d;
                border-radius: 15px;
                font-size: 16px;
                color: #888;
            }
            QPushButton:hover {
                background-color: #3d3d3d;
                color: #ff4444;
            }
            QPushButton:disabled {
                color: #444;
                border-color: #2d2d2d;
            }
        )");
    }
}

void PassagePreviewWidget::onToggleStar() {
    if (m_treatiseCode.isEmpty() || m_passage.isEmpty()) {
        return;
    }

    codex::db::FavoriteRepository repo;
    repo.toggleFavorite(m_treatiseCode, m_passage.left(200), m_startPos, m_endPos,
                        codex::db::FavoriteType::Star);
    updateFavoriteButtons();
    emit favoriteChanged();

    LOG_INFO(QString("Star toggled for passage in %1").arg(m_treatiseCode));
}

void PassagePreviewWidget::onToggleHeart() {
    if (m_treatiseCode.isEmpty() || m_passage.isEmpty()) {
        return;
    }

    codex::db::FavoriteRepository repo;
    repo.toggleFavorite(m_treatiseCode, m_passage.left(200), m_startPos, m_endPos,
                        codex::db::FavoriteType::Heart);
    updateFavoriteButtons();
    emit favoriteChanged();

    LOG_INFO(QString("Heart toggled for passage in %1").arg(m_treatiseCode));
}

} // namespace codex::ui
