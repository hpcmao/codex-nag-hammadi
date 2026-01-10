#include "MainWindow.h"
#include "widgets/TextViewerWidget.h"
#include "widgets/ImageViewerWidget.h"
#include "widgets/TreatiseListWidget.h"
#include "widgets/PassagePreviewWidget.h"
// Audio player removed - playback is in slideshow dialog
#include "widgets/SlideshowWidget.h"
#include "widgets/InfoDockWidget.h"
#include "widgets/ApiPricingDockWidget.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/ProjectDialog.h"
#include "dialogs/SlideshowDialog.h"
#include "dialogs/SessionPickerDialog.h"
#include "db/repositories/PassageRepository.h"
#include "db/repositories/ImageRepository.h"
#include "db/repositories/AudioRepository.h"
#include "api/ElevenLabsClient.h"
#include "api/EdgeTTSClient.h"
#include "api/VeoClient.h"
#include "core/services/TextParser.h"
#include "core/services/NarrationCleaner.h"
#include "core/controllers/PipelineController.h"
#include "utils/Logger.h"
#include "utils/Config.h"
#include "utils/SecureStorage.h"
#include "utils/MessageBox.h"
#include "utils/ThemeManager.h"
#include "utils/MediaStorage.h"
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
#include <QPointer>
#include <QDesktopServices>
#include <QUrl>
#include <QCloseEvent>

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

    // Load saved text if "remember text" option is enabled (deferred to ensure UI is ready)
    QTimer::singleShot(600, this, [this]() {
        auto& cfg = codex::utils::Config::instance();
        if (cfg.rememberText()) {
            QString lastText = cfg.lastText();
            QString lastTreatise = cfg.lastTreatiseCode();
            int selStart = cfg.lastSelectionStart();
            int selEnd = cfg.lastSelectionEnd();

            if (!lastTreatise.isEmpty()) {
                // First, select the treatise (this loads the text into the viewer)
                m_treatiseList->selectTreatiseByCode(lastTreatise);

                // Then after a short delay, highlight the selection in the text viewer
                QTimer::singleShot(300, this, [this, lastText, lastTreatise, selStart, selEnd]() {
                    if (selStart >= 0 && selEnd > selStart) {
                        m_textViewer->selectRange(selStart, selEnd);
                        m_selectedPassage = lastText;
                        m_currentTreatiseCode = lastTreatise;
                        m_selectionStart = selStart;
                        m_selectionEnd = selEnd;
                        m_passagePreview->setPassage(lastText, selStart, selEnd);
                        LOG_INFO(QString("Restored selection [%1-%2] in treatise: %3")
                                 .arg(selStart).arg(selEnd).arg(lastTreatise));
                    }
                });
            }
        }
    });

    LOG_INFO("MainWindow initialized");
}

MainWindow::~MainWindow() {
    delete m_textParser;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Save text if "remember text" option is enabled
    auto& config = codex::utils::Config::instance();
    if (config.rememberText()) {
        config.setLastText(m_selectedPassage);
        config.setLastTreatiseCode(m_currentTreatiseCode);
        config.setLastSelection(m_selectionStart, m_selectionEnd);
        LOG_INFO(QString("Saved text (%1 chars) and selection [%2-%3] for next session")
                 .arg(m_selectedPassage.length()).arg(m_selectionStart).arg(m_selectionEnd));
    }

    QMainWindow::closeEvent(event);
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
    // Image viewer (full height, no audio player - audio is in slideshow)
    m_imageViewer = new ImageViewerWidget(this);
    rightSplitter->addWidget(m_imageViewer);

    // Set right splitter sizes (50% text area, 50% image viewer)
    rightSplitter->setSizes({500, 500});

    mainSplitter->addWidget(rightSplitter);

    // Set main splitter sizes (300px list, rest for content)
    mainSplitter->setSizes({300, 900});

    setCentralWidget(mainSplitter);

    // Info dock widget (API pricing/quotas)
    m_infoDock = new InfoDockWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, m_infoDock);
    m_infoDock->hide();  // Hidden by default

    // API Pricing dock - shows model selection and costs
    m_pricingDock = new ApiPricingDockWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, m_pricingDock);
    m_pricingDock->show();  // Visible by default

    // Toolbar
    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    auto* openAction = toolbar->addAction("Ouvrir");
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    toolbar->addSeparator();

    auto* slideshowAction = toolbar->addAction("Diaporama");
    connect(slideshowAction, &QAction::triggered, this, &MainWindow::onStartSlideshow);

    toolbar->addSeparator();

    // Plate size selector
    toolbar->addWidget(new QLabel(" Planche: ", this));
    m_plateSizeCombo = new QComboBox(this);
    m_plateSizeCombo->addItem("2x2 (4)", QSize(2, 2));
    m_plateSizeCombo->addItem("3x3 (9)", QSize(3, 3));
    m_plateSizeCombo->addItem("4x4 (16)", QSize(4, 4));
    m_plateSizeCombo->addItem("2x3 (6)", QSize(2, 3));
    m_plateSizeCombo->addItem("3x2 (6)", QSize(3, 2));
    m_plateSizeCombo->addItem("4x3 (12)", QSize(4, 3));
    m_plateSizeCombo->setCurrentIndex(1);  // Default 3x3
    m_plateSizeCombo->setFixedWidth(90);
    m_plateSizeCombo->setStyleSheet(R"(
        QComboBox {
            background-color: #2d2d2d;
            color: #d4d4d4;
            border: 1px solid #3d3d3d;
            border-radius: 3px;
            padding: 3px 8px;
        }
        QComboBox:hover { border-color: #007acc; }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView {
            background-color: #2d2d2d;
            color: #d4d4d4;
            selection-background-color: #094771;
        }
    )");
    connect(m_plateSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QSize size = m_plateSizeCombo->itemData(index).toSize();
        m_plateCols = size.width();
        m_plateRows = size.height();
    });
    // Initialize with default value (3x3)
    m_plateCols = 3;
    m_plateRows = 3;
    toolbar->addWidget(m_plateSizeCombo);

    // "Générer Tout + Diaporama" button
    m_genAllBtn = new QPushButton("Generer Tout + Diapo", this);
    m_genAllBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #1e5a1e;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 5px 12px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #2d7a2d; }
        QPushButton:pressed { background-color: #0d3a0d; }
    )");
    m_genAllBtn->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
    connect(m_genAllBtn, &QPushButton::clicked, this, &MainWindow::onGenerateAllAndSlideshow);
    toolbar->addWidget(m_genAllBtn);

    // "Générer Vidéo" button
    m_genVideoBtn = new QPushButton("Generer Video", this);
    m_genVideoBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #5a1e5a;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 5px 12px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #7a2d7a; }
        QPushButton:pressed { background-color: #3a0d3a; }
    )");
    m_genVideoBtn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_V));
    m_genVideoBtn->setToolTip("Generer une video a partir du texte selectionne uniquement (Ctrl+V)");
    connect(m_genVideoBtn, &QPushButton::clicked, this, [this]() {
        if (!m_selectedPassage.isEmpty()) {
            onGenerateVideoFromPreview(m_selectedPassage);
        }
    });
    toolbar->addWidget(m_genVideoBtn);

    // Progress bar next to the button
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFixedWidth(250);
    m_progressBar->setFixedHeight(24);
    m_progressBar->hide();  // Hidden by default
    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            border: 1px solid #3d3d3d;
            border-radius: 4px;
            background-color: #2d2d2d;
            text-align: center;
            color: #fff;
            font-weight: bold;
        }
        QProgressBar::chunk {
            background-color: #1e5a1e;
            border-radius: 3px;
        }
    )");
    toolbar->addWidget(m_progressBar);

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

    auto* pricingAction = viewMenu->addAction("&Tarifs && Modeles...");
    pricingAction->setShortcut(QKeySequence(Qt::Key_F2));
    pricingAction->setCheckable(true);
    pricingAction->setChecked(true);  // Visible by default
    connect(pricingAction, &QAction::triggered, this, [this](bool checked) {
        m_pricingDock->setVisible(checked);
    });
    connect(m_pricingDock, &QDockWidget::visibilityChanged, pricingAction, &QAction::setChecked);

    viewMenu->addSeparator();

    auto* openImagesFolderAction = viewMenu->addAction("Ouvrir dossier &Images");
    connect(openImagesFolderAction, &QAction::triggered, this, &MainWindow::onOpenImagesFolder);

    auto* openVideosFolderAction = viewMenu->addAction("Ouvrir dossier &Videos");
    connect(openVideosFolderAction, &QAction::triggered, this, &MainWindow::onOpenVideosFolder);

    auto* openAudioFolderAction = viewMenu->addAction("Ouvrir dossier &Audio");
    connect(openAudioFolderAction, &QAction::triggered, this, &MainWindow::onOpenAudioFolder);

    viewMenu->addSeparator();

    // Remember text option
    auto* rememberTextAction = viewMenu->addAction("&Garder texte a la fermeture");
    rememberTextAction->setCheckable(true);
    rememberTextAction->setChecked(codex::utils::Config::instance().rememberText());
    connect(rememberTextAction, &QAction::triggered, this, [](bool checked) {
        codex::utils::Config::instance().setRememberText(checked);
    });

    // Help menu
    auto* helpMenu = menuBar()->addMenu("&Aide");

    auto* aboutAction = helpMenu->addAction("À &propos...");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onShowAbout);

    // Planche menu
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

    // Generation menu (after Planche)
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

    // Update MediaStorage base path to project root
    codex::utils::MediaStorage::instance().updateBasePath();

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
    m_selectionStart = start;
    m_selectionEnd = end;
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

    // Nettoyer le texte pour la narration
    codex::core::NarrationCleaner cleaner;
    QString cleanedPassage = cleaner.clean(passage);

    LOG_INFO(QString("Narration text cleaned: %1 -> %2 chars")
             .arg(passage.length()).arg(cleanedPassage.length()));

    auto& config = codex::utils::Config::instance();
    QString ttsProvider = config.ttsProvider();

    statusBar()->showMessage("Generation audio en cours...");

    if (ttsProvider == "edge") {
        // Use Microsoft Edge Neural TTS (free, high quality)
        codex::api::EdgeVoiceSettings edgeSettings;
        edgeSettings.voiceId = config.edgeTtsVoice();
        edgeSettings.rate = -15;     // Slightly slower for narration (-100 to 100)
        edgeSettings.pitch = 0;      // Normal pitch
        edgeSettings.volume = 100;   // Full volume

        m_edgeTTSClient->generateSpeech(cleanedPassage, edgeSettings);

        LOG_INFO(QString("Edge TTS generation started for passage: %1 chars, voice: %2")
                 .arg(cleanedPassage.length()).arg(edgeSettings.voiceId));
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

        m_elevenLabsClient->generateSpeech(cleanedPassage, voiceSettings);

        LOG_INFO(QString("ElevenLabs generation started for passage: %1 chars, voice: %2")
                 .arg(cleanedPassage.length()).arg(voiceSettings.voiceId));
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
    }
    // Audio playback moved to slideshow dialog
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
    }
    // Audio playback moved to slideshow dialog
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
    // Update progress bar for plate generation mode
    if (m_plateGenerating && m_plateTextSegments.size() > 0) {
        // Calculate fine progress: current image + progress within that image
        int totalImages = m_plateTextSegments.size();
        int completedImages = m_plateNextIndex;

        // Each image contributes (100 / totalImages) to total progress
        // Current image progress adds partial contribution
        int basePercent = (completedImages * 100) / totalImages;
        int totalPercent = basePercent + (percent / totalImages);

        m_progressBar->setValue(totalPercent * totalImages / 100);  // Scale to range

        // Update button text with fine progress
        m_genAllBtn->setText(QString("Pause (%1%)").arg(totalPercent));

        statusBar()->showMessage(QString("Image %1/%2: %3 (%4%)")
            .arg(completedImages + 1).arg(totalImages).arg(step).arg(percent));
    }
    // Update progress bar if in full generation mode
    else if (m_fullGenerating) {
        // Steps: 0-33% = prompt, 33-66% = image, 66-100% = audio
        if (step.contains("prompt", Qt::CaseInsensitive)) {
            m_progressBar->setValue(percent / 3);
            m_progressBar->setFormat(QString("Etape 1/3: Prompt... %1%").arg(percent));
        } else if (step.contains("image", Qt::CaseInsensitive) || step.contains("Imagen", Qt::CaseInsensitive)) {
            m_progressBar->setValue(33 + percent / 3);
            m_progressBar->setFormat(QString("Etape 2/3: Image... %1%").arg(percent));
        }
        statusBar()->showMessage(QString("%1 (%2%)").arg(step).arg(percent));
    }
    else {
        statusBar()->showMessage(QString("%1 (%2%)").arg(step).arg(percent));
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

        // Auto-save image to MediaStorage
        codex::utils::MediaStorage::instance().saveImage(image, m_plateNextIndex, segmentText);

        // Open slideshow on first image if requested
        if (m_plateNextIndex == 0 && m_openSlideshowOnFirstImage && !m_activeSlideshowDialog) {
            m_openSlideshowOnFirstImage = false;  // Only open once

            SlideshowDialog* dialog = new SlideshowDialog(this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            m_activeSlideshowDialog = dialog;

            connect(dialog, &QObject::destroyed, this, [this]() {
                m_activeSlideshowDialog = nullptr;
                LOG_INFO("Slideshow closed");
            });

            dialog->setContent(m_selectedPassage, m_currentTreatiseCode, m_currentCategory, m_plateCols, m_plateRows);
            dialog->prepareSlideshow();

            // Show the slideshow dialog maximized
            dialog->show();
            dialog->setWindowState(Qt::WindowMaximized);
            dialog->raise();
            dialog->activateWindow();

            statusBar()->showMessage("Diaporama ouvert - generation en cours...");
            LOG_INFO("Slideshow opened on first image");
        }

        // Send image to active slideshow dialog if one exists
        if (m_activeSlideshowDialog) {
            m_activeSlideshowDialog->addImage(image, segmentText, m_plateNextIndex);
            LOG_INFO(QString("Sent image %1 to slideshow").arg(m_plateNextIndex));
        }

        // Move to next image
        m_plateNextIndex++;

        // Update progress bar and button text
        m_progressBar->setValue(m_plateNextIndex);
        int percent = (m_plateNextIndex * 100) / m_plateTextSegments.size();
        m_genAllBtn->setText(QString("Pause (%1%)").arg(percent));

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

        // Notify slideshow of failure (it will handle placeholder)
        // Note: slideshow will just not receive this image

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

    // Close any existing slideshow dialog
    if (m_activeSlideshowDialog) {
        m_activeSlideshowDialog->close();
        m_activeSlideshowDialog = nullptr;
    }

    // Create the slideshow dialog (no PipelineController - it receives images from MainWindow)
    SlideshowDialog* dialog = new SlideshowDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    m_activeSlideshowDialog = dialog;

    // When dialog is destroyed, clear the pointer
    connect(dialog, &QObject::destroyed, this, [this]() {
        m_activeSlideshowDialog = nullptr;
        LOG_INFO("Slideshow closed");
    });

    // Use grid dimensions from ImageViewer if available, otherwise default to 2x2
    int cols = m_imageViewer->gridCols() > 0 ? m_imageViewer->gridCols() : (m_plateCols > 0 ? m_plateCols : 2);
    int rows = m_imageViewer->gridRows() > 0 ? m_imageViewer->gridRows() : (m_plateRows > 0 ? m_plateRows : 2);

    dialog->setContent(m_selectedPassage, m_currentTreatiseCode, m_currentCategory, cols, rows);

    // Prepare the slideshow (creates placeholder slots for images)
    dialog->prepareSlideshow();

    // Show the dialog maximized
    dialog->show();
    dialog->setWindowState(Qt::WindowMaximized);
    dialog->raise();
    dialog->activateWindow();

    // Send existing images from ImageViewer grid to slideshow
    int existingImages = m_imageViewer->gridImageCount();
    if (existingImages > 0) {
        LOG_INFO(QString("Sending %1 existing grid images to slideshow").arg(existingImages));
        for (int i = 0; i < existingImages; ++i) {
            QPixmap img = m_imageViewer->gridImage(i);
            QString text = m_imageViewer->gridText(i);
            if (!img.isNull()) {
                dialog->addImage(img, text, i);
                LOG_INFO(QString("Sent existing image %1 to slideshow").arg(i));
            }
        }

        // If plate generation is done, notify slideshow
        if (!m_plateGenerating) {
            dialog->finishAddingImages();
            LOG_INFO("Notified slideshow that existing images are complete");
        }
    }

    statusBar()->showMessage("Diaporama ouvert");
    LOG_INFO(QString("Slideshow dialog opened with %1 existing images").arg(existingImages));

    // If no images exist and no generation in progress, start plate generation
    if (existingImages == 0 && !m_plateGenerating && !m_pipelineController->isRunning()) {
        LOG_INFO("Starting plate generation for slideshow");
        statusBar()->showMessage("Diaporama ouvert - generation en cours...");
        onPlateSizeChanged(cols, rows);
    } else if (m_plateGenerating) {
        statusBar()->showMessage("Diaporama ouvert - generation en cours...");
        LOG_INFO("Plate generation already in progress, slideshow will receive new images");
    }
}

void MainWindow::onGenerateAllAndSlideshow() {
    // If generation is in progress, handle pause/resume
    if (m_plateGenerating) {
        int percent = m_plateTextSegments.size() > 0
            ? (m_plateNextIndex * 100) / m_plateTextSegments.size()
            : 0;

        if (m_platePaused) {
            // Resume generation
            m_platePaused = false;
            m_genAllBtn->setText(QString("Pause (%1%)").arg(percent));
            m_genAllBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: #8a6d00;
                    color: white;
                    border: none;
                    border-radius: 4px;
                    padding: 5px 12px;
                    font-weight: bold;
                }
                QPushButton:hover { background-color: #a88500; }
                QPushButton:pressed { background-color: #6a5500; }
            )");
            statusBar()->showMessage("Generation reprise...");
            LOG_INFO("Plate generation resumed");
            // Continue generating
            generateNextPlateImage();
        } else {
            // Pause generation
            m_platePaused = true;
            m_genAllBtn->setText(QString("Reprendre (%1%)").arg(percent));
            m_genAllBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: #1e5a1e;
                    color: white;
                    border: none;
                    border-radius: 4px;
                    padding: 5px 12px;
                    font-weight: bold;
                }
                QPushButton:hover { background-color: #2d7a2d; }
                QPushButton:pressed { background-color: #0d3a0d; }
            )");
            statusBar()->showMessage(QString("Generation en pause (%1/%2 images)")
                .arg(m_plateNextIndex).arg(m_plateTextSegments.size()));
            LOG_INFO(QString("Plate generation paused at %1/%2")
                .arg(m_plateNextIndex).arg(m_plateTextSegments.size()));
        }
        return;
    }

    // Show session picker dialog
    SessionPickerDialog picker(this);
    if (picker.exec() != QDialog::Accepted) {
        return;
    }

    // If user chose an existing session, open it directly
    if (picker.resultType() == SessionPickerDialog::ExistingSession) {
        auto sessionInfo = picker.selectedSessionInfo();
        QString sessionPath = picker.selectedSessionPath();

        LOG_INFO(QString("Opening existing session: %1 with %2 images")
                 .arg(sessionPath).arg(sessionInfo.imageCount));

        // Create slideshow dialog with existing media
        SlideshowDialog* dialog = new SlideshowDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setPipelineController(m_pipelineController);

        // Load images and texts from session
        auto& storage = codex::utils::MediaStorage::instance();
        for (int i = 0; i < sessionInfo.imageCount; ++i) {
            QPixmap image = storage.loadImage(sessionPath, i);
            QString text = (i < sessionInfo.texts.size()) ? sessionInfo.texts[i] : QString();
            if (!image.isNull()) {
                dialog->addSlide(image, text);
            }
        }

        dialog->show();
        dialog->setWindowState(Qt::WindowMaximized);

        // Start auto-play if images loaded
        if (sessionInfo.imageCount > 0) {
            QPointer<SlideshowDialog> safeDialog = dialog;
            QTimer::singleShot(500, this, [safeDialog]() {
                if (safeDialog) {
                    safeDialog->startAutoPlay();
                }
            });
        }

        return;
    }

    // Start new generation
    if (m_selectedPassage.isEmpty()) {
        codex::utils::MessageBox::info(this, "Generation complete",
            "Selectionnez un passage de texte avant de lancer la generation complete.");
        return;
    }

    // Close any existing slideshow dialog
    if (m_activeSlideshowDialog) {
        m_activeSlideshowDialog->close();
        m_activeSlideshowDialog = nullptr;
    }

    // Clear existing plate images to start fresh
    m_imageViewer->clearPlate();
    m_platePaused = false;
    m_openSlideshowOnFirstImage = true;  // Open slideshow when first image is ready

    // Use default plate size (3x3 = 9 images) or last used size
    int cols = m_plateCols > 0 ? m_plateCols : 3;
    int rows = m_plateRows > 0 ? m_plateRows : 3;

    // Update button to show "Pause" state with 0%
    m_genAllBtn->setText("Pause (0%)");
    m_genAllBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #8a6d00;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 5px 12px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #a88500; }
        QPushButton:pressed { background-color: #6a5500; }
    )");

    // Show progress bar (keep it as backup indicator)
    m_progressBar->setRange(0, cols * rows);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Generation: %v/%m images");
    m_progressBar->show();

    statusBar()->showMessage("Generation en cours... Le diaporama s'ouvrira a la 1ere image.");
    LOG_INFO(QString("Starting plate generation %1x%2, slideshow will open on first image").arg(cols).arg(rows));

    // Start plate generation - slideshow will open when first image is ready
    QTimer::singleShot(100, this, [this, cols, rows]() {
        onGeneratePlateFromPreview(m_selectedPassage, cols, rows);
    });
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

    // Create a new session in MediaStorage for auto-saving
    // Note: MediaStorage uses the codex file's parent directory as base path
    codex::utils::MediaStorage::instance().updateBasePath();
    codex::utils::MediaStorage::instance().createSession(m_currentTreatiseCode);

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
    // Check if paused - don't continue generating
    if (m_platePaused) {
        LOG_INFO("Plate generation paused, waiting for resume");
        return;
    }

    if (!m_plateGenerating || m_plateNextIndex >= m_plateTextSegments.size()) {
        // All done
        m_plateGenerating = false;
        m_platePaused = false;
        m_imageViewer->finishPlateGrid();
        statusBar()->showMessage(QString("Planche %1x%2 terminee! Images sauvegardees dans %3")
                                 .arg(m_plateCols).arg(m_plateRows)
                                 .arg(codex::utils::MediaStorage::instance().currentSessionPath()));
        LOG_INFO("Plate generation completed");

        // Reset button to initial state
        m_genAllBtn->setText("Generer Tout + Diapo");
        m_genAllBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #1e5a1e;
                color: white;
                border: none;
                border-radius: 4px;
                padding: 5px 12px;
                font-weight: bold;
            }
            QPushButton:hover { background-color: #2d7a2d; }
            QPushButton:pressed { background-color: #0d3a0d; }
        )");

        // Hide progress bar
        m_progressBar->hide();

        // Save session metadata
        codex::utils::MediaStorage::instance().saveSessionMetadata(
            m_currentTreatiseCode, m_currentCategory,
            m_plateTextSegments.size(), m_plateTextSegments);

        // Notify slideshow that all images have been sent
        if (m_activeSlideshowDialog) {
            m_activeSlideshowDialog->finishAddingImages();
            LOG_INFO("Notified slideshow that all images are done");
        }
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
