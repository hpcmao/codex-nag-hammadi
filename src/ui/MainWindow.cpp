#include "MainWindow.h"
#include "widgets/TextViewerWidget.h"
#include "widgets/ImageViewerWidget.h"
#include "widgets/TreatiseListWidget.h"
#include "widgets/PassagePreviewWidget.h"
#include "widgets/AudioPlayerWidget.h"
#include "widgets/SlideshowWidget.h"
#include "widgets/InfoDockWidget.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/ProjectDialog.h"
#include "dialogs/SlideshowDialog.h"
#include "db/repositories/PassageRepository.h"
#include "db/repositories/ImageRepository.h"
#include "db/repositories/AudioRepository.h"
#include "api/ElevenLabsClient.h"
#include "api/EdgeTTSClient.h"
#include "api/VeoClient.h"
#include "core/services/TextParser.h"
#include "core/controllers/PipelineController.h"
#include "utils/Logger.h"
#include "utils/Config.h"
#include "utils/SecureStorage.h"
#include "utils/MessageBox.h"
#include "utils/ThemeManager.h"
#include "db/Database.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QStatusBar>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QToolBar>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>

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

    // Create Edge TTS client (free alternative)
    m_edgeTTSClient = new codex::api::EdgeTTSClient(this);

    // Create Veo client for video generation
    m_veoClient = new codex::api::VeoClient(this);

    // Configure Google AI provider (AI Studio or Vertex AI) for Veo
    // Les deux utilisent la même clé API, seul l'endpoint change
    auto& config = codex::utils::Config::instance();
    QString googleProvider = config.googleAiProvider();
    if (googleProvider == "vertex") {
        m_veoClient->setProvider(codex::api::GoogleAIProvider::VertexAI);
        LOG_INFO("Veo: Using Vertex AI endpoint");
    } else {
        m_veoClient->setProvider(codex::api::GoogleAIProvider::AIStudio);
        LOG_INFO("Veo: Using AI Studio endpoint");
    }
    m_veoClient->setApiKey(
        codex::utils::SecureStorage::instance().getApiKey(
            codex::utils::SecureStorage::SERVICE_IMAGEN));

    setupUi();
    setupMenus();
    setupConnections();

    // Apply theme from settings
    codex::utils::ThemeManager::instance().apply();

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

    // Right splitter for text and image/audio
    auto* rightSplitter = new QSplitter(Qt::Horizontal, this);

    // Center vertical splitter for text viewer + tabs
    auto* centerSplitter = new QSplitter(Qt::Vertical, this);

    // Text viewer (top center)
    m_textViewer = new TextViewerWidget(this);
    centerSplitter->addWidget(m_textViewer);

    // Tab widget for passage and prompt (middle center)
    m_centerTabWidget = new QTabWidget(this);
    m_centerTabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            border: 1px solid #3d3d3d;
            background-color: #252525;
        }
        QTabBar::tab {
            background-color: #2d2d2d;
            color: #888;
            padding: 8px 16px;
            border: 1px solid #3d3d3d;
            border-bottom: none;
        }
        QTabBar::tab:selected {
            background-color: #094771;
            color: #fff;
        }
        QTabBar::tab:hover:!selected {
            background-color: #3d3d3d;
        }
    )");

    // Passage preview tab
    m_passagePreview = new PassagePreviewWidget(this);
    m_centerTabWidget->addTab(m_passagePreview, "Passage");

    // Prompt tab
    m_promptEdit = new QTextEdit(this);
    m_promptEdit->setReadOnly(true);
    m_promptEdit->setPlaceholderText("Le prompt genere apparaitra ici...");
    m_promptEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: #252525;
            color: #d4d4d4;
            border: none;
            padding: 10px;
            font-family: Consolas, monospace;
            font-size: 11px;
        }
    )");
    m_centerTabWidget->addTab(m_promptEdit, "Prompt");

    centerSplitter->addWidget(m_centerTabWidget);

    // Set center splitter sizes (text takes most space, tabs smaller)
    centerSplitter->setSizes({450, 200});

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

    // Info dock widget (API pricing/quotas)
    m_infoDock = new InfoDockWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, m_infoDock);
    m_infoDock->hide();  // Hidden by default

    // Toolbar
    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    auto* openAction = toolbar->addAction("Ouvrir");
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    toolbar->addSeparator();

    auto* slideshowAction = toolbar->addAction("Diaporama");
    connect(slideshowAction, &QAction::triggered, this, &MainWindow::onStartSlideshow);

    // Settings toolbar
    auto* settingsToolbar = addToolBar("Reglages");
    settingsToolbar->setMovable(false);

    // Voice selection (Edge TTS Neural voices)
    settingsToolbar->addWidget(new QLabel(" Voix: ", this));
    m_voiceCombo = new QComboBox(this);
    m_voiceCombo->addItem("Henri (FR homme)", "fr-FR-HenriNeural");
    m_voiceCombo->addItem("Denise (FR femme)", "fr-FR-DeniseNeural");
    m_voiceCombo->addItem("Eloise (FR femme)", "fr-FR-EloiseNeural");
    m_voiceCombo->addItem("Remy (FR multi)", "fr-FR-RemyMultilingualNeural");
    m_voiceCombo->addItem("Antoine (CA homme)", "fr-CA-AntoineNeural");
    m_voiceCombo->addItem("Sylvie (CA femme)", "fr-CA-SylvieNeural");
    m_voiceCombo->addItem("Guy (US homme)", "en-US-GuyNeural");
    m_voiceCombo->addItem("Jenny (US femme)", "en-US-JennyNeural");
    m_voiceCombo->setMinimumWidth(150);
    // Load saved voice
    QString savedVoice = codex::utils::Config::instance().edgeTtsVoice();
    int voiceIndex = m_voiceCombo->findData(savedVoice);
    if (voiceIndex >= 0) m_voiceCombo->setCurrentIndex(voiceIndex);
    connect(m_voiceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onVoiceChanged);
    settingsToolbar->addWidget(m_voiceCombo);

    // Test voice button
    auto* testVoiceBtn = new QPushButton("Test", this);
    testVoiceBtn->setMaximumWidth(50);
    connect(testVoiceBtn, &QPushButton::clicked, this, &MainWindow::onTestVoice);
    settingsToolbar->addWidget(testVoiceBtn);

    settingsToolbar->addSeparator();

    // Images output folder
    settingsToolbar->addWidget(new QLabel(" Images: ", this));
    m_outputFolderEdit = new QLineEdit(this);
    m_outputFolderEdit->setReadOnly(true);
    m_outputFolderEdit->setMinimumWidth(150);
    m_outputFolderEdit->setText(codex::utils::Config::instance().outputImagesPath());
    settingsToolbar->addWidget(m_outputFolderEdit);

    auto* browseBtn = new QPushButton("...", this);
    browseBtn->setMaximumWidth(30);
    connect(browseBtn, &QPushButton::clicked, this, &MainWindow::onBrowseOutputFolder);
    settingsToolbar->addWidget(browseBtn);

    auto* openImagesBtn = new QPushButton("Ouvrir", this);
    openImagesBtn->setMaximumWidth(50);
    connect(openImagesBtn, &QPushButton::clicked, this, &MainWindow::onOpenImagesFolder);
    settingsToolbar->addWidget(openImagesBtn);

    // Videos output folder
    settingsToolbar->addWidget(new QLabel(" Videos: ", this));
    m_videoFolderEdit = new QLineEdit(this);
    m_videoFolderEdit->setReadOnly(true);
    m_videoFolderEdit->setMinimumWidth(150);
    m_videoFolderEdit->setText(codex::utils::Config::instance().outputVideosPath());
    settingsToolbar->addWidget(m_videoFolderEdit);

    auto* browseVideoBtn = new QPushButton("...", this);
    browseVideoBtn->setMaximumWidth(30);
    connect(browseVideoBtn, &QPushButton::clicked, this, &MainWindow::onBrowseVideoFolder);
    settingsToolbar->addWidget(browseVideoBtn);

    auto* openVideosBtn = new QPushButton("Ouvrir", this);
    openVideosBtn->setMaximumWidth(50);
    connect(openVideosBtn, &QPushButton::clicked, this, &MainWindow::onOpenVideosFolder);
    settingsToolbar->addWidget(openVideosBtn);

    // Progress bar in status bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFixedWidth(200);
    m_progressBar->setFixedHeight(18);
    m_progressBar->hide();  // Hidden by default
    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            border: 1px solid #3d3d3d;
            border-radius: 3px;
            background-color: #2d2d2d;
            text-align: center;
            color: #fff;
        }
        QProgressBar::chunk {
            background-color: #094771;
            border-radius: 2px;
        }
    )");
    statusBar()->addPermanentWidget(m_progressBar);

    // Status bar
    statusBar()->showMessage("Pret");
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

    // View menu
    auto* viewMenu = menuBar()->addMenu("&Affichage");

    auto* infoAction = viewMenu->addAction("&Informations API...");
    infoAction->setShortcut(QKeySequence(Qt::Key_F1));
    infoAction->setCheckable(true);
    connect(infoAction, &QAction::triggered, this, [this](bool checked) {
        m_infoDock->setVisible(checked);
    });
    connect(m_infoDock, &QDockWidget::visibilityChanged, infoAction, &QAction::setChecked);

    viewMenu->addSeparator();

    auto* openImagesFolderAction = viewMenu->addAction("Ouvrir dossier &Images");
    connect(openImagesFolderAction, &QAction::triggered, this, &MainWindow::onOpenImagesFolder);

    auto* openVideosFolderAction = viewMenu->addAction("Ouvrir dossier &Videos");
    connect(openVideosFolderAction, &QAction::triggered, this, &MainWindow::onOpenVideosFolder);

    auto* openAudioFolderAction = viewMenu->addAction("Ouvrir dossier &Audio");
    connect(openAudioFolderAction, &QAction::triggered, this, &MainWindow::onOpenAudioFolder);

    // Help menu
    auto* helpMenu = menuBar()->addMenu("&Aide");

    auto* aboutAction = helpMenu->addAction("À &propos...");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onShowAbout);

    // Generation menu (after Help)
    auto* genMenu = menuBar()->addMenu("&Génération");

    auto* genImageAction = genMenu->addAction("Générer &Image");
    genImageAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    connect(genImageAction, &QAction::triggered, this, [this]() {
        if (!m_selectedPassage.isEmpty()) {
            onGenerateImageFromPreview(m_selectedPassage);
        }
    });

    auto* genAudioAction = genMenu->addAction("Générer &Audio");
    genAudioAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
    connect(genAudioAction, &QAction::triggered, this, [this]() {
        if (!m_selectedPassage.isEmpty()) {
            onGenerateAudioFromPreview(m_selectedPassage);
        }
    });

    auto* genVideoAction = genMenu->addAction("Générer &Vidéo");
    genVideoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_V));
    connect(genVideoAction, &QAction::triggered, this, [this]() {
        if (!m_selectedPassage.isEmpty()) {
            onGenerateVideoFromPreview(m_selectedPassage);
        }
    });

    genMenu->addSeparator();

    auto* genAllAction = genMenu->addAction("Générer &Tout + Diaporama");
    genAllAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
    connect(genAllAction, &QAction::triggered, this, &MainWindow::onGenerateAllAndSlideshow);

    // Planche menu (separate top-level menu)
    auto* plateMenu = menuBar()->addMenu("&Planche");

    auto* plate2x2 = plateMenu->addAction("2x2 (4 images)");
    connect(plate2x2, &QAction::triggered, this, [this]() {
        onGeneratePlateFromPreview(m_selectedPassage, 2, 2);
    });

    auto* plate3x2 = plateMenu->addAction("3x2 (6 images)");
    connect(plate3x2, &QAction::triggered, this, [this]() {
        onGeneratePlateFromPreview(m_selectedPassage, 3, 2);
    });

    auto* plate3x3 = plateMenu->addAction("3x3 (9 images)");
    connect(plate3x3, &QAction::triggered, this, [this]() {
        onGeneratePlateFromPreview(m_selectedPassage, 3, 3);
    });

    auto* plate3x4 = plateMenu->addAction("3x4 (12 images)");
    connect(plate3x4, &QAction::triggered, this, [this]() {
        onGeneratePlateFromPreview(m_selectedPassage, 3, 4);
    });

    auto* plate4x4 = plateMenu->addAction("4x4 (16 images)");
    connect(plate4x4, &QAction::triggered, this, [this]() {
        onGeneratePlateFromPreview(m_selectedPassage, 4, 4);
    });

    auto* plate4x5 = plateMenu->addAction("4x5 (20 images)");
    connect(plate4x5, &QAction::triggered, this, [this]() {
        onGeneratePlateFromPreview(m_selectedPassage, 4, 5);
    });

    auto* plate5x6 = plateMenu->addAction("5x6 (30 images)");
    connect(plate5x6, &QAction::triggered, this, [this]() {
        onGeneratePlateFromPreview(m_selectedPassage, 5, 6);
    });
}

void MainWindow::setupConnections() {
    connect(m_textViewer, &TextViewerWidget::passageSelected,
            this, &MainWindow::onPassageSelected);

    connect(m_treatiseList, &TreatiseListWidget::treatiseSelected,
            this, &MainWindow::onTreatiseSelected);

    connect(m_treatiseList, &TreatiseListWidget::treatiseDoubleClicked,
            this, &MainWindow::onTreatiseDoubleClicked);

    // Generation is now handled via the Generation menu

    // Veo (video generation) signals
    connect(m_veoClient, &codex::api::VeoClient::videoGenerated,
            this, &MainWindow::onVideoGenerated);
    connect(m_veoClient, &codex::api::VeoClient::generationProgress,
            this, &MainWindow::onVideoProgress);
    connect(m_veoClient, &codex::api::VeoClient::errorOccurred,
            this, &MainWindow::onVideoError);

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

    // Edge TTS signals
    connect(m_edgeTTSClient, &codex::api::EdgeTTSClient::speechGenerated,
            this, &MainWindow::onEdgeAudioGenerated);
    connect(m_edgeTTSClient, &codex::api::EdgeTTSClient::errorOccurred,
            this, &MainWindow::onEdgeAudioError);
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
        codex::utils::MessageBox::warning(this, "Erreur",
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
        codex::utils::MessageBox::warning(this, "Erreur", "Impossible de sauvegarder le projet.");
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
        codex::utils::MessageBox::warning(this, "Erreur", "Veuillez d'abord selectionner un passage de texte.");
        return;
    }

    statusBar()->showMessage("Generation en cours...");
    m_imageViewer->showLoading();

    // TODO: Implement full pipeline
    // For now, just show a placeholder message
    codex::utils::MessageBox::info(this, "Generation",
        QString("Pipeline de generation a implementer.\n\nPassage selectionne:\n%1")
            .arg(m_selectedPassage.left(200) + "..."));
}

void MainWindow::onTreatiseSelected(const QString& code, const QString& title, const QString& category) {
    // Load treatise on single click
    m_currentTreatiseCode = code;
    m_currentCategory = category;
    setProjectModified(true);

    // Update passage preview with treatise code for favorites
    m_passagePreview->setTreatiseCode(code);

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
        codex::utils::MessageBox::warning(this, "Generation en cours",
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
        codex::utils::MessageBox::warning(this, "Erreur", "Aucun passage selectionne pour la narration.");
        return;
    }

    auto& config = codex::utils::Config::instance();
    QString ttsProvider = config.ttsProvider();

    statusBar()->showMessage("Generation audio en cours...");
    m_audioPlayer->stop();

    if (ttsProvider == "edge") {
        // Use Microsoft Edge Neural TTS (free, high quality)
        codex::api::EdgeVoiceSettings edgeSettings;
        edgeSettings.voiceId = config.edgeTtsVoice();
        edgeSettings.rate = -15;     // Slightly slower for narration (-100 to 100)
        edgeSettings.pitch = 0;      // Normal pitch
        edgeSettings.volume = 100;   // Full volume

        m_edgeTTSClient->generateSpeech(passage, edgeSettings);

        LOG_INFO(QString("Edge TTS generation started for passage: %1 chars, voice: %2")
                 .arg(passage.length()).arg(edgeSettings.voiceId));
    } else {
        // Use ElevenLabs (premium)
        QString apiKey = codex::utils::SecureStorage::instance().getApiKey(
            codex::utils::SecureStorage::SERVICE_ELEVENLABS);
        if (apiKey.isEmpty()) {
            codex::utils::MessageBox::warning(this, "Configuration requise",
                "Veuillez configurer votre cle API ElevenLabs dans les parametres,\n"
                "ou selectionnez Edge TTS (gratuit) comme fournisseur.");
            onShowSettings();
            return;
        }

        codex::api::VoiceSettings voiceSettings;
        voiceSettings.voiceId = config.elevenLabsVoiceId();
        if (voiceSettings.voiceId.isEmpty()) {
            voiceSettings.voiceId = "onwK4e9ZLuTAKqWW03F9";
        }
        voiceSettings.stability = 0.5;
        voiceSettings.similarityBoost = 0.75;
        voiceSettings.speed = 0.85;

        m_elevenLabsClient->generateSpeech(passage, voiceSettings);

        LOG_INFO(QString("ElevenLabs generation started for passage: %1 chars, voice: %2")
                 .arg(passage.length()).arg(voiceSettings.voiceId));
    }
}

void MainWindow::onAudioGenerated(const QByteArray& audioData, int durationMs) {
    if (audioData.isEmpty()) {
        codex::utils::MessageBox::warning(this, "Erreur", "Les donnees audio recues sont vides.");

        if (m_fullGenerating) {
            m_progressBar->setValue(100);
            onFullGenerationCompleted();
        }
        return;
    }

    m_audioPlayer->loadFromData(audioData);

    int seconds = durationMs / 1000;
    statusBar()->showMessage(QString("Audio genere avec succes! Duree: %1:%2")
                             .arg(seconds / 60, 2, 10, QChar('0'))
                             .arg(seconds % 60, 2, 10, QChar('0')));
    LOG_INFO(QString("Audio generated successfully, size: %1 bytes, duration: %2 ms")
             .arg(audioData.size()).arg(durationMs));

    if (m_fullGenerating) {
        m_progressBar->setValue(100);
        m_progressBar->setFormat("Termine!");
        onFullGenerationCompleted();
    } else {
        m_audioPlayer->play();
    }
}

void MainWindow::onAudioError(const QString& error) {
    statusBar()->showMessage(QString("Erreur audio: %1").arg(error));
    codex::utils::MessageBox::critical(this, "Erreur de generation audio", error);
    LOG_ERROR(QString("Audio generation failed: %1").arg(error));

    if (m_fullGenerating) {
        // Still launch slideshow even without audio
        m_progressBar->setValue(100);
        onFullGenerationCompleted();
    }
}

void MainWindow::onEdgeAudioGenerated(const QByteArray& audioData, int durationMs) {
    if (audioData.isEmpty()) {
        statusBar()->showMessage("Erreur: donnees audio vides");
        LOG_ERROR("Edge TTS returned empty audio data");

        if (m_fullGenerating) {
            // Still launch slideshow even without audio
            m_progressBar->setValue(100);
            onFullGenerationCompleted();
        }
        return;
    }

    // Load and play the MP3 audio data
    m_audioPlayer->loadFromData(audioData);

    int seconds = durationMs / 1000;
    statusBar()->showMessage(QString("Audio genere! Duree: %1:%2 | Taille: %3 KB")
                             .arg(seconds / 60, 2, 10, QChar('0'))
                             .arg(seconds % 60, 2, 10, QChar('0'))
                             .arg(audioData.size() / 1024));
    LOG_INFO(QString("Edge TTS audio generated, size: %1 bytes, duration: %2 ms")
             .arg(audioData.size()).arg(durationMs));

    // Check if we're in full generation mode
    if (m_fullGenerating) {
        m_progressBar->setValue(100);
        m_progressBar->setFormat("Termine!");
        onFullGenerationCompleted();
    } else {
        m_audioPlayer->play();
    }
}

void MainWindow::onEdgeAudioError(const QString& error) {
    statusBar()->showMessage(QString("Erreur Edge TTS: %1").arg(error));
    codex::utils::MessageBox::critical(this, "Erreur Edge TTS", error);
    LOG_ERROR(QString("Edge TTS generation failed: %1").arg(error));

    if (m_fullGenerating) {
        // Still launch slideshow even without audio
        m_progressBar->setValue(100);
        onFullGenerationCompleted();
    }
}

void MainWindow::onGenerateVideoFromPreview(const QString& passage) {
    if (passage.isEmpty()) {
        codex::utils::MessageBox::warning(this, "Erreur", "Aucun passage selectionne pour la video.");
        return;
    }

    // Check if API key is configured
    QString apiKey = codex::utils::SecureStorage::instance().getApiKey(
        codex::utils::SecureStorage::SERVICE_IMAGEN);
    if (apiKey.isEmpty()) {
        codex::utils::MessageBox::warning(this, "Configuration requise",
            "Veuillez configurer votre cle Google AI API dans les parametres.");
        onShowSettings();
        return;
    }

    statusBar()->showMessage("Generation video en cours (cela peut prendre plusieurs minutes)...");

    // Create video prompt from passage
    QString videoPrompt = QString(
        "Cinematic slow motion shot, mystical ancient scene. "
        "Divine light rays pierce through clouds. "
        "Style of Denis Villeneuve, ethereal atmosphere. "
        "Gnostic symbolism: %1"
    ).arg(passage.left(200));

    // Generate video
    codex::api::VideoGenerationParams params;
    params.prompt = videoPrompt;
    params.aspectRatio = "16:9";
    params.durationSeconds = 5;

    m_veoClient->generateVideo(params);

    LOG_INFO(QString("Video generation started for passage: %1 chars")
             .arg(passage.length()));
}

void MainWindow::onVideoGenerated(const QByteArray& videoData, const QString& prompt) {
    Q_UNUSED(prompt)

    if (videoData.isEmpty()) {
        codex::utils::MessageBox::warning(this, "Erreur", "Les donnees video recues sont vides.");
        return;
    }

    // Save video to output folder
    QString outputPath = codex::utils::Config::instance().outputImagesPath();
    QDir().mkpath(outputPath);  // Ensure directory exists
    QString fileName = QString("codex_video_%1.mp4")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString filePath = outputPath + "/" + fileName;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(videoData);
        file.close();

        statusBar()->showMessage(QString("Video generee: %1").arg(filePath));
        codex::utils::MessageBox::info(this, "Video generee",
            QString("La video a ete sauvegardee:\n%1\n\nTaille: %2 KB")
                .arg(filePath)
                .arg(videoData.size() / 1024));

        LOG_INFO(QString("Video saved: %1 (%2 bytes)").arg(filePath).arg(videoData.size()));
    } else {
        codex::utils::MessageBox::warning(this, "Erreur",
            QString("Impossible de sauvegarder la video:\n%1").arg(filePath));
        LOG_ERROR(QString("Failed to save video: %1").arg(filePath));
    }
}

void MainWindow::onVideoProgress(int percent) {
    statusBar()->showMessage(QString("Generation video: %1%").arg(percent));
}

void MainWindow::onVideoError(const QString& error) {
    statusBar()->showMessage(QString("Erreur video: %1").arg(error));
    codex::utils::MessageBox::critical(this, "Erreur de generation video", error);
    LOG_ERROR(QString("Video generation failed: %1").arg(error));
}

void MainWindow::onPipelineStateChanged(codex::core::PipelineState state, const QString& message) {
    Q_UNUSED(state)
    statusBar()->showMessage(message);
}

void MainWindow::onPipelineProgress(int percent, const QString& step) {
    statusBar()->showMessage(QString("%1 (%2%)").arg(step).arg(percent));

    // Update progress bar if in full generation mode
    if (m_fullGenerating) {
        // Steps: 0-33% = prompt, 33-66% = image, 66-100% = audio
        if (step.contains("prompt", Qt::CaseInsensitive)) {
            m_progressBar->setValue(percent / 3);
            m_progressBar->setFormat(QString("Etape 1/3: Prompt... %1%").arg(percent));
        } else if (step.contains("image", Qt::CaseInsensitive) || step.contains("Imagen", Qt::CaseInsensitive)) {
            m_progressBar->setValue(33 + percent / 3);
            m_progressBar->setFormat(QString("Etape 2/3: Image... %1%").arg(percent));
        }
    }
}

void MainWindow::onPipelineCompleted(const QPixmap& image, const QString& prompt) {
    // Store the prompt and display it in the prompt tab
    if (!prompt.isEmpty()) {
        m_generatedPrompt = prompt;
        m_promptEdit->setPlainText(prompt);
        m_centerTabWidget->setCurrentIndex(1);  // Switch to Prompt tab
    }

    if (m_plateGenerating && m_plateNextIndex < m_plateTextSegments.size()) {
        // We're in plate generation mode - add image to grid
        QString segmentText = m_plateTextSegments[m_plateNextIndex];
        m_imageViewer->setPlateGridImage(m_plateNextIndex, image, segmentText);

        LOG_INFO(QString("Plate image %1 completed, size: %2x%3")
                 .arg(m_plateNextIndex + 1).arg(image.width()).arg(image.height()));

        // Move to next image
        m_plateNextIndex++;

        // Generate next image after a small delay to avoid overwhelming the API
        QTimer::singleShot(500, this, &MainWindow::generateNextPlateImage);
    } else if (m_fullGenerating) {
        // Full generation mode - image done, now generate audio
        m_imageViewer->setImage(image);
        m_fullGenStep = 2;

        m_progressBar->setValue(66);
        m_progressBar->setFormat("Etape 3/3: Generation de l'audio...");
        statusBar()->showMessage("Generation complete: creation de l'audio...");

        LOG_INFO(QString("Full generation: image completed, starting audio. Size: %1x%2")
                 .arg(image.width()).arg(image.height()));

        // Generate audio
        onGenerateAudioFromPreview(m_selectedPassage);
    } else {
        // Normal single image generation
        m_imageViewer->setImage(image);
        statusBar()->showMessage("Image generee avec succes!");

        LOG_INFO(QString("Pipeline completed, image size: %1x%2")
                 .arg(image.width()).arg(image.height()));
    }
}

void MainWindow::onPipelineFailed(const QString& error) {
    LOG_ERROR(QString("Pipeline failed: %1").arg(error));

    if (m_plateGenerating && m_plateNextIndex < m_plateTextSegments.size()) {
        // In plate mode - skip this image and continue
        statusBar()->showMessage(QString("Image %1 echouee, passage a la suivante...")
                                 .arg(m_plateNextIndex + 1));

        m_plateNextIndex++;
        QTimer::singleShot(500, this, &MainWindow::generateNextPlateImage);
    } else if (m_fullGenerating) {
        // Full generation mode - cancel
        m_fullGenerating = false;
        m_progressBar->hide();
        m_imageViewer->showPlaceholder();
        statusBar()->showMessage(QString("Erreur: %1").arg(error));
        codex::utils::MessageBox::critical(this, "Erreur de generation", error);
    } else {
        // Normal mode - show error
        m_imageViewer->showPlaceholder();
        statusBar()->showMessage(QString("Erreur: %1").arg(error));
        codex::utils::MessageBox::critical(this, "Erreur de generation", error);
    }
}

void MainWindow::onSaveImage() {
    if (!m_imageViewer->hasImage()) {
        codex::utils::MessageBox::warning(this, "Erreur", "Aucune image a sauvegarder.");
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
        codex::utils::MessageBox::warning(this, "Erreur", "Impossible de sauvegarder l'image.");
        LOG_ERROR(QString("Failed to save image to: %1").arg(filePath));
    }
}

void MainWindow::onStartSlideshow() {
    // Check if we have selected text
    if (m_selectedPassage.isEmpty()) {
        codex::utils::MessageBox::info(this, "Diaporama",
            "Selectionnez un passage de texte avant de lancer le diaporama.\n\n"
            "Le diaporama generera automatiquement les images et l'audio pour le texte selectionne.");
        return;
    }

    // Create and show the slideshow dialog
    SlideshowDialog* dialog = new SlideshowDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Set the content with selected text and current treatise info
    dialog->setContent(m_selectedPassage, m_currentTreatiseCode, m_currentCategory, 2, 2);

    dialog->show();

    statusBar()->showMessage("Diaporama ouvert");
    LOG_INFO("Slideshow dialog opened");
}

void MainWindow::onGenerateAllAndSlideshow() {
    if (m_selectedPassage.isEmpty()) {
        codex::utils::MessageBox::info(this, "Generation complete",
            "Selectionnez un passage de texte avant de lancer la generation complete.");
        return;
    }

    if (m_fullGenerating) {
        codex::utils::MessageBox::warning(this, "Generation en cours",
            "Une generation est deja en cours. Veuillez patienter.");
        return;
    }

    // Start full generation pipeline: prompt -> image -> audio -> slideshow
    m_fullGenerating = true;
    m_fullGenStep = 0;
    m_generatedPrompt.clear();
    m_lastAudioPath.clear();

    // Show progress bar
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Etape 1/3: Generation du prompt...");

    statusBar()->showMessage("Generation complete: creation du prompt...");
    LOG_INFO("Starting full generation pipeline");

    // Step 1: Generate prompt (this triggers the pipeline which generates prompt then image)
    m_imageViewer->showLoading();
    m_pipelineController->startGeneration(m_selectedPassage, m_currentTreatiseCode, m_currentCategory);
}

void MainWindow::onFullGenerationCompleted() {
    if (!m_fullGenerating) return;

    m_fullGenerating = false;
    m_progressBar->hide();

    statusBar()->showMessage("Generation complete terminee! Lancement du diaporama...");
    LOG_INFO("Full generation completed, launching slideshow");

    // Launch slideshow
    QTimer::singleShot(500, this, &MainWindow::onStartSlideshow);
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

void MainWindow::onVoiceChanged(int /*index*/) {
    QString voiceId = m_voiceCombo->currentData().toString();
    codex::utils::Config::instance().setEdgeTtsVoice(voiceId);
    LOG_INFO(QString("Voice changed to: %1").arg(voiceId));
}

void MainWindow::onBrowseOutputFolder() {
    QString path = QFileDialog::getExistingDirectory(
        this, "Selectionner dossier images",
        m_outputFolderEdit->text()
    );
    if (!path.isEmpty()) {
        m_outputFolderEdit->setText(path);
        codex::utils::Config::instance().setOutputImagesPath(path);
        LOG_INFO(QString("Images folder changed to: %1").arg(path));
    }
}

void MainWindow::onBrowseVideoFolder() {
    QString path = QFileDialog::getExistingDirectory(
        this, "Selectionner dossier videos",
        m_videoFolderEdit->text()
    );
    if (!path.isEmpty()) {
        m_videoFolderEdit->setText(path);
        codex::utils::Config::instance().setOutputVideosPath(path);
        LOG_INFO(QString("Videos folder changed to: %1").arg(path));
    }
}

void MainWindow::onTestVoice() {
    QString voiceId = m_voiceCombo->currentData().toString();
    QString testText = QString("Bonjour, je suis la voix %1. Ceci est un test de synthese vocale.")
        .arg(m_voiceCombo->currentText());

    statusBar()->showMessage(QString("Test de la voix %1...").arg(m_voiceCombo->currentText()));

    codex::api::EdgeVoiceSettings settings;
    settings.voiceId = voiceId;
    settings.rate = 0;
    settings.pitch = 0;
    settings.volume = 100;

    m_edgeTTSClient->generateSpeech(testText, settings);

    LOG_INFO(QString("Testing voice: %1").arg(voiceId));
}

void MainWindow::onOpenImagesFolder() {
    QString path = codex::utils::Config::instance().outputImagesPath();
    if (path.isEmpty()) {
        path = QDir::homePath();
    }
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    LOG_INFO(QString("Opening images folder: %1").arg(path));
}

void MainWindow::onOpenVideosFolder() {
    QString path = codex::utils::Config::instance().outputVideosPath();
    if (path.isEmpty()) {
        path = QDir::homePath();
    }
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    LOG_INFO(QString("Opening videos folder: %1").arg(path));
}

void MainWindow::onOpenAudioFolder() {
    QString path = codex::utils::Config::instance().outputAudioPath();
    if (path.isEmpty()) {
        path = QDir::homePath();
    }
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    LOG_INFO(QString("Opening audio folder: %1").arg(path));
}

void MainWindow::onPlateSizeChanged(int cols, int rows) {
    // Show empty grid immediately when plate size is selected
    if (!m_imageViewer) return;  // Safety check

    m_plateCols = cols;
    m_plateRows = rows;
    m_imageViewer->startPlateGrid(cols, rows);

    statusBar()->showMessage(QString("Planche %1x%2 prete - Selectionnez un passage et cliquez 'Generer Planche'")
                             .arg(cols).arg(rows));

    LOG_INFO(QString("Plate grid preview: %1x%2").arg(cols).arg(rows));
}

void MainWindow::onGeneratePlateFromPreview(const QString& passage, int cols, int rows) {
    if (passage.isEmpty()) {
        codex::utils::MessageBox::warning(this, "Erreur",
            "Aucun passage selectionne pour la generation de planche.");
        return;
    }

    if (m_pipelineController->isRunning()) {
        codex::utils::MessageBox::warning(this, "Generation en cours",
            "Une generation est deja en cours. Veuillez patienter.");
        return;
    }

    int totalImages = cols * rows;
    m_plateCols = cols;
    m_plateRows = rows;
    m_plateTextSegments = splitTextForPlate(passage, totalImages);
    m_plateNextIndex = 0;
    m_plateGenerating = true;

    // Start the grid display
    m_imageViewer->startPlateGrid(cols, rows);

    statusBar()->showMessage(QString("Generation de planche %1x%2 : demarrage...")
                             .arg(cols).arg(rows));

    LOG_INFO(QString("Starting plate generation: %1x%2, %3 segments")
             .arg(cols).arg(rows).arg(m_plateTextSegments.size()));

    // Start generating first image
    generateNextPlateImage();
}

QStringList MainWindow::splitTextForPlate(const QString& text, int count) {
    QStringList segments;
    if (text.isEmpty() || count <= 0) return segments;

    // Split by sentences first
    QStringList sentences;
    QString current;
    for (int i = 0; i < text.length(); ++i) {
        current += text[i];
        if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
            // Check if it's end of sentence (not abbreviation)
            if (i + 1 >= text.length() || text[i + 1] == ' ' || text[i + 1] == '\n') {
                sentences.append(current.trimmed());
                current.clear();
            }
        }
    }
    if (!current.trimmed().isEmpty()) {
        sentences.append(current.trimmed());
    }

    // If we have enough sentences, distribute them
    if (sentences.size() >= count) {
        int perSegment = sentences.size() / count;
        int remainder = sentences.size() % count;

        int idx = 0;
        for (int i = 0; i < count; ++i) {
            QString segment;
            int sentenceCount = perSegment + (i < remainder ? 1 : 0);
            for (int j = 0; j < sentenceCount && idx < sentences.size(); ++j, ++idx) {
                if (!segment.isEmpty()) segment += " ";
                segment += sentences[idx];
            }
            segments.append(segment);
        }
    } else {
        // Not enough sentences - split text into equal parts
        int charsPerSegment = text.length() / count;
        for (int i = 0; i < count; ++i) {
            int start = i * charsPerSegment;
            int len = (i == count - 1) ? -1 : charsPerSegment;

            QString segment = text.mid(start, len).trimmed();

            // Try to end at word boundary
            if (len > 0 && start + len < text.length()) {
                int lastSpace = segment.lastIndexOf(' ');
                if (lastSpace > segment.length() * 0.7) {
                    segment = segment.left(lastSpace).trimmed();
                }
            }

            if (!segment.isEmpty()) {
                segments.append(segment);
            }
        }
    }

    // Ensure we have exactly 'count' segments
    while (segments.size() < count) {
        // Duplicate last segment if needed
        segments.append(segments.isEmpty() ? text.left(100) : segments.last());
    }
    while (segments.size() > count) {
        segments.removeLast();
    }

    return segments;
}

void MainWindow::generateNextPlateImage() {
    if (!m_plateGenerating || m_plateNextIndex >= m_plateTextSegments.size()) {
        // All done
        m_plateGenerating = false;
        m_imageViewer->finishPlateGrid();
        statusBar()->showMessage(QString("Planche %1x%2 terminee!")
                                 .arg(m_plateCols).arg(m_plateRows));
        LOG_INFO("Plate generation completed");
        return;
    }

    QString segment = m_plateTextSegments[m_plateNextIndex];
    statusBar()->showMessage(QString("Generation planche: image %1/%2...")
                             .arg(m_plateNextIndex + 1)
                             .arg(m_plateTextSegments.size()));

    LOG_INFO(QString("Generating plate image %1/%2: %3 chars")
             .arg(m_plateNextIndex + 1)
             .arg(m_plateTextSegments.size())
             .arg(segment.length()));

    // Start generation for this segment
    m_pipelineController->startGeneration(segment, m_currentTreatiseCode, m_currentCategory);
}

} // namespace codex::ui
