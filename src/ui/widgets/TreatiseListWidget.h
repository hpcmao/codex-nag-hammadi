#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QLabel>
#include <QMap>
#include "core/services/TextParser.h"

namespace codex::core {
class MythicClassifier;
}

namespace codex::ui {

class TreatiseListWidget : public QWidget {
    Q_OBJECT

public:
    explicit TreatiseListWidget(QWidget* parent = nullptr);

    // Load treatises from parser
    void loadTreatises(const QVector<codex::core::TreatiseInfo>& treatises);

    // Get currently selected treatise code
    QString selectedTreatiseCode() const;

    // Clear the list
    void clear();

signals:
    void treatiseSelected(const QString& code, const QString& title, const QString& category);
    void treatiseDoubleClicked(const QString& code, const QString& title, const QString& category);

private slots:
    void onSearchTextChanged(const QString& text);
    void onItemSelectionChanged();
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    void setupUi();
    void applyFilter(const QString& filter);
    QString getCodexName(const QString& code) const;
    QString getCategoryForCode(const QString& code) const;

    QLineEdit* m_searchEdit;
    QTreeWidget* m_treeWidget;
    QLabel* m_categoryLabel;
    QMap<QString, QTreeWidgetItem*> m_codexItems;  // Codex root items
    QVector<codex::core::TreatiseInfo> m_treatises;
    codex::core::MythicClassifier* m_classifier = nullptr;
};

} // namespace codex::ui
