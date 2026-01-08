#include "ProjectDialog.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QDateTime>

namespace codex::ui {

ProjectDialog::ProjectDialog(Mode mode, QWidget* parent)
    : QDialog(parent)
    , m_mode(mode)
{
    setWindowTitle(mode == NewProject ? "Nouveau Projet" : "Ouvrir un Projet");
    setMinimumSize(500, 400);
    setupUi();
    loadProjects();
}

void ProjectDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Search/Filter
    auto* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel("Rechercher:", this));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Filtrer les projets...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ProjectDialog::onSearchTextChanged);
    searchLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(searchLayout);

    // Project list
    auto* listGroup = new QGroupBox("Projets recents", this);
    auto* listLayout = new QVBoxLayout(listGroup);

    m_projectList = new QListWidget(this);
    m_projectList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_projectList->setStyleSheet(R"(
        QListWidget {
            background-color: #2d2d2d;
            color: #d4d4d4;
            border: 1px solid #3d3d3d;
            border-radius: 4px;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #3d3d3d;
        }
        QListWidget::item:selected {
            background-color: #094771;
        }
        QListWidget::item:hover {
            background-color: #3d3d3d;
        }
    )");
    connect(m_projectList, &QListWidget::itemClicked, this, &ProjectDialog::onProjectItemClicked);
    connect(m_projectList, &QListWidget::itemDoubleClicked, this, &ProjectDialog::onProjectItemDoubleClicked);
    listLayout->addWidget(m_projectList);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setStyleSheet("color: #888; font-size: 11px;");
    m_infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    listLayout->addWidget(m_infoLabel);

    mainLayout->addWidget(listGroup);

    // New project section (only in NewProject mode)
    if (m_mode == NewProject) {
        auto* newGroup = new QGroupBox("Creer un nouveau projet", this);
        auto* newLayout = new QHBoxLayout(newGroup);

        newLayout->addWidget(new QLabel("Nom:", this));
        m_newProjectNameEdit = new QLineEdit(this);
        m_newProjectNameEdit->setPlaceholderText("Mon projet Codex...");
        newLayout->addWidget(m_newProjectNameEdit, 1);

        m_createBtn = new QPushButton("Creer", this);
        m_createBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #0e639c;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
                font-weight: bold;
            }
            QPushButton:hover { background-color: #1177bb; }
            QPushButton:disabled { background-color: #3d3d3d; color: #666; }
        )");
        connect(m_createBtn, &QPushButton::clicked, this, &ProjectDialog::onCreateClicked);
        newLayout->addWidget(m_createBtn);

        mainLayout->addWidget(newGroup);
    }

    // Buttons
    auto* buttonLayout = new QHBoxLayout();

    m_deleteBtn = new QPushButton("Supprimer", this);
    m_deleteBtn->setEnabled(false);
    m_deleteBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #5a1d1d;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
        }
        QPushButton:hover { background-color: #7a2d2d; }
        QPushButton:disabled { background-color: #3d3d3d; color: #666; }
    )");
    connect(m_deleteBtn, &QPushButton::clicked, this, &ProjectDialog::onDeleteClicked);
    buttonLayout->addWidget(m_deleteBtn);

    buttonLayout->addStretch();

    auto* cancelBtn = new QPushButton("Annuler", this);
    cancelBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3d3d3d;
            color: #d4d4d4;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
        }
        QPushButton:hover { background-color: #4d4d4d; }
    )");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);

    m_openBtn = new QPushButton(m_mode == NewProject ? "Ouvrir selectionne" : "Ouvrir", this);
    m_openBtn->setEnabled(false);
    m_openBtn->setDefault(true);
    m_openBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #0e639c;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #1177bb; }
        QPushButton:disabled { background-color: #3d3d3d; color: #666; }
    )");
    connect(m_openBtn, &QPushButton::clicked, this, &ProjectDialog::onOpenClicked);
    buttonLayout->addWidget(m_openBtn);

    mainLayout->addLayout(buttonLayout);

    // Apply dark theme
    setStyleSheet(R"(
        QDialog {
            background-color: #1e1e1e;
            color: #d4d4d4;
        }
        QGroupBox {
            font-weight: bold;
            border: 1px solid #3d3d3d;
            border-radius: 4px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        QLineEdit {
            background-color: #2d2d2d;
            color: #d4d4d4;
            border: 1px solid #3d3d3d;
            border-radius: 4px;
            padding: 6px;
        }
        QLineEdit:focus {
            border-color: #0e639c;
        }
        QLabel {
            color: #d4d4d4;
        }
    )");
}

void ProjectDialog::loadProjects() {
    codex::db::ProjectRepository repo;
    m_projects = repo.findRecent(50);

    m_projectList->clear();

    for (const auto& project : m_projects) {
        QString displayText = QString("%1\n%2 - %3")
            .arg(project.name)
            .arg(project.treatiseCode.isEmpty() ? "Aucun traite" : project.treatiseCode)
            .arg(project.updatedAt.toString("dd/MM/yyyy HH:mm"));

        auto* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, project.id);
        m_projectList->addItem(item);
    }

    m_infoLabel->setText(QString("%1 projet(s) trouve(s)").arg(m_projects.size()));
}

void ProjectDialog::filterProjects(const QString& filter) {
    for (int i = 0; i < m_projectList->count(); ++i) {
        auto* item = m_projectList->item(i);
        bool matches = filter.isEmpty() ||
                       item->text().contains(filter, Qt::CaseInsensitive);
        item->setHidden(!matches);
    }

    // Count visible items
    int visibleCount = 0;
    for (int i = 0; i < m_projectList->count(); ++i) {
        if (!m_projectList->item(i)->isHidden()) {
            ++visibleCount;
        }
    }
    m_infoLabel->setText(QString("%1 projet(s) affiche(s)").arg(visibleCount));
}

void ProjectDialog::onProjectItemClicked(QListWidgetItem* item) {
    int projectId = item->data(Qt::UserRole).toInt();

    // Find the project in our list
    for (const auto& project : m_projects) {
        if (project.id == projectId) {
            m_selectedProject = project;
            break;
        }
    }

    m_openBtn->setEnabled(true);
    m_deleteBtn->setEnabled(true);
}

void ProjectDialog::onProjectItemDoubleClicked(QListWidgetItem* item) {
    onProjectItemClicked(item);
    onOpenClicked();
}

void ProjectDialog::onCreateClicked() {
    if (!m_newProjectNameEdit) return;

    QString name = m_newProjectNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez entrer un nom pour le projet.");
        m_newProjectNameEdit->setFocus();
        return;
    }

    // Create new project in database
    codex::db::ProjectRepository repo;
    codex::db::Project newProject;
    newProject.name = name;

    int newId = repo.create(newProject);
    if (newId > 0) {
        newProject.id = newId;
        m_selectedProject = newProject;

        LOG_INFO(QString("Created new project: %1 (id=%2)").arg(name).arg(newId));

        emit newProjectRequested(name);
        accept();
    } else {
        QMessageBox::critical(this, "Erreur", "Impossible de creer le projet.");
    }
}

void ProjectDialog::onOpenClicked() {
    if (m_selectedProject.id <= 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez selectionner un projet.");
        return;
    }

    emit projectSelected(m_selectedProject);
    accept();
}

void ProjectDialog::onDeleteClicked() {
    if (m_selectedProject.id <= 0) return;

    auto result = QMessageBox::question(this, "Confirmer la suppression",
        QString("Voulez-vous vraiment supprimer le projet \"%1\"?\n\n"
                "Cette action est irreversible et supprimera tous les passages, "
                "images et fichiers audio associes.")
            .arg(m_selectedProject.name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (result == QMessageBox::Yes) {
        codex::db::ProjectRepository repo;
        if (repo.remove(m_selectedProject.id)) {
            LOG_INFO(QString("Deleted project: %1 (id=%2)")
                     .arg(m_selectedProject.name)
                     .arg(m_selectedProject.id));

            m_selectedProject = codex::db::Project();
            m_openBtn->setEnabled(false);
            m_deleteBtn->setEnabled(false);

            loadProjects();
        } else {
            QMessageBox::critical(this, "Erreur", "Impossible de supprimer le projet.");
        }
    }
}

void ProjectDialog::onSearchTextChanged(const QString& text) {
    filterProjects(text);
}

} // namespace codex::ui
