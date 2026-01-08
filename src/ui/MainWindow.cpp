#include "MainWindow.h"
#include "widgets/TextViewerWidget.h"
#include "widgets/ImageViewerWidget.h"
#include "widgets/TreatiseListWidget.h"
#include "widgets/PassagePreviewWidget.h"
#include "widgets/AudioPlayerWidget.h"
#include "widgets/SlideshowWidget.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/ProjectDialog.h"
#include "db/repositories/PassageRepository.h"
#include "db/repositories/ImageRepository.h"
#include "db/repositories/AudioRepository.h"
#include "api/ElevenLabsClient.h"
#include "core/services/TextParser.h"
#include "core/controllers/PipelineController.h"
#include "utils/Logger.h"
#include "utils/Config.h"
#include "utils/SecureStorage.h"
#include "db/Database.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QStatusBar>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QToolBar>
#include <QDateTime>

namespace codex::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Codex Nag Hammadi BD");
    setMinimumSize(1200, 800);

    // Create text parser
    m_textParser = new codex::core::TextParser();

    // Create pipeline controller
    m_pipelineController = new codex::core::PipelineController(this);

    // Create ElevenLabs client
    m_elevenLabsClient = new codex::api::ElevenLabsClient(this);
    m_elevenLabsClient->setApiKey(
        codex::utils::SecureStorage::instance().getApiKey(
            codex::utils::SecureStorage::SERVICE_ELEVENLABS));

    setupUi();
    setupMenus();
    setupConnections();
    applyDarkTheme();

    // Initialize database
    codex::db::Database::instance().initialize();

    // Setup auto-save timer (every 2 minutes)
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(120000);  // 2 minutes
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::onAutoSave);

    // Load last used codex file if available
    QString lastCodexPath = codex::utils::Config::instance().codexFilePath();
    if (!lastCodexPath.isEmpty() && QFile::exists(lastCodexPath)) {
        loadCodexAndRefreshUI(lastCodexPath);
    }

    // Update window title
    updateWindowTitle();

    // Show recent projects dialog on startup (deferred)
    QTimer::singleShot(500, this, &MainWindow::showRecentProjectsOnStartup);

    LOG_INFO("MainWindow initialized");
}

MainWindow::~MainWindow() {
    delete m_textParser;
}

void MainWindow::setupUi() {
    // Main horizontal splitter
    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Treatise list (left panel)
    m_treatiseList = new TreatiseListWidget(this);
    m_treatiseList->setMinimumWidth(300);
    m_treatiseList->setMaximumWidth(400);
    mainSplitter->addWidget(m_treatiseList);

    // Right splitter for text/preview and image
    auto* rightSplitter = new QSplitter(Qt::Horizontal, this);

    // Center vertical splitter for text viewer + passage preview
    auto* centerSplitter = new QSplitter(Qt::Vertical, this);

    // Text viewer (top center)
    m_textViewer = new TextViewerWidget(this);
    centerSplitter->addWidget(m_textViewer);

    // Passage preview (bottom center)
    m_passagePreview = new PassagePreviewWidget(this);
    centerSplitter->addWidget(m_passagePreview);

    // Set center splitter sizes (text takes most space)
    centerSplitter->setSizes({500, 150});

    rightSplitter->addWidget(centerSplitter);

    // Right panel: Image viewer + Audio player
    auto* rightPanelSplitter = new QSplitter(Qt::Vertical, this);

    // Image viewer
    m_imageViewer = new ImageViewerWidget(this);
    rightPanelSplitter->addWidget(m_imageViewer);

    // Audio player
    m_audioPlayer = new AudioPlayerWidget(this);
    rightPanelSplitter->addWidget(m_audioPlayer);

    // Set right panel sizes (image takes most space)
    rightPanelSplitter->setSizes({400, 100});

    rightSplitter->addWidget(rightPanelSplitter);

    // Set right splitter sizes (50% text area, 50% image/audio)
    rightSplitter->setSizes({500, 500});

    mainSplitter->addWidget(rightSplitter);

    // Set main splitter sizes (300px list, rest for content)
    mainSplitter->setSizes({300, 900});

    setCentralWidget(mainSplitter);

    // Toolbar
    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    auto* openAction = toolbar->addAction("Ouvrir");
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    toolbar->addSeparator();

    auto* generateAction = toolbar->addAction("Générer Image");
    connect(generateAction, &QAction::triggered, this, &MainWindow::onGenerateImage);

    toolbar->addSeparator();

    auto* slideshowAction = toolbar->addAction("Diaporama");
    connect(slideshowAction, &QAction::triggered, this, &MainWindow::onStartSlideshow);

    // Status bar
    statusBar()->showMessage("Prêt");
}

void MainWindow::setupMenus() {
    // File menu
    auto* fileMenu = menuBar()->addMenu("&Fichier");

    auto* newProjectAction = fileMenu->addAction("&Nouveau Projet...");
    newProjectAction->setShortcut(QKeySequence::New);
    connect(newProjectAction, &QAction::triggered, this, &MainWindow::onNewProject);

    auto* openProjectAction = fileMenu->addAction("&Ouvrir Projet...");
    openProjectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::onOpenProject);

    fileMenu->addSeparator();

    auto* openAction = fileMenu->addAction("Ouvrir &Codex...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    auto* saveAction = fileMenu->addAction("&Sauvegarder Projet");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveProject);

    fileMenu->addSeparator();

    auto* slideshowMenuAction = fileMenu->addAction("&Diaporama...");
    slideshowMenuAction->setShortcut(QKeySequence(Qt::Key_F5));
    connect(slideshowMenuAction, &QAction::triggered, this, &MainWindow::onStartSlideshow);

    fileMenu->addSeparator();

    auto* quitAction = fileMenu->addAction("&Quitter");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);

    // Edit menu
    auto* editMenu = menuBar()->addMenu("&Édition");

    auto* settingsAction = editMenu->addAction("&Paramètres...");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onShowSettings);

    // Help menu
    auto* helpMenu = menuBar()->addMenu("&Aide");

    auto* aboutAction = helpMenu->addAction("À &propos...");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onShowAbout);
}

void MainWindow::setupConnections() {
    connect(m_textViewer, &TextViewerWidget::passageSelected,
            this, &MainWindow::onPassageSelected);

    connect(m_treatiseList, &TreatiseListWidget::treatiseSelected,
            this, &MainWindow::onTreatiseSelected);

    connect(m_treatiseList, &TreatiseListWidget::treatiseDoubleClicked,
            this, &MainWindow::onTreatiseDoubleClicked);

    connect(m_passagePreview, &PassagePreviewWidget::generateImageRequested,
            this, &MainWindow::onGenerateImageFromPreview);

    connect(m_passagePreview, &PassagePreviewWidget::generateAudioRequested,
            this, &MainWindow::onGenerateAudioFromPreview);

    // Pipeline controller signals
    connect(m_pipelineController, &codex::core::PipelineController::stateChanged,
            this, &MainWindow::onPipelineStateChanged);
    connect(m_pipelineController, &codex::core::PipelineController::progressUpdated,
            this, &MainWindow::onPipelineProgress);
    connect(m_pipelineController, &codex::core::PipelineController::generationCompleted,
            this, &MainWindow::onPipelineCompleted);
    connect(m_pipelineController, &codex::core::PipelineController::generationFailed,
            this, &MainWindow::onPipelineFailed);

    // Image viewer save signal
    connect(m_imageViewer, &ImageViewerWidget::imageSaveRequested,
            this, &MainWindow::onSaveImage);

    // ElevenLabs signals
    connect(m_elevenLabsClient, &codex::api::ElevenLabsClient::speechGenerated,
            this, &MainWindow::onAudioGenerated);
    connect(m_elevenLabsClient, &codex::api::ElevenLabsClient::errorOccurred,
            this, &MainWindow::onAudioError);
}

void MainWindow::applyDarkTheme() {
    QString style = R"(
        QMainWindow, QWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
        }
        QMenuBar {
            background-color: #2d2d2d;
            color: #d4d4d4;
        }
        QMenuBar::item:selected {
            background-color: #3d3d3d;
        }
        QMenu {
            background-color: #2d2d2d;
            color: #d4d4d4;
            border: 1px solid #3d3d3d;
        }
        QMenu::item:selected {
            background-color: #094771;
        }
        QToolBar {
            background-color: #2d2d2d;
            border: none;
            spacing: 5px;
            padding: 5px;
        }
        QToolButton {
            background-color: #3d3d3d;
            color: #d4d4d4;
            border: none;
            padding: 5px 10px;
            border-radius: 3px;
        }
        QToolButton:hover {
            background-color: #094771;
        }
        QStatusBar {
            background-color: #007acc;
            color: white;
        }
        QSplitter::handle {
            background-color: #3d3d3d;
        }
        QTextEdit, QPlainTextEdit {
            background-color: #1e1e1e;
            color: #d4d4d4;
            border: 1px solid #3d3d3d;
            selection-background-color: #264f78;
        }
        QScrollBar:vertical {
            background-color: #1e1e1e;
            width: 12px;
        }
        QScrollBar::handle:vertical {
            background-color: #5a5a5a;
            border-radius: 6px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #7a7a7a;
        }
    )";
    setStyleSheet(style);
}

void MainWindow::onOpenFile() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Ouvrir fichier Codex",
        QString(),
        "Markdown (*.md);;Tous les fichiers (*.*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    loadCodexAndRefreshUI(filePath);
    codex::utils::Config::instance().setCodexFilePath(filePath);
}

void MainWindow::loadCodexAndRefreshUI(const QString& filePath) {
    if (!m_textParser->loadCodexFile(filePath)) {
        QMessageBox::warning(this, "Erreur",
            QString("Impossible de charger le fichier Codex:\n%1").arg(filePath));
        return;
    }

    // Update treatise list
    QVector<codex::core::TreatiseInfo> treatises = m_textParser->parseTableOfContents();
    m_treatiseList->loadTreatises(treatises);

    // Clear text viewer
    m_textViewer->setText("Selectionnez un traite dans la liste a gauche.");

    statusBar()->showMessage(QString("Codex charge: %1 traites, %2 pages")
                             .arg(treatises.size())
                             .arg(m_textParser->pageCount()));

    LOG_INFO(QString("Loaded Codex: %1").arg(filePath));
}

void MainWindow::onSaveProject() {
    if (m_currentProject.id <= 0) {
        // No project loaded, create new one
        onNewProject();
        return;
    }

    // Update project with current state
    m_currentProject.treatiseCode = m_currentTreatiseCode;
    m_currentProject.category = m_currentCategory;

    codex::db::ProjectRepository repo;
    if (repo.update(m_currentProject)) {
        setProjectModified(false);
        statusBar()->showMessage(QString("Projet sauvegarde: %1").arg(m_currentProject.name));
        LOG_INFO(QString("Saved project: %1").arg(m_currentProject.name));
    } else {
        QMessageBox::warning(this, "Erreur", "Impossible de sauvegarder le projet.");
        LOG_ERROR(QString("Failed to save project: %1").arg(m_currentProject.name));
    }
}

void MainWindow::onShowSettings() {
    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::onShowAbout() {
    QMessageBox::about(this, "À propos",
        "<h2>Codex Nag Hammadi BD</h2>"
        "<p>Version 1.0</p>"
        "<p>Application de génération de BD photoréaliste<br>"
        "à partir des textes gnostiques de Nag Hammadi.</p>"
        "<p>Style visuel: Denis Villeneuve</p>"
    );
}

void MainWindow::onPassageSelected(const QString& text, int start, int end) {
    m_selectedPassage = text;
    m_passagePreview->setPassage(text, start, end);
    statusBar()->showMessage(QString("Passage selectionne: %1 caracteres").arg(text.length()));
}

void MainWindow::onGenerateImage() {
    if (m_selectedPassage.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez d'abord selectionner un passage de texte.");
        return;
    }

    statusBar()->showMessage("Generation en cours...");
    m_imageViewer->showLoading();

    // TODO: Implement full pipeline
    // For now, just show a placeholder message
    QMessageBox::information(this, "Generation",
        QString("Pipeline de generation a implementer.\n\nPassage selectionne:\n%1")
            .arg(m_selectedPassage.left(200) + "..."));
}

void MainWindow::onTreatiseSelected(const QString& code, const QString& title, const QString& category) {
    // Load treatise on single click
    m_currentTreatiseCode = code;
    m_currentCategory = category;
    setProjectModified(true);

    // Extract and display treatise content
    codex::core::ParsedTreatise treatise = m_textParser->extractTreatise(code);

    if (treatise.fullText.isEmpty()) {
        statusBar()->showMessage(QString("Erreur: impossible d'extraire %1").arg(code));
        return;
    }

    // Display in text viewer with verse numbering (Page:Paragraph)
    QString header = QString("══════════════════════════════════════\n"
                             "  %1 - %2\n"
                             "  Categorie: %3\n"
                             "  Pages manuscrit: %4+\n"
                             "══════════════════════════════════════\n\n")
                     .arg(code, title, category)
                     .arg(treatise.startPage);
    m_textViewer->setTextWithVerses(header + treatise.fullText, treatise.startPage);

    statusBar()->showMessage(QString("Traite: %1 - %2 | Categorie: %3 | Page %4 | %5 caracteres")
                             .arg(code, title, category)
                             .arg(treatise.startPage)
                             .arg(treatise.fullText.length()));

    LOG_INFO(QString("Displayed treatise: %1, category: %2").arg(code, category));
}

void MainWindow::onTreatiseDoubleClicked(const QString& code, const QString& title, const QString& category) {
    // Double-click does the same as single click (for compatibility)
    onTreatiseSelected(code, title, category);
}

void MainWindow::onGenerateImageFromPreview(const QString& passage) {
    m_selectedPassage = passage;

    // Check if pipeline is already running
    if (m_pipelineController->isRunning()) {
        QMessageBox::warning(this, "Generation en cours",
            "Une generation est deja en cours. Veuillez patienter.");
        return;
    }

    statusBar()->showMessage(QString("Generation... Categorie: %1").arg(m_currentCategory));
    m_imageViewer->showLoading();

    // Start the generation pipeline with category
    m_pipelineController->startGeneration(passage, m_currentTreatiseCode, m_currentCategory);

    LOG_INFO(QString("Image generation started for passage: %1 chars, category: %2")
             .arg(passage.length()).arg(m_currentCategory));
}

void MainWindow::onGenerateAudioFromPreview(const QString& passage) {
    if (passage.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Aucun passage selectionne pour la narration.");
        return;
    }

    // Check if API key is configured
    QString apiKey = codex::utils::SecureStorage::instance().getApiKey(
        codex::utils::SecureStorage::SERVICE_ELEVENLABS);
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, "Configuration requise",
            "Veuillez configurer votre cle API ElevenLabs dans les parametres.");
        onShowSettings();
        return;
    }

    statusBar()->showMessage("Generation audio en cours...");
    m_audioPlayer->stop();

    // Get voice settings from config
    codex::api::VoiceSettings voiceSettings;
    voiceSettings.voiceId = codex::utils::Config::instance().elevenLabsVoiceId();
    if (voiceSettings.voiceId.isEmpty()) {
        // Default voice: "Daniel" (deep, narrator-like)
        voiceSettings.voiceId = "onwK4e9ZLuTAKqWW03F9";
    }
    voiceSettings.stability = 0.5;
    voiceSettings.similarityBoost = 0.75;
    voiceSettings.speed = 0.85;

    // Generate speech using ElevenLabs
    m_elevenLabsClient->generateSpeech(passage, voiceSettings);

    LOG_INFO(QString("Audio generation started for passage: %1 chars, voice: %2")
             .arg(passage.length()).arg(voiceSettings.voiceId));
}

void MainWindow::onAudioGenerated(const QByteArray& audioData, int durationMs) {
    if (audioData.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Les donnees audio recues sont vides.");
        return;
    }

    m_audioPlayer->loadFromData(audioData);
    m_audioPlayer->play();

    int seconds = durationMs / 1000;
    statusBar()->showMessage(QString("Audio genere avec succes! Duree: %1:%2")
                             .arg(seconds / 60, 2, 10, QChar('0'))
                             .arg(seconds % 60, 2, 10, QChar('0')));
    LOG_INFO(QString("Audio generated successfully, size: %1 bytes, duration: %2 ms")
             .arg(audioData.size()).arg(durationMs));
}

void MainWindow::onAudioError(const QString& error) {
    statusBar()->showMessage(QString("Erreur audio: %1").arg(error));
    QMessageBox::critical(this, "Erreur de generation audio", error);
    LOG_ERROR(QString("Audio generation failed: %1").arg(error));
}

void MainWindow::onPipelineStateChanged(codex::core::PipelineState state, const QString& message) {
    Q_UNUSED(state)
    statusBar()->showMessage(message);
}

void MainWindow::onPipelineProgress(int percent, const QString& step) {
    statusBar()->showMessage(QString("%1 (%2%)").arg(step).arg(percent));
}

void MainWindow::onPipelineCompleted(const QPixmap& image, const QString& prompt) {
    m_imageViewer->setImage(image);
    statusBar()->showMessage("Image generee avec succes!");

    LOG_INFO(QString("Pipeline completed, image size: %1x%2")
             .arg(image.width()).arg(image.height()));

    // Optional: show the prompt used
    Q_UNUSED(prompt)
}

void MainWindow::onPipelineFailed(const QString& error) {
    m_imageViewer->showPlaceholder();
    statusBar()->showMessage(QString("Erreur: %1").arg(error));

    QMessageBox::critical(this, "Erreur de generation", error);

    LOG_ERROR(QString("Pipeline failed: %1").arg(error));
}

void MainWindow::onSaveImage() {
    if (!m_imageViewer->hasImage()) {
        QMessageBox::warning(this, "Erreur", "Aucune image a sauvegarder.");
        return;
    }

    QString defaultName = QString("codex_image_%1.png")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Sauvegarder l'image",
        codex::utils::Config::instance().outputImagesPath() + "/" + defaultName,
        "Images PNG (*.png);;Images JPEG (*.jpg *.jpeg);;Tous les fichiers (*.*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    QPixmap image = m_imageViewer->currentImage();
    if (image.save(filePath)) {
        statusBar()->showMessage(QString("Image sauvegardee: %1").arg(filePath));
        LOG_INFO(QString("Image saved to: %1").arg(filePath));
    } else {
        QMessageBox::warning(this, "Erreur", "Impossible de sauvegarder l'image.");
        LOG_ERROR(QString("Failed to save image to: %1").arg(filePath));
    }
}

void MainWindow::onStartSlideshow() {
    // Check if we have any image to show
    if (!m_imageViewer->hasImage()) {
        QMessageBox::information(this, "Diaporama",
            "Generez au moins une image avant de lancer le diaporama.\n\n"
            "Le diaporama affichera les images generees avec la narration audio synchronisee.");
        return;
    }

    // Create slideshow widget if needed
    if (!m_slideshowWidget) {
        m_slideshowWidget = new SlideshowWidget(nullptr);
        m_slideshowWidget->setAttribute(Qt::WA_DeleteOnClose, false);
    }

    // Clear previous slides
    m_slideshowWidget->clear();

    // Add current image as a slide (for demo purposes)
    // In full implementation, this would load from the database
    SlideData slide;
    slide.image = m_imageViewer->currentImage();
    slide.passageText = m_selectedPassage;
    slide.audioDurationMs = 5000;  // 5 seconds default

    m_slideshowWidget->addSlide(slide);

    // Show slideshow
    m_slideshowWidget->showFullScreen();
    m_slideshowWidget->start();

    statusBar()->showMessage("Diaporama lance (Echap pour quitter)");
    LOG_INFO("Slideshow started");
}

void MainWindow::onNewProject() {
    // Check if current project needs saving
    if (m_projectModified && m_currentProject.id > 0) {
        auto result = QMessageBox::question(this, "Sauvegarder?",
            "Le projet actuel a ete modifie. Voulez-vous le sauvegarder?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (result == QMessageBox::Cancel) {
            return;
        }
        if (result == QMessageBox::Yes) {
            onSaveProject();
        }
    }

    ProjectDialog dialog(ProjectDialog::NewProject, this);
    if (dialog.exec() == QDialog::Accepted) {
        loadProject(dialog.selectedProject());
    }
}

void MainWindow::onOpenProject() {
    // Check if current project needs saving
    if (m_projectModified && m_currentProject.id > 0) {
        auto result = QMessageBox::question(this, "Sauvegarder?",
            "Le projet actuel a ete modifie. Voulez-vous le sauvegarder?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (result == QMessageBox::Cancel) {
            return;
        }
        if (result == QMessageBox::Yes) {
            onSaveProject();
        }
    }

    ProjectDialog dialog(ProjectDialog::OpenProject, this);
    if (dialog.exec() == QDialog::Accepted) {
        loadProject(dialog.selectedProject());
    }
}

void MainWindow::onAutoSave() {
    if (m_projectModified && m_currentProject.id > 0) {
        onSaveProject();
        LOG_INFO("Auto-saved project");
    }
}

void MainWindow::updateWindowTitle() {
    QString title = "Codex Nag Hammadi BD";

    if (m_currentProject.id > 0) {
        title = QString("%1 - %2").arg(m_currentProject.name, title);
        if (m_projectModified) {
            title = "* " + title;
        }
    }

    setWindowTitle(title);
}

void MainWindow::setProjectModified(bool modified) {
    if (m_projectModified != modified) {
        m_projectModified = modified;
        updateWindowTitle();

        // Start/stop auto-save timer
        if (modified && m_currentProject.id > 0) {
            m_autoSaveTimer->start();
        } else {
            m_autoSaveTimer->stop();
        }
    }
}

void MainWindow::loadProject(const codex::db::Project& project) {
    m_currentProject = project;
    m_projectModified = false;

    // Update UI with project info
    m_currentTreatiseCode = project.treatiseCode;
    m_currentCategory = project.category;

    // Load treatise if set
    if (!project.treatiseCode.isEmpty()) {
        codex::core::ParsedTreatise treatise = m_textParser->extractTreatise(project.treatiseCode);
        if (!treatise.fullText.isEmpty()) {
            m_textViewer->setTextWithVerses(treatise.fullText, treatise.startPage);
        }
    }

    // Load passages for this project
    codex::db::PassageRepository passageRepo;
    QVector<codex::db::Passage> passages = passageRepo.findByProjectId(project.id);

    updateWindowTitle();
    statusBar()->showMessage(QString("Projet charge: %1 (%2 passages)")
                             .arg(project.name)
                             .arg(passages.size()));

    LOG_INFO(QString("Loaded project: %1 (id=%2)").arg(project.name).arg(project.id));
}

void MainWindow::showRecentProjectsOnStartup() {
    // Check if there are any recent projects
    codex::db::ProjectRepository repo;
    QVector<codex::db::Project> recentProjects = repo.findRecent(5);

    if (!recentProjects.isEmpty()) {
        // Show project dialog
        ProjectDialog dialog(ProjectDialog::OpenProject, this);
        if (dialog.exec() == QDialog::Accepted) {
            loadProject(dialog.selectedProject());
        }
    }
}

} // namespace codex::ui
