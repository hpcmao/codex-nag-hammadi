# Architecture Technique
# Codex Nag Hammadi BD

**Version:** 1.0
**Date:** 2026-01-08
**Auteur:** Hpcmao
**Statut:** Draft

---

## 1. Vue d'Ensemble

### 1.1 Stack Technique Validé

| Composant | Technologie | Version |
|-----------|-------------|---------|
| Langage | C++20 | GCC 12+ / Clang 15+ / MSVC 2022 |
| Framework GUI | Qt 6 | 6.5+ |
| Base de données | SQLite | 3.40+ |
| Build System | CMake | 3.25+ |
| Package Manager | vcpkg (optionnel) | - |
| JSON | nlohmann/json | 3.11+ |
| HTTP | Qt Network | (inclus Qt) |
| Tests | Qt Test + Catch2 | - |

### 1.2 Décisions Architecturales

| ID | Décision | Justification |
|----|----------|---------------|
| ADR-01 | Signals/Slots Qt | Pattern natif, découplage, thread-safe |
| ADR-02 | QThread + Workers | Appels API non-bloquants |
| ADR-03 | SQLite embarqué | Portable, pas de serveur |
| ADR-04 | CMake + Presets | Build cross-platform standardisé |
| ADR-05 | Singleton pour Services | Accès global aux APIs, config |

---

## 2. Structure du Projet

```
codex-nag-hammadi/
├── CMakeLists.txt              # Build principal
├── CMakePresets.json           # Presets cross-platform
├── README.md
├── .gitignore
│
├── src/
│   ├── main.cpp                # Point d'entrée
│   │
│   ├── core/                   # Logique métier
│   │   ├── CMakeLists.txt
│   │   ├── models/             # Structures de données
│   │   │   ├── Project.h/.cpp
│   │   │   ├── Passage.h/.cpp
│   │   │   ├── GeneratedImage.h/.cpp
│   │   │   └── AudioFile.h/.cpp
│   │   ├── services/           # Services métier
│   │   │   ├── TextParser.h/.cpp
│   │   │   ├── MythicClassifier.h/.cpp
│   │   │   └── PromptBuilder.h/.cpp
│   │   └── entities/           # Dictionnaire gnostique
│   │       └── GnosticEntities.h/.cpp
│   │
│   ├── api/                    # Clients API externes
│   │   ├── CMakeLists.txt
│   │   ├── ApiClient.h/.cpp    # Base abstraite
│   │   ├── ClaudeClient.h/.cpp
│   │   ├── ImagenClient.h/.cpp
│   │   └── ElevenLabsClient.h/.cpp
│   │
│   ├── db/                     # Couche données
│   │   ├── CMakeLists.txt
│   │   ├── Database.h/.cpp     # Connexion SQLite
│   │   ├── migrations/         # Scripts SQL
│   │   │   └── 001_initial.sql
│   │   └── repositories/       # CRUD
│   │       ├── ProjectRepository.h/.cpp
│   │       ├── PassageRepository.h/.cpp
│   │       └── ImageRepository.h/.cpp
│   │
│   ├── ui/                     # Interface Qt
│   │   ├── CMakeLists.txt
│   │   ├── MainWindow.h/.cpp/.ui
│   │   ├── widgets/
│   │   │   ├── TextViewerWidget.h/.cpp
│   │   │   ├── ImageViewerWidget.h/.cpp
│   │   │   ├── ProjectListWidget.h/.cpp
│   │   │   └── SlideshowWidget.h/.cpp
│   │   ├── dialogs/
│   │   │   ├── SettingsDialog.h/.cpp/.ui
│   │   │   └── ExportDialog.h/.cpp/.ui
│   │   └── styles/
│   │       └── dark_theme.qss
│   │
│   └── utils/                  # Utilitaires
│       ├── CMakeLists.txt
│       ├── Config.h/.cpp       # Gestion configuration
│       ├── Logger.h/.cpp       # Journalisation
│       └── SecureStorage.h/.cpp # Stockage clés API
│
├── resources/
│   ├── codex-nag-hammadi.qrc   # Ressources Qt
│   ├── icons/
│   ├── fonts/
│   └── data/
│       ├── gnostic_entities.json
│       ├── mythic_categories.json
│       └── style_templates.json
│
├── tests/
│   ├── CMakeLists.txt
│   ├── test_text_parser.cpp
│   ├── test_mythic_classifier.cpp
│   └── test_prompt_builder.cpp
│
├── docs/
│   └── api_setup.md
│
└── scripts/
    ├── setup_vcpkg.sh
    └── build.sh
```

---

## 3. Architecture des Modules

### 3.1 Diagramme de Dépendances

```
┌─────────────────────────────────────────────────────────────────┐
│                            UI (Qt Widgets)                       │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐   │
│  │ MainWindow │ │TextViewer  │ │ImageViewer │ │ Slideshow  │   │
│  └─────┬──────┘ └─────┬──────┘ └─────┬──────┘ └─────┬──────┘   │
└────────┼──────────────┼──────────────┼──────────────┼───────────┘
         │              │              │              │
         └──────────────┴──────┬───────┴──────────────┘
                               │ signals/slots
┌──────────────────────────────┴──────────────────────────────────┐
│                          CORE (Business Logic)                   │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐   │
│  │ TextParser │ │ Classifier │ │PromptBuild │ │  Models    │   │
│  └─────┬──────┘ └─────┬──────┘ └─────┬──────┘ └─────┬──────┘   │
└────────┼──────────────┼──────────────┼──────────────┼───────────┘
         │              │              │              │
┌────────┴──────────────┴──────────────┴──────────────┴───────────┐
│                          API (External Services)                 │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐                   │
│  │ClaudeClient│ │ImagenClient│ │ElevenLabs  │  (QThread Workers)│
│  └─────┬──────┘ └─────┬──────┘ └─────┬──────┘                   │
└────────┼──────────────┼──────────────┼──────────────────────────┘
         │              │              │
         └──────────────┴──────┬───────┘
                               │ HTTP (Qt Network)
                               ▼
                        [APIs Externes]

┌─────────────────────────────────────────────────────────────────┐
│                          DB (Data Layer)                         │
│  ┌────────────┐ ┌────────────────────────────────┐              │
│  │  Database  │ │      Repositories              │              │
│  │  (SQLite)  │ │ Project | Passage | Image      │              │
│  └────────────┘ └────────────────────────────────┘              │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
                        [SQLite File]
```

---

## 4. Composants Détaillés

### 4.1 Module Core

#### 4.1.1 TextParser

```cpp
// src/core/services/TextParser.h

#pragma once

#include <QString>
#include <QVector>
#include <memory>

namespace codex::core {

struct ParsedTreatise {
    QString code;           // "I-1", "II-3", etc.
    QString title;
    QString fullText;
    QVector<QString> pages;
};

struct ParsedPassage {
    QString text;
    int startPos;
    int endPos;
    QStringList detectedEntities;
};

class TextParser {
public:
    // Parse un fichier Markdown complet
    QVector<ParsedTreatise> parseCodexFile(const QString& filePath);

    // Extrait un passage sélectionné
    ParsedPassage extractPassage(const QString& fullText, int start, int end);

    // Détecte les entités gnostiques dans le texte
    QStringList detectGnosticEntities(const QString& text);

private:
    QMap<QString, QStringList> m_entityKeywords;
    void loadEntityKeywords();
};

} // namespace codex::core
```

#### 4.1.2 MythicClassifier

```cpp
// src/core/services/MythicClassifier.h

#pragma once

#include <QString>
#include <QColor>
#include <QVector>

namespace codex::core {

enum class MythicCategory {
    Plerome,      // Lumière divine
    Sophia,       // Chute
    Demiurge,     // Création matérielle
    Gnosis,       // Révélation
    Ascension,    // Retour
    Liturgy,      // Prières
    Hermetic,     // Philosophie mixte
    Narrative,    // Récits
    Fragments     // Incomplets
};

struct CategoryStyle {
    MythicCategory category;
    QString name;
    QVector<QColor> palette;
    QStringList visualKeywords;
    QString lightingStyle;
};

class MythicClassifier {
public:
    MythicClassifier();

    // Classifie un traité par son code
    MythicCategory classifyTreatise(const QString& treatiseCode);

    // Retourne le style visuel pour une catégorie
    CategoryStyle getStyleForCategory(MythicCategory category);

    // Override manuel de la classification
    void setManualClassification(const QString& treatiseCode, MythicCategory category);

private:
    QMap<QString, MythicCategory> m_treatiseMap;
    QMap<MythicCategory, CategoryStyle> m_styleMap;

    void initializeTreatiseMap();
    void initializeStyleMap();
};

} // namespace codex::core
```

#### 4.1.3 PromptBuilder

```cpp
// src/core/services/PromptBuilder.h

#pragma once

#include <QString>
#include "MythicClassifier.h"

namespace codex::core {

struct EnrichedPrompt {
    QString sceneDescription;
    QString emotionalTone;
    QString composition;
    QString finalPrompt;
};

struct ClaudeResponse {
    QString scene;
    QString emotion;
    QString composition;
    QStringList visualElements;
};

class PromptBuilder {
public:
    PromptBuilder();

    // Construit le prompt pour Claude API
    QString buildClaudePrompt(const QString& passageText,
                              const QStringList& entities,
                              MythicCategory category);

    // Assemble le prompt final pour Imagen
    QString buildImagenPrompt(const ClaudeResponse& claudeResponse,
                              const CategoryStyle& style);

    // Template Villeneuve de base
    QString getVilleneuveTemplate() const;

private:
    QString m_baseTemplate;
    void loadTemplates();
};

} // namespace codex::core
```

---

### 4.2 Module API

#### 4.2.1 ApiClient (Base)

```cpp
// src/api/ApiClient.h

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <functional>

namespace codex::api {

class ApiClient : public QObject {
    Q_OBJECT

public:
    explicit ApiClient(QObject* parent = nullptr);
    virtual ~ApiClient() = default;

    void setApiKey(const QString& key);
    bool isConfigured() const;

signals:
    void requestStarted();
    void requestFinished();
    void errorOccurred(const QString& error);

protected:
    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    QString m_baseUrl;

    QNetworkRequest createRequest(const QString& endpoint);
    void handleNetworkError(QNetworkReply* reply);
};

} // namespace codex::api
```

#### 4.2.2 ClaudeClient

```cpp
// src/api/ClaudeClient.h

#pragma once

#include "ApiClient.h"
#include <QJsonObject>

namespace codex::api {

struct ClaudeMessage {
    QString role;
    QString content;
};

class ClaudeClient : public ApiClient {
    Q_OBJECT

public:
    explicit ClaudeClient(QObject* parent = nullptr);

    // Appel asynchrone à Claude API
    void enrichPassage(const QString& prompt);

signals:
    void enrichmentCompleted(const QJsonObject& response);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QString m_model = "claude-sonnet-4-20250514";
    int m_maxTokens = 1000;
};

} // namespace codex::api
```

#### 4.2.3 ImagenClient

```cpp
// src/api/ImagenClient.h

#pragma once

#include "ApiClient.h"
#include <QPixmap>

namespace codex::api {

struct ImageGenerationParams {
    QString prompt;
    QString aspectRatio = "16:9";
    int numberOfImages = 1;
    QString outputFormat = "png";
};

class ImagenClient : public ApiClient {
    Q_OBJECT

public:
    explicit ImagenClient(QObject* parent = nullptr);

    // Génère une image à partir du prompt
    void generateImage(const ImageGenerationParams& params);

signals:
    void imageGenerated(const QPixmap& image, const QString& prompt);
    void generationProgress(int percent);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QString m_model = "imagen-3.0-generate-001";
};

} // namespace codex::api
```

#### 4.2.4 ElevenLabsClient

```cpp
// src/api/ElevenLabsClient.h

#pragma once

#include "ApiClient.h"
#include <QByteArray>

namespace codex::api {

struct VoiceSettings {
    QString voiceId;
    double stability = 0.5;
    double similarityBoost = 0.75;
    double speed = 0.85;  // Plus lent pour narration contemplative
};

class ElevenLabsClient : public ApiClient {
    Q_OBJECT

public:
    explicit ElevenLabsClient(QObject* parent = nullptr);

    // Génère l'audio pour un texte
    void generateSpeech(const QString& text, const VoiceSettings& settings);

    // Liste les voix disponibles
    void fetchAvailableVoices();

signals:
    void speechGenerated(const QByteArray& audioData, int durationMs);
    void voicesListReceived(const QJsonArray& voices);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QString m_modelId = "eleven_multilingual_v2";
};

} // namespace codex::api
```

---

### 4.3 Module DB

#### 4.3.1 Database

```cpp
// src/db/Database.h

#pragma once

#include <QSqlDatabase>
#include <QString>
#include <memory>

namespace codex::db {

class Database {
public:
    static Database& instance();

    bool initialize(const QString& dbPath);
    bool isInitialized() const;

    QSqlDatabase& connection();

    // Migrations
    bool runMigrations();
    int getCurrentVersion();

private:
    Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase m_db;
    bool m_initialized = false;
    QString m_dbPath;

    bool createTables();
};

} // namespace codex::db
```

#### 4.3.2 ProjectRepository

```cpp
// src/db/repositories/ProjectRepository.h

#pragma once

#include <QVector>
#include <optional>
#include "../models/Project.h"

namespace codex::db {

class ProjectRepository {
public:
    ProjectRepository();

    // CRUD
    int create(const core::Project& project);
    std::optional<core::Project> findById(int id);
    QVector<core::Project> findAll();
    bool update(const core::Project& project);
    bool remove(int id);

    // Queries spécifiques
    QVector<core::Project> findRecent(int limit = 10);
    std::optional<core::Project> findByTreatiseCode(const QString& code);
};

} // namespace codex::db
```

---

### 4.4 Module UI

#### 4.4.1 MainWindow

```cpp
// src/ui/MainWindow.h

#pragma once

#include <QMainWindow>
#include <memory>

namespace Ui { class MainWindow; }

namespace codex::ui {

class TextViewerWidget;
class ImageViewerWidget;
class ProjectListWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // Menu actions
    void onOpenFile();
    void onSaveProject();
    void onExportImages();
    void onShowSettings();

    // Workflow actions
    void onPassageSelected(const QString& text, int start, int end);
    void onGenerateImage();
    void onGenerateAudio();
    void onStartSlideshow();

    // API callbacks
    void onEnrichmentCompleted(const QJsonObject& response);
    void onImageGenerated(const QPixmap& image);
    void onAudioGenerated(const QByteArray& audio);
    void onApiError(const QString& error);

private:
    std::unique_ptr<Ui::MainWindow> ui;

    // Widgets
    TextViewerWidget* m_textViewer;
    ImageViewerWidget* m_imageViewer;
    ProjectListWidget* m_projectList;

    // État
    QString m_currentTreatiseCode;
    QString m_selectedPassage;

    void setupConnections();
    void setupMenus();
    void loadSettings();
};

} // namespace codex::ui
```

#### 4.4.2 SlideshowWidget

```cpp
// src/ui/widgets/SlideshowWidget.h

#pragma once

#include <QWidget>
#include <QVector>
#include <QPixmap>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>

namespace codex::ui {

struct SlideshowItem {
    QPixmap image;
    QString audioPath;
    int durationMs;
};

class SlideshowWidget : public QWidget {
    Q_OBJECT

public:
    explicit SlideshowWidget(QWidget* parent = nullptr);

    void setItems(const QVector<SlideshowItem>& items);
    void start();
    void pause();
    void stop();

signals:
    void finished();
    void itemChanged(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onTimerTick();
    void onAudioFinished();

private:
    QVector<SlideshowItem> m_items;
    int m_currentIndex = 0;
    bool m_isPlaying = false;

    QMediaPlayer* m_audioPlayer;
    QAudioOutput* m_audioOutput;
    QTimer* m_transitionTimer;

    // Animation
    QPixmap m_currentPixmap;
    QPixmap m_nextPixmap;
    double m_transitionProgress = 0.0;

    void advanceToNext();
    void renderCurrentFrame();
};

} // namespace codex::ui
```

---

## 5. Flux de Données

### 5.1 Pipeline de Génération

```
┌─────────────────────────────────────────────────────────────────┐
│                    PIPELINE DE GÉNÉRATION                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. SÉLECTION TEXTE                                             │
│     ┌──────────────┐                                            │
│     │ TextViewer   │ ──► passageSelected(text, start, end)      │
│     └──────────────┘                                            │
│            │                                                     │
│            ▼                                                     │
│  2. ANALYSE LOCALE                                              │
│     ┌──────────────┐     ┌──────────────┐                       │
│     │ TextParser   │ ──► │ MythicClass. │                       │
│     │ (entités)    │     │ (catégorie)  │                       │
│     └──────────────┘     └──────────────┘                       │
│            │                    │                                │
│            └────────┬───────────┘                                │
│                     ▼                                            │
│  3. ENRICHISSEMENT LLM                                          │
│     ┌──────────────┐     ┌──────────────┐                       │
│     │PromptBuilder │ ──► │ ClaudeClient │ ──► [Claude API]      │
│     │ (claude req) │     │ (async)      │                       │
│     └──────────────┘     └──────────────┘                       │
│                                │                                 │
│                    enrichmentCompleted(response)                 │
│                                │                                 │
│                                ▼                                 │
│  4. GÉNÉRATION IMAGE                                            │
│     ┌──────────────┐     ┌──────────────┐                       │
│     │PromptBuilder │ ──► │ImagenClient  │ ──► [Imagen API]      │
│     │ (imagen req) │     │ (async)      │                       │
│     └──────────────┘     └──────────────┘                       │
│                                │                                 │
│                    imageGenerated(pixmap)                        │
│                                │                                 │
│                                ▼                                 │
│  5. AFFICHAGE + SAUVEGARDE                                      │
│     ┌──────────────┐     ┌──────────────┐                       │
│     │ ImageViewer  │     │ ImageRepo    │ ──► [SQLite]          │
│     │ (display)    │     │ (persist)    │                       │
│     └──────────────┘     └──────────────┘                       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Séquence Diaporama

```
┌─────────────────────────────────────────────────────────────────┐
│                    SÉQUENCE DIAPORAMA                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  User clique "Start Slideshow"                                  │
│            │                                                     │
│            ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Load passages   │ ◄── ProjectRepository.getPassages()        │
│  │ + images        │ ◄── ImageRepository.getForProject()        │
│  │ + audio         │ ◄── AudioRepository.getForProject()        │
│  └────────┬────────┘                                            │
│           │                                                      │
│           ▼                                                      │
│  ┌─────────────────┐                                            │
│  │ SlideshowWidget │                                            │
│  │ .setItems()     │                                            │
│  │ .start()        │                                            │
│  └────────┬────────┘                                            │
│           │                                                      │
│           ▼                                                      │
│  ┌─────────────────┐     ┌─────────────────┐                    │
│  │ Pour chaque     │     │                 │                    │
│  │ passage:        │ ──► │ Afficher image  │                    │
│  │                 │     │ Jouer audio     │                    │
│  │                 │     │ Attendre durée  │                    │
│  │                 │     │ Transition fade │                    │
│  └─────────────────┘     └─────────────────┘                    │
│           │                                                      │
│           ▼                                                      │
│  ┌─────────────────┐                                            │
│  │ finished()      │                                            │
│  │ Retour UI       │                                            │
│  └─────────────────┘                                            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6. Gestion des Threads

### 6.1 Architecture Threading

```cpp
// Pattern: Worker dans QThread séparé

// Worker pour appels API
class ApiWorker : public QObject {
    Q_OBJECT

public slots:
    void processRequest(const ApiRequest& request) {
        // Exécuté dans thread séparé
        auto result = executeHttpRequest(request);
        emit resultReady(result);
    }

signals:
    void resultReady(const ApiResponse& response);
};

// Utilisation dans MainWindow
void MainWindow::setupApiThreads() {
    // Thread pour Claude API
    m_claudeThread = new QThread(this);
    m_claudeWorker = new ClaudeWorker();
    m_claudeWorker->moveToThread(m_claudeThread);

    connect(m_claudeThread, &QThread::finished,
            m_claudeWorker, &QObject::deleteLater);
    connect(this, &MainWindow::requestEnrichment,
            m_claudeWorker, &ClaudeWorker::process);
    connect(m_claudeWorker, &ClaudeWorker::resultReady,
            this, &MainWindow::onEnrichmentCompleted);

    m_claudeThread->start();
}
```

### 6.2 Diagramme Threading

```
┌─────────────────────────────────────────────────────────────────┐
│                      MAIN THREAD (UI)                            │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  MainWindow   TextViewer   ImageViewer   Slideshow       │   │
│  │      │            │            │             │           │   │
│  │      └────────────┴────────────┴─────────────┘           │   │
│  │                        │                                  │   │
│  │                 signals/slots                             │   │
│  │                        │                                  │   │
│  └────────────────────────┼──────────────────────────────────┘   │
│                           │                                      │
├───────────────────────────┼──────────────────────────────────────┤
│                           │                                      │
│  ┌────────────────────────┴──────────────────────────────────┐   │
│  │                   WORKER THREADS                           │   │
│  │                                                            │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │   │
│  │  │ClaudeWorker │  │ImagenWorker │  │ElevenWorker │        │   │
│  │  │  (Thread 1) │  │  (Thread 2) │  │  (Thread 3) │        │   │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘        │   │
│  │         │                │                │               │   │
│  │         └────────────────┴────────────────┘               │   │
│  │                          │                                │   │
│  │                   Qt Network                              │   │
│  │                          │                                │   │
│  └──────────────────────────┼────────────────────────────────┘   │
│                             │                                    │
└─────────────────────────────┼────────────────────────────────────┘
                              │
                              ▼
                    [APIs Externes HTTPS]
```

---

## 7. Configuration

### 7.1 Fichier de Configuration

```json
// config.json (stocké dans QStandardPaths::AppDataLocation)
{
  "version": "1.0",
  "apis": {
    "claude": {
      "model": "claude-sonnet-4-20250514",
      "max_tokens": 1000,
      "temperature": 0.7
    },
    "imagen": {
      "model": "imagen-3.0-generate-001",
      "aspect_ratio": "16:9",
      "output_format": "png"
    },
    "elevenlabs": {
      "model_id": "eleven_multilingual_v2",
      "voice_id": "",
      "stability": 0.5,
      "similarity_boost": 0.75,
      "speed": 0.85
    }
  },
  "ui": {
    "theme": "dark",
    "language": "fr",
    "slideshow_transition_ms": 500
  },
  "paths": {
    "codex_file": "",
    "output_images": "./output/images",
    "output_audio": "./output/audio"
  }
}
```

### 7.2 Stockage Sécurisé des Clés

```cpp
// src/utils/SecureStorage.h

#pragma once

#include <QString>

namespace codex::utils {

class SecureStorage {
public:
    static SecureStorage& instance();

    // Stocke une clé API de manière sécurisée
    bool storeApiKey(const QString& service, const QString& key);

    // Récupère une clé API
    QString getApiKey(const QString& service);

    // Vérifie si une clé existe
    bool hasApiKey(const QString& service);

    // Supprime une clé
    bool removeApiKey(const QString& service);

private:
    SecureStorage() = default;

#ifdef Q_OS_WIN
    // Windows: DPAPI
    QString encryptWindows(const QString& data);
    QString decryptWindows(const QString& data);
#elif defined(Q_OS_MACOS)
    // macOS: Keychain
    bool storeKeychain(const QString& service, const QString& key);
    QString retrieveKeychain(const QString& service);
#else
    // Linux: libsecret ou fichier chiffré
    QString getSecretServiceKey(const QString& service);
#endif
};

} // namespace codex::utils
```

---

## 8. Build System (CMake)

### 8.1 CMakeLists.txt Principal

```cmake
# CMakeLists.txt

cmake_minimum_required(VERSION 3.25)
project(CodexNagHammadi VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Sql
    Multimedia
)

# nlohmann/json
find_package(nlohmann_json 3.11 REQUIRED)

# Sous-modules
add_subdirectory(src/core)
add_subdirectory(src/api)
add_subdirectory(src/db)
add_subdirectory(src/ui)
add_subdirectory(src/utils)

# Exécutable principal
add_executable(codex-nag-hammadi
    src/main.cpp
    resources/codex-nag-hammadi.qrc
)

target_link_libraries(codex-nag-hammadi PRIVATE
    codex_core
    codex_api
    codex_db
    codex_ui
    codex_utils
    Qt6::Core
    Qt6::Widgets
)

# Tests
enable_testing()
add_subdirectory(tests)

# Installation
install(TARGETS codex-nag-hammadi
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
```

### 8.2 CMakePresets.json

```json
{
  "version": 6,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 25,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "default",
      "hidden": true,
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
      }
    },
    {
      "name": "windows-release",
      "inherits": "default",
      "displayName": "Windows Release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_PREFIX_PATH": "C:/Qt/6.5.3/msvc2019_64"
      }
    },
    {
      "name": "linux-release",
      "inherits": "default",
      "displayName": "Linux Release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    },
    {
      "name": "macos-release",
      "inherits": "default",
      "displayName": "macOS Release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_PREFIX_PATH": "/opt/homebrew/opt/qt@6"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "windows-release",
      "configurePreset": "windows-release"
    },
    {
      "name": "linux-release",
      "configurePreset": "linux-release"
    },
    {
      "name": "macos-release",
      "configurePreset": "macos-release"
    }
  ]
}
```

---

## 9. Tests

### 9.1 Structure des Tests

```cpp
// tests/test_mythic_classifier.cpp

#include <catch2/catch_test_macros.hpp>
#include "core/services/MythicClassifier.h"

using namespace codex::core;

TEST_CASE("MythicClassifier classifies treatises correctly", "[classifier]") {
    MythicClassifier classifier;

    SECTION("Plerome category") {
        REQUIRE(classifier.classifyTreatise("I-5") == MythicCategory::Plerome);
        REQUIRE(classifier.classifyTreatise("III-3") == MythicCategory::Plerome);
    }

    SECTION("Sophia category") {
        REQUIRE(classifier.classifyTreatise("II-4") == MythicCategory::Sophia);
        REQUIRE(classifier.classifyTreatise("II-6") == MythicCategory::Sophia);
    }

    SECTION("Liturgy category") {
        REQUIRE(classifier.classifyTreatise("I-1") == MythicCategory::Liturgy);
    }
}

TEST_CASE("MythicClassifier returns correct styles", "[classifier]") {
    MythicClassifier classifier;

    SECTION("Plerome style has golden palette") {
        auto style = classifier.getStyleForCategory(MythicCategory::Plerome);
        REQUIRE(style.palette.contains(QColor("#FFD700")));
        REQUIRE(style.visualKeywords.contains("infinite light"));
    }
}
```

---

## 10. Prochaines Étapes

**Workflow suivant:** `/bmad:bmm:workflows:create-epics-and-stories`

Les Epics & Stories détailleront :
- Découpage précis des tâches de développement
- Critères d'acceptation détaillés
- Estimation des efforts
- Ordre d'implémentation

---

*Architecture créée le 2026-01-08*
*Basée sur le PRD v1.0*
