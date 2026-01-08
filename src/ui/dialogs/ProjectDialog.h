#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "db/repositories/ProjectRepository.h"

namespace codex::ui {

class ProjectDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode {
        OpenProject,
        NewProject
    };

    explicit ProjectDialog(Mode mode, QWidget* parent = nullptr);

    // Get selected/created project
    codex::db::Project selectedProject() const { return m_selectedProject; }
    bool hasSelection() const { return m_selectedProject.id > 0 || !m_selectedProject.name.isEmpty(); }

signals:
    void projectSelected(const codex::db::Project& project);
    void newProjectRequested(const QString& name);

private slots:
    void onProjectItemClicked(QListWidgetItem* item);
    void onProjectItemDoubleClicked(QListWidgetItem* item);
    void onCreateClicked();
    void onOpenClicked();
    void onDeleteClicked();
    void onSearchTextChanged(const QString& text);

private:
    void setupUi();
    void loadProjects();
    void filterProjects(const QString& filter);

    Mode m_mode;
    codex::db::Project m_selectedProject;
    QVector<codex::db::Project> m_projects;

    QLineEdit* m_searchEdit = nullptr;
    QLineEdit* m_newProjectNameEdit = nullptr;
    QListWidget* m_projectList = nullptr;
    QPushButton* m_createBtn = nullptr;
    QPushButton* m_openBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QLabel* m_infoLabel = nullptr;
};

} // namespace codex::ui
