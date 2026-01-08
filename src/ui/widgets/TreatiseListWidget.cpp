#include "TreatiseListWidget.h"
#include "core/services/TextParser.h"
#include "core/services/MythicClassifier.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QFrame>

namespace codex::ui {

TreatiseListWidget::TreatiseListWidget(QWidget* parent)
    : QWidget(parent)
{
    m_classifier = new codex::core::MythicClassifier();
    setupUi();
}

void TreatiseListWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // Title
    auto* titleLabel = new QLabel("Traites du Codex", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #d4d4d4;");
    layout->addWidget(titleLabel);

    // Search field
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Rechercher un traite...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #2d2d2d;
            color: #d4d4d4;
            border: 1px solid #3d3d3d;
            border-radius: 3px;
            padding: 5px;
        }
        QLineEdit:focus {
            border-color: #007acc;
        }
    )");
    layout->addWidget(m_searchEdit);

    // Tree widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels({"Code", "Titre", "Pages"});
    m_treeWidget->setColumnCount(3);
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeWidget->setAnimated(true);

    // Column widths
    m_treeWidget->header()->setStretchLastSection(true);
    m_treeWidget->setColumnWidth(0, 70);
    m_treeWidget->setColumnWidth(1, 250);
    m_treeWidget->setColumnWidth(2, 60);

    // Styling
    m_treeWidget->setStyleSheet(R"(
        QTreeWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
            border: 1px solid #3d3d3d;
            alternate-background-color: #252525;
        }
        QTreeWidget::item {
            padding: 3px;
        }
        QTreeWidget::item:selected {
            background-color: #094771;
        }
        QTreeWidget::item:hover {
            background-color: #2a2d2e;
        }
        QTreeWidget::branch {
            background-color: #1e1e1e;
        }
        QHeaderView::section {
            background-color: #2d2d2d;
            color: #d4d4d4;
            padding: 5px;
            border: none;
            border-right: 1px solid #3d3d3d;
        }
    )");

    layout->addWidget(m_treeWidget, 1);

    // Category display
    auto* categoryFrame = new QFrame(this);
    categoryFrame->setStyleSheet(R"(
        QFrame {
            background-color: #252525;
            border: 1px solid #3d3d3d;
            border-radius: 3px;
            padding: 5px;
        }
    )");
    auto* categoryLayout = new QVBoxLayout(categoryFrame);
    categoryLayout->setContentsMargins(8, 5, 8, 5);
    categoryLayout->setSpacing(2);

    auto* categoryTitle = new QLabel("Categorie Mythique", categoryFrame);
    categoryTitle->setStyleSheet("color: #888; font-size: 10px; border: none;");
    categoryLayout->addWidget(categoryTitle);

    m_categoryLabel = new QLabel("Aucun traite selectionne", categoryFrame);
    m_categoryLabel->setStyleSheet("color: #569cd6; font-weight: bold; font-size: 12px; border: none;");
    categoryLayout->addWidget(m_categoryLabel);

    layout->addWidget(categoryFrame);

    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &TreatiseListWidget::onSearchTextChanged);
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged,
            this, &TreatiseListWidget::onItemSelectionChanged);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &TreatiseListWidget::onItemDoubleClicked);
}

void TreatiseListWidget::loadTreatises(const QVector<codex::core::TreatiseInfo>& treatises) {
    m_treatises = treatises;
    m_treeWidget->clear();
    m_codexItems.clear();

    for (const auto& treatise : treatises) {
        QString codexName = getCodexName(treatise.code);

        // Create codex parent item if not exists
        if (!m_codexItems.contains(codexName)) {
            auto* codexItem = new QTreeWidgetItem(m_treeWidget);
            codexItem->setText(0, codexName);
            codexItem->setExpanded(true);
            codexItem->setFlags(codexItem->flags() & ~Qt::ItemIsSelectable);

            // Style parent items
            QFont font = codexItem->font(0);
            font.setBold(true);
            codexItem->setFont(0, font);
            codexItem->setForeground(0, QColor("#569cd6"));

            m_codexItems[codexName] = codexItem;
        }

        // Add treatise item
        auto* item = new QTreeWidgetItem(m_codexItems[codexName]);
        item->setText(0, treatise.code);
        item->setText(1, treatise.title);
        item->setText(2, treatise.pages);
        item->setData(0, Qt::UserRole, treatise.code);
        item->setData(1, Qt::UserRole, treatise.title);

        // Color code by codex
        if (codexName.startsWith("Codex I")) {
            item->setForeground(0, QColor("#4ec9b0"));
        } else if (codexName.startsWith("Codex II")) {
            item->setForeground(0, QColor("#dcdcaa"));
        } else if (codexName.startsWith("Codex III")) {
            item->setForeground(0, QColor("#c586c0"));
        } else if (codexName.startsWith("BG 8502")) {
            item->setForeground(0, QColor("#ce9178"));
        }
    }

    LOG_INFO(QString("Loaded %1 treatises in %2 codices")
             .arg(treatises.size()).arg(m_codexItems.size()));
}

QString TreatiseListWidget::getCodexName(const QString& code) const {
    if (code.startsWith("I-")) return "Codex I";
    if (code.startsWith("II-")) return "Codex II";
    if (code.startsWith("III-")) return "Codex III";
    if (code.startsWith("IV-")) return "Codex IV";
    if (code.startsWith("V-")) return "Codex V";
    if (code.startsWith("VI-")) return "Codex VI";
    if (code.startsWith("VII-")) return "Codex VII";
    if (code.startsWith("VIII-")) return "Codex VIII";
    if (code.startsWith("IX-")) return "Codex IX";
    if (code.startsWith("X-") || code == "X") return "Codex X";
    if (code.startsWith("XI-")) return "Codex XI";
    if (code.startsWith("XII-")) return "Codex XII";
    if (code.startsWith("XIII-")) return "Codex XIII";
    if (code.startsWith("8502-")) return "BG 8502";
    return "Autres";
}

void TreatiseListWidget::onSearchTextChanged(const QString& text) {
    applyFilter(text);
}

void TreatiseListWidget::applyFilter(const QString& filter) {
    QString lowerFilter = filter.toLower();

    for (auto it = m_codexItems.begin(); it != m_codexItems.end(); ++it) {
        QTreeWidgetItem* codexItem = it.value();
        bool hasVisibleChild = false;

        for (int i = 0; i < codexItem->childCount(); ++i) {
            QTreeWidgetItem* child = codexItem->child(i);
            QString code = child->text(0).toLower();
            QString title = child->text(1).toLower();

            bool matches = filter.isEmpty() ||
                           code.contains(lowerFilter) ||
                           title.contains(lowerFilter);

            child->setHidden(!matches);
            if (matches) hasVisibleChild = true;
        }

        // Hide codex if no visible children
        codexItem->setHidden(!hasVisibleChild && !filter.isEmpty());
    }
}

void TreatiseListWidget::onItemSelectionChanged() {
    QList<QTreeWidgetItem*> selected = m_treeWidget->selectedItems();
    if (selected.isEmpty()) {
        m_categoryLabel->setText("Aucun traite selectionne");
        return;
    }

    QTreeWidgetItem* item = selected.first();

    // Only emit for treatise items (not codex parents)
    if (item->parent() != nullptr) {
        QString code = item->data(0, Qt::UserRole).toString();
        QString title = item->data(1, Qt::UserRole).toString();
        QString category = getCategoryForCode(code);

        // Update category display
        m_categoryLabel->setText(category);

        emit treatiseSelected(code, title, category);
    }
}

void TreatiseListWidget::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)

    if (item && item->parent() != nullptr) {
        QString code = item->data(0, Qt::UserRole).toString();
        QString title = item->data(1, Qt::UserRole).toString();
        QString category = getCategoryForCode(code);
        emit treatiseDoubleClicked(code, title, category);
    }
}

QString TreatiseListWidget::getCategoryForCode(const QString& code) const {
    if (!m_classifier) return "Inconnu";

    codex::core::MythicCategory cat = m_classifier->classifyTreatise(code);
    return m_classifier->categoryName(cat);
}

QString TreatiseListWidget::selectedTreatiseCode() const {
    QList<QTreeWidgetItem*> selected = m_treeWidget->selectedItems();
    if (selected.isEmpty()) return QString();

    QTreeWidgetItem* item = selected.first();
    if (item->parent() != nullptr) {
        return item->data(0, Qt::UserRole).toString();
    }
    return QString();
}

void TreatiseListWidget::clear() {
    m_treeWidget->clear();
    m_codexItems.clear();
    m_treatises.clear();
}

} // namespace codex::ui
