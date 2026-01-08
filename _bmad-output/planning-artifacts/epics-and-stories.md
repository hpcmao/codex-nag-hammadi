# Epics & Stories
# Codex Nag Hammadi BD

**Version:** 1.0
**Date:** 2026-01-08
**Auteur:** Hpcmao
**Statut:** Draft

---

## Vue d'Ensemble

### Résumé des Epics

| ID | Epic | Stories | Points | Priorité |
|----|------|---------|--------|----------|
| E0 | Infrastructure & Setup | 4 | 13 | P0 |
| E1 | Gestion des Textes | 4 | 13 | P0 |
| E2 | Pipeline de Génération | 6 | 21 | P0 |
| E3 | Classification Mythique | 3 | 8 | P1 |
| E4 | Narration Audio | 4 | 13 | P1 |
| E5 | Mode Diaporama | 3 | 8 | P1 |
| E6 | Persistance | 4 | 13 | P1 |
| **TOTAL** | | **28** | **89** | |

### Ordre d'Implémentation Recommandé

```
E0 (Setup) → E1 (Textes) → E2 (Pipeline) → E6 (Persistance)
                                    ↓
              E3 (Classification) ← ┘
                      ↓
              E4 (Audio) → E5 (Diaporama)
```

---

## Epic 0: Infrastructure & Setup

**Objectif:** Mettre en place la structure du projet, le build system et les fondations.

**Dépendances:** Aucune

---

### Story E0-S1: Initialisation du projet CMake

**ID:** E0-S1
**Titre:** Créer la structure CMake du projet
**Points:** 3
**Priorité:** P0

**Description:**
En tant que développeur, je veux avoir une structure de projet CMake fonctionnelle afin de pouvoir compiler le projet sur toutes les plateformes cibles.

**Critères d'acceptation:**
- [ ] CMakeLists.txt principal créé avec configuration C++20
- [ ] CMakePresets.json avec presets Windows/Linux/macOS
- [ ] Sous-modules CMake pour core, api, db, ui, utils
- [ ] Build réussi sur au moins une plateforme
- [ ] Qt6 détecté et lié correctement
- [ ] nlohmann/json intégré

**Tâches techniques:**
1. Créer CMakeLists.txt racine
2. Créer CMakePresets.json
3. Créer CMakeLists.txt pour chaque sous-module
4. Configurer find_package pour Qt6
5. Tester le build

**Fichiers à créer:**
- `CMakeLists.txt`
- `CMakePresets.json`
- `src/core/CMakeLists.txt`
- `src/api/CMakeLists.txt`
- `src/db/CMakeLists.txt`
- `src/ui/CMakeLists.txt`
- `src/utils/CMakeLists.txt`

---

### Story E0-S2: Configuration et Logging

**ID:** E0-S2
**Titre:** Implémenter la gestion de configuration et logging
**Points:** 3
**Priorité:** P0

**Description:**
En tant que développeur, je veux avoir un système de configuration et de logging afin de gérer les paramètres de l'application et tracer les erreurs.

**Critères d'acceptation:**
- [ ] Classe Config singleton implémentée
- [ ] Lecture/écriture fichier config.json
- [ ] Classe Logger avec niveaux (DEBUG, INFO, WARN, ERROR)
- [ ] Logs écrits dans fichier + console
- [ ] Chemin config utilise QStandardPaths

**Tâches techniques:**
1. Implémenter Config.h/.cpp
2. Implémenter Logger.h/.cpp
3. Créer config.json par défaut
4. Tests unitaires

**Fichiers à créer:**
- `src/utils/Config.h`
- `src/utils/Config.cpp`
- `src/utils/Logger.h`
- `src/utils/Logger.cpp`

---

### Story E0-S3: Stockage sécurisé des clés API

**ID:** E0-S3
**Titre:** Implémenter le stockage sécurisé des clés API
**Points:** 5
**Priorité:** P0

**Description:**
En tant qu'utilisateur, je veux que mes clés API soient stockées de manière sécurisée afin de protéger mes credentials.

**Critères d'acceptation:**
- [ ] Windows: utilisation DPAPI
- [ ] macOS: utilisation Keychain
- [ ] Linux: utilisation libsecret ou fichier chiffré
- [ ] Interface unifiée SecureStorage
- [ ] Clés jamais en clair sur disque

**Tâches techniques:**
1. Implémenter SecureStorage.h/.cpp
2. Implémenter backend Windows (DPAPI)
3. Implémenter backend macOS (Keychain)
4. Implémenter backend Linux (libsecret)
5. Tests sur chaque plateforme

**Fichiers à créer:**
- `src/utils/SecureStorage.h`
- `src/utils/SecureStorage.cpp`

---

### Story E0-S4: Fenêtre principale vide

**ID:** E0-S4
**Titre:** Créer la fenêtre principale Qt
**Points:** 2
**Priorité:** P0

**Description:**
En tant qu'utilisateur, je veux voir une fenêtre d'application avec menu et zones de contenu afin d'avoir l'interface de base.

**Critères d'acceptation:**
- [ ] MainWindow avec layout principal (splitters)
- [ ] Menu Fichier (Ouvrir, Sauvegarder, Quitter)
- [ ] Menu Édition (Paramètres)
- [ ] Menu Aide (À propos)
- [ ] Barre de status
- [ ] Thème sombre appliqué

**Tâches techniques:**
1. Créer MainWindow.h/.cpp/.ui
2. Créer fichier QSS thème sombre
3. Implémenter structure layout
4. Connecter actions menu de base

**Fichiers à créer:**
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/ui/MainWindow.ui`
- `src/ui/styles/dark_theme.qss`
- `src/main.cpp`

---

## Epic 1: Gestion des Textes

**Objectif:** Charger, afficher et permettre la sélection de passages dans les traités du Codex.

**Dépendances:** E0

---

### Story E1-S1: Parser les fichiers Markdown

**ID:** E1-S1
**Titre:** Implémenter le parsing des fichiers Codex Markdown
**Points:** 5
**Priorité:** P0

**Description:**
En tant que système, je veux parser les fichiers Markdown du Codex afin d'extraire la structure des traités (code, titre, pages, contenu).

**Critères d'acceptation:**
- [ ] Lecture fichiers .md UTF-8
- [ ] Extraction code traité (I-1, II-3, etc.)
- [ ] Extraction titre du traité
- [ ] Découpage par pages
- [ ] Liste de 52+ traités parsés
- [ ] Gestion erreurs (fichier invalide)

**Tâches techniques:**
1. Créer struct ParsedTreatise
2. Implémenter TextParser::parseCodexFile()
3. Regex/parsing pour structure Codex
4. Tests unitaires avec fichier réel

**Fichiers à créer:**
- `src/core/services/TextParser.h`
- `src/core/services/TextParser.cpp`
- `tests/test_text_parser.cpp`

---

### Story E1-S2: Widget affichage texte

**ID:** E1-S2
**Titre:** Créer le widget d'affichage des textes
**Points:** 3
**Priorité:** P0

**Description:**
En tant qu'utilisateur, je veux voir le contenu d'un traité dans un panneau dédié afin de lire et sélectionner des passages.

**Critères d'acceptation:**
- [ ] QTextEdit/QPlainTextEdit avec contenu
- [ ] Mise en forme basique (titres, paragraphes)
- [ ] Scroll fluide
- [ ] Police lisible, taille configurable
- [ ] Numéros de page visibles

**Tâches techniques:**
1. Créer TextViewerWidget.h/.cpp
2. Méthode setTreatise(ParsedTreatise)
3. Styling du texte
4. Intégration dans MainWindow

**Fichiers à créer:**
- `src/ui/widgets/TextViewerWidget.h`
- `src/ui/widgets/TextViewerWidget.cpp`

---

### Story E1-S3: Liste des traités

**ID:** E1-S3
**Titre:** Créer la liste de sélection des traités
**Points:** 2
**Priorité:** P0

**Description:**
En tant qu'utilisateur, je veux voir la liste des 52 traités dans un panneau latéral afin de choisir lequel visualiser.

**Critères d'acceptation:**
- [ ] QListWidget ou QTreeWidget avec traités
- [ ] Affichage: code + titre
- [ ] Double-clic charge le traité
- [ ] Recherche/filtre par nom
- [ ] Indication traité sélectionné

**Tâches techniques:**
1. Créer TreatiseListWidget.h/.cpp
2. Signal treatiseSelected(QString code)
3. Méthode de recherche/filtre
4. Intégration dans MainWindow

**Fichiers à créer:**
- `src/ui/widgets/TreatiseListWidget.h`
- `src/ui/widgets/TreatiseListWidget.cpp`

---

### Story E1-S4: Sélection de passage

**ID:** E1-S4
**Titre:** Permettre la sélection d'un passage de texte
**Points:** 3
**Priorité:** P0

**Description:**
En tant qu'utilisateur, je veux sélectionner un passage de texte afin de le transformer en image.

**Critères d'acceptation:**
- [ ] Sélection par clic-glisser dans TextViewer
- [ ] Bouton "Sélectionner passage" dédié
- [ ] Affichage du texte sélectionné dans zone preview
- [ ] Compteur caractères/mots
- [ ] Signal passageSelected(text, start, end)

**Tâches techniques:**
1. Implémenter sélection dans TextViewerWidget
2. Créer PassagePreviewWidget
3. Connecter signals
4. Validation longueur passage

**Fichiers à modifier:**
- `src/ui/widgets/TextViewerWidget.cpp`

**Fichiers à créer:**
- `src/ui/widgets/PassagePreviewWidget.h`
- `src/ui/widgets/PassagePreviewWidget.cpp`

---

## Epic 2: Pipeline de Génération

**Objectif:** Transformer un passage texte en image via le pipeline Claude → Imagen.

**Dépendances:** E0, E1

---

### Story E2-S1: Client API base

**ID:** E2-S1
**Titre:** Implémenter la classe ApiClient de base
**Points:** 3
**Priorité:** P0

**Description:**
En tant que développeur, je veux une classe de base pour les appels API afin de factoriser le code HTTP commun.

**Critères d'acceptation:**
- [ ] Classe ApiClient abstraite
- [ ] QNetworkAccessManager intégré
- [ ] Gestion headers (Authorization, Content-Type)
- [ ] Signals: requestStarted, requestFinished, errorOccurred
- [ ] Timeout configurable

**Tâches techniques:**
1. Créer ApiClient.h/.cpp
2. Méthodes createRequest(), handleError()
3. Gestion clé API depuis SecureStorage

**Fichiers à créer:**
- `src/api/ApiClient.h`
- `src/api/ApiClient.cpp`

---

### Story E2-S2: Client Claude API

**ID:** E2-S2
**Titre:** Implémenter le client Claude API
**Points:** 5
**Priorité:** P0

**Description:**
En tant que système, je veux appeler Claude API afin d'enrichir les passages avec contexte, émotion et composition.

**Critères d'acceptation:**
- [ ] Hérite de ApiClient
- [ ] Méthode enrichPassage(prompt) asynchrone
- [ ] Parsing réponse JSON
- [ ] Signal enrichmentCompleted(QJsonObject)
- [ ] Gestion erreurs API (rate limit, auth)
- [ ] Worker thread pour non-blocage UI

**Tâches techniques:**
1. Créer ClaudeClient.h/.cpp
2. Implémenter enrichPassage()
3. Créer ClaudeWorker pour threading
4. Tests avec mock ou API réelle

**Fichiers à créer:**
- `src/api/ClaudeClient.h`
- `src/api/ClaudeClient.cpp`

---

### Story E2-S3: Client Imagen API

**ID:** E2-S3
**Titre:** Implémenter le client Imagen 3 API
**Points:** 5
**Priorité:** P0

**Description:**
En tant que système, je veux appeler Imagen 3 API afin de générer des images à partir des prompts.

**Critères d'acceptation:**
- [ ] Hérite de ApiClient
- [ ] Méthode generateImage(params) asynchrone
- [ ] Support aspect ratio 16:9
- [ ] Décodage image base64 → QPixmap
- [ ] Signal imageGenerated(QPixmap, prompt)
- [ ] Worker thread pour non-blocage UI

**Tâches techniques:**
1. Créer ImagenClient.h/.cpp
2. Implémenter generateImage()
3. Créer ImagenWorker pour threading
4. Gestion format réponse Google Cloud

**Fichiers à créer:**
- `src/api/ImagenClient.h`
- `src/api/ImagenClient.cpp`

---

### Story E2-S4: Constructeur de prompts

**ID:** E2-S4
**Titre:** Implémenter le PromptBuilder
**Points:** 3
**Priorité:** P0

**Description:**
En tant que système, je veux construire des prompts optimisés afin d'obtenir des images style Villeneuve.

**Critères d'acceptation:**
- [ ] Méthode buildClaudePrompt() pour enrichissement
- [ ] Méthode buildImagenPrompt() avec template Villeneuve
- [ ] Injection entités gnostiques détectées
- [ ] Injection palette/keywords de catégorie
- [ ] Template configurable

**Tâches techniques:**
1. Créer PromptBuilder.h/.cpp
2. Charger templates depuis JSON
3. Méthodes de composition prompt
4. Tests unitaires

**Fichiers à créer:**
- `src/core/services/PromptBuilder.h`
- `src/core/services/PromptBuilder.cpp`
- `resources/data/style_templates.json`

---

### Story E2-S5: Widget affichage image

**ID:** E2-S5
**Titre:** Créer le widget d'affichage des images générées
**Points:** 2
**Priorité:** P0

**Description:**
En tant qu'utilisateur, je veux voir l'image générée dans un panneau dédié afin de visualiser le résultat.

**Critères d'acceptation:**
- [ ] QLabel ou QGraphicsView pour image
- [ ] Zoom in/out
- [ ] Fit to window
- [ ] Placeholder quand pas d'image
- [ ] Indicateur de chargement

**Tâches techniques:**
1. Créer ImageViewerWidget.h/.cpp
2. Méthodes setImage(), showLoading()
3. Gestion zoom
4. Intégration dans MainWindow

**Fichiers à créer:**
- `src/ui/widgets/ImageViewerWidget.h`
- `src/ui/widgets/ImageViewerWidget.cpp`

---

### Story E2-S6: Orchestration pipeline complet

**ID:** E2-S6
**Titre:** Orchestrer le pipeline complet de génération
**Points:** 3
**Priorité:** P0

**Description:**
En tant qu'utilisateur, je veux cliquer sur "Générer" et voir le pipeline s'exécuter automatiquement afin d'obtenir une image sans intervention manuelle.

**Critères d'acceptation:**
- [ ] Bouton "Générer Image" actif après sélection passage
- [ ] Séquence: Analyse → Claude → Imagen → Affichage
- [ ] Progress bar ou status updates
- [ ] Gestion erreurs à chaque étape
- [ ] Annulation possible

**Tâches techniques:**
1. Créer PipelineController.h/.cpp
2. Machine à états pour le pipeline
3. Connexion signals entre étapes
4. UI feedback (progress, status)

**Fichiers à créer:**
- `src/core/controllers/PipelineController.h`
- `src/core/controllers/PipelineController.cpp`

---

## Epic 3: Classification Mythique

**Objectif:** Classifier automatiquement les traités et appliquer les palettes visuelles appropriées.

**Dépendances:** E2

---

### Story E3-S1: Dictionnaire entités gnostiques

**ID:** E3-S1
**Titre:** Créer le dictionnaire des entités gnostiques
**Points:** 3
**Priorité:** P1

**Description:**
En tant que système, je veux avoir un dictionnaire d'entités gnostiques afin de les détecter dans les textes.

**Critères d'acceptation:**
- [ ] Fichier JSON avec entités (Plérôme, Sophia, Éons, etc.)
- [ ] Keywords associés à chaque entité
- [ ] Visual keywords pour prompts
- [ ] Classe GnosticEntities pour accès
- [ ] Au moins 20 entités définies

**Tâches techniques:**
1. Créer gnostic_entities.json
2. Créer GnosticEntities.h/.cpp
3. Méthode detect(text) → liste entités
4. Tests unitaires

**Fichiers à créer:**
- `resources/data/gnostic_entities.json`
- `src/core/entities/GnosticEntities.h`
- `src/core/entities/GnosticEntities.cpp`

---

### Story E3-S2: Classifier mythique

**ID:** E3-S2
**Titre:** Implémenter le classificateur mythique
**Points:** 3
**Priorité:** P1

**Description:**
En tant que système, je veux classifier chaque traité dans une catégorie mythique afin d'appliquer le style visuel approprié.

**Critères d'acceptation:**
- [ ] 9 catégories définies
- [ ] Mapping traité → catégorie (52 traités)
- [ ] Style par catégorie (palette, keywords, lighting)
- [ ] Override manuel possible
- [ ] Fichier JSON de configuration

**Tâches techniques:**
1. Créer mythic_categories.json
2. Créer MythicClassifier.h/.cpp
3. Enum MythicCategory
4. Tests unitaires

**Fichiers à créer:**
- `resources/data/mythic_categories.json`
- `src/core/services/MythicClassifier.h`
- `src/core/services/MythicClassifier.cpp`
- `tests/test_mythic_classifier.cpp`

---

### Story E3-S3: Intégration classification dans pipeline

**ID:** E3-S3
**Titre:** Intégrer la classification dans le pipeline de génération
**Points:** 2
**Priorité:** P1

**Description:**
En tant que système, je veux que la classification soit automatiquement appliquée lors de la génération afin d'avoir des images cohérentes avec la catégorie.

**Critères d'acceptation:**
- [ ] Détection catégorie au chargement traité
- [ ] Affichage catégorie dans UI
- [ ] Injection palette dans PromptBuilder
- [ ] Injection keywords dans prompt Imagen
- [ ] Option override dans UI

**Tâches techniques:**
1. Modifier PipelineController
2. Modifier PromptBuilder.buildImagenPrompt()
3. Ajouter indicateur catégorie dans MainWindow
4. Ajouter combo override catégorie

**Fichiers à modifier:**
- `src/core/controllers/PipelineController.cpp`
- `src/core/services/PromptBuilder.cpp`
- `src/ui/MainWindow.cpp`

---

## Epic 4: Narration Audio

**Objectif:** Générer et jouer la narration audio des passages.

**Dépendances:** E2

---

### Story E4-S1: Client ElevenLabs API

**ID:** E4-S1
**Titre:** Implémenter le client ElevenLabs API
**Points:** 5
**Priorité:** P1

**Description:**
En tant que système, je veux appeler ElevenLabs API afin de générer la narration audio des passages.

**Critères d'acceptation:**
- [ ] Hérite de ApiClient
- [ ] Méthode generateSpeech(text, settings) asynchrone
- [ ] Support voix FR masculine
- [ ] Paramètres: stabilité, vitesse, similarité
- [ ] Signal speechGenerated(audioData, durationMs)
- [ ] Sauvegarde fichier audio local

**Tâches techniques:**
1. Créer ElevenLabsClient.h/.cpp
2. Implémenter generateSpeech()
3. Créer ElevenLabsWorker pour threading
4. Gestion format audio (MP3)

**Fichiers à créer:**
- `src/api/ElevenLabsClient.h`
- `src/api/ElevenLabsClient.cpp`

---

### Story E4-S2: Lecteur audio

**ID:** E4-S2
**Titre:** Implémenter le lecteur audio intégré
**Points:** 3
**Priorité:** P1

**Description:**
En tant qu'utilisateur, je veux écouter la narration générée afin d'avoir l'expérience audio.

**Critères d'acceptation:**
- [ ] QMediaPlayer pour lecture audio
- [ ] Contrôles: Play, Pause, Stop
- [ ] Barre de progression
- [ ] Volume ajustable
- [ ] Signal audioFinished()

**Tâches techniques:**
1. Créer AudioPlayerWidget.h/.cpp
2. Intégration QMediaPlayer + QAudioOutput
3. Contrôles UI
4. Intégration dans MainWindow

**Fichiers à créer:**
- `src/ui/widgets/AudioPlayerWidget.h`
- `src/ui/widgets/AudioPlayerWidget.cpp`

---

### Story E4-S3: Sélection voix ElevenLabs

**ID:** E4-S3
**Titre:** Permettre la sélection de la voix
**Points:** 2
**Priorité:** P1

**Description:**
En tant qu'utilisateur, je veux choisir la voix de narration afin de personnaliser l'expérience.

**Critères d'acceptation:**
- [ ] Récupération liste voix disponibles
- [ ] Dropdown de sélection voix
- [ ] Preview voix (sample)
- [ ] Sauvegarde voix préférée en config

**Tâches techniques:**
1. Implémenter ElevenLabsClient::fetchVoices()
2. Ajouter dropdown dans SettingsDialog
3. Stocker voiceId en config

**Fichiers à modifier:**
- `src/api/ElevenLabsClient.cpp`
- `src/ui/dialogs/SettingsDialog.cpp`

---

### Story E4-S4: Bouton génération audio

**ID:** E4-S4
**Titre:** Ajouter bouton de génération audio
**Points:** 3
**Priorité:** P1

**Description:**
En tant qu'utilisateur, je veux un bouton pour générer l'audio du passage sélectionné afin de lancer la narration.

**Critères d'acceptation:**
- [ ] Bouton "Générer Audio" dans toolbar
- [ ] Actif après sélection passage
- [ ] Progress indicator pendant génération
- [ ] Lecture auto après génération (optionnel)
- [ ] Sauvegarde fichier audio

**Tâches techniques:**
1. Ajouter bouton dans MainWindow toolbar
2. Connecter à PipelineController
3. Gérer état bouton (enabled/disabled)
4. Feedback utilisateur

**Fichiers à modifier:**
- `src/ui/MainWindow.cpp`
- `src/core/controllers/PipelineController.cpp`

---

## Epic 5: Mode Diaporama

**Objectif:** Afficher une séquence d'images avec audio synchronisé en mode plein écran.

**Dépendances:** E2, E4, E6

---

### Story E5-S1: Widget diaporama

**ID:** E5-S1
**Titre:** Créer le widget de diaporama plein écran
**Points:** 3
**Priorité:** P1

**Description:**
En tant qu'utilisateur, je veux voir les images en plein écran avec transitions fluides afin d'avoir une expérience immersive.

**Critères d'acceptation:**
- [ ] Mode plein écran (F11 ou bouton)
- [ ] Affichage image centré, fond noir
- [ ] Transition fade entre images
- [ ] Durée configurable par slide
- [ ] Touche Échap pour quitter

**Tâches techniques:**
1. Créer SlideshowWidget.h/.cpp
2. Implémenter paintEvent pour transitions
3. Gestion fullscreen
4. Timer pour avancement auto

**Fichiers à créer:**
- `src/ui/widgets/SlideshowWidget.h`
- `src/ui/widgets/SlideshowWidget.cpp`

---

### Story E5-S2: Synchronisation audio/images

**ID:** E5-S2
**Titre:** Synchroniser l'audio avec le défilement des images
**Points:** 3
**Priorité:** P1

**Description:**
En tant qu'utilisateur, je veux que l'audio soit synchronisé avec les images afin d'avoir une expérience cohérente.

**Critères d'acceptation:**
- [ ] Calcul durée audio par passage
- [ ] Image affichée pendant durée audio correspondante
- [ ] Transition à la fin de chaque audio
- [ ] Silence optionnel entre passages

**Tâches techniques:**
1. Modifier SlideshowWidget pour intégrer QMediaPlayer
2. Logique de synchronisation
3. Gestion des événements audio (finished)
4. Tests manuels

**Fichiers à modifier:**
- `src/ui/widgets/SlideshowWidget.cpp`

---

### Story E5-S3: Contrôles diaporama

**ID:** E5-S3
**Titre:** Ajouter les contrôles du diaporama
**Points:** 2
**Priorité:** P1

**Description:**
En tant qu'utilisateur, je veux pouvoir contrôler le diaporama (pause, suivant, précédent) afin de naviguer à mon rythme.

**Critères d'acceptation:**
- [ ] Overlay contrôles (apparaît au mouvement souris)
- [ ] Boutons: Précédent, Play/Pause, Suivant
- [ ] Barre de progression globale
- [ ] Raccourcis clavier (Espace, Flèches)
- [ ] Auto-hide après 3 secondes

**Tâches techniques:**
1. Créer overlay contrôles dans SlideshowWidget
2. Implémenter keyPressEvent
3. Timer pour auto-hide
4. Styling overlay semi-transparent

**Fichiers à modifier:**
- `src/ui/widgets/SlideshowWidget.cpp`

---

## Epic 6: Persistance

**Objectif:** Sauvegarder et charger les projets, images et audio générés.

**Dépendances:** E0

---

### Story E6-S1: Base de données SQLite

**ID:** E6-S1
**Titre:** Initialiser la base de données SQLite
**Points:** 3
**Priorité:** P1

**Description:**
En tant que système, je veux une base SQLite initialisée afin de stocker les données des projets.

**Critères d'acceptation:**
- [ ] Classe Database singleton
- [ ] Fichier .db créé au premier lancement
- [ ] Tables créées (projects, passages, images, audio)
- [ ] Système de migrations
- [ ] Chemin dans QStandardPaths::AppDataLocation

**Tâches techniques:**
1. Créer Database.h/.cpp
2. Créer script SQL 001_initial.sql
3. Implémenter runMigrations()
4. Tests création tables

**Fichiers à créer:**
- `src/db/Database.h`
- `src/db/Database.cpp`
- `src/db/migrations/001_initial.sql`

---

### Story E6-S2: Repository Project

**ID:** E6-S2
**Titre:** Implémenter le repository des projets
**Points:** 3
**Priorité:** P1

**Description:**
En tant que système, je veux un repository pour les projets afin de faire le CRUD.

**Critères d'acceptation:**
- [ ] Méthodes: create, findById, findAll, update, delete
- [ ] Méthode findRecent(limit)
- [ ] Struct Project avec tous les champs
- [ ] Gestion transactions

**Tâches techniques:**
1. Créer modèle Project.h/.cpp
2. Créer ProjectRepository.h/.cpp
3. Requêtes SQL paramétrées
4. Tests unitaires

**Fichiers à créer:**
- `src/core/models/Project.h`
- `src/core/models/Project.cpp`
- `src/db/repositories/ProjectRepository.h`
- `src/db/repositories/ProjectRepository.cpp`

---

### Story E6-S3: Repository Passages, Images, Audio

**ID:** E6-S3
**Titre:** Implémenter les repositories secondaires
**Points:** 5
**Priorité:** P1

**Description:**
En tant que système, je veux des repositories pour passages, images et audio afin de persister tous les éléments générés.

**Critères d'acceptation:**
- [ ] PassageRepository avec CRUD + findByProject
- [ ] ImageRepository avec CRUD + findByPassage
- [ ] AudioRepository avec CRUD + findByPassage
- [ ] Relations FK respectées

**Tâches techniques:**
1. Créer modèles Passage, GeneratedImage, AudioFile
2. Créer les 3 repositories
3. Tests unitaires

**Fichiers à créer:**
- `src/core/models/Passage.h/.cpp`
- `src/core/models/GeneratedImage.h/.cpp`
- `src/core/models/AudioFile.h/.cpp`
- `src/db/repositories/PassageRepository.h/.cpp`
- `src/db/repositories/ImageRepository.h/.cpp`
- `src/db/repositories/AudioRepository.h/.cpp`

---

### Story E6-S4: UI gestion projets

**ID:** E6-S4
**Titre:** Interface de gestion des projets
**Points:** 2
**Priorité:** P1

**Description:**
En tant qu'utilisateur, je veux voir mes projets récents et en créer de nouveaux afin de gérer mon travail.

**Critères d'acceptation:**
- [ ] Liste projets récents au démarrage
- [ ] Menu Fichier > Nouveau projet
- [ ] Menu Fichier > Ouvrir projet
- [ ] Sauvegarde automatique
- [ ] Nom projet éditable

**Tâches techniques:**
1. Créer ProjectListWidget ou dialog
2. Connecter à ProjectRepository
3. Implémenter sauvegarde auto (timer ou on change)
4. Menu actions

**Fichiers à créer:**
- `src/ui/widgets/ProjectListWidget.h`
- `src/ui/widgets/ProjectListWidget.cpp`

**Fichiers à modifier:**
- `src/ui/MainWindow.cpp`

---

## Récapitulatif des Dépendances

```
E0-S1 (CMake)
    │
    ├── E0-S2 (Config/Logger)
    │       │
    │       └── E0-S3 (SecureStorage)
    │
    └── E0-S4 (MainWindow)
            │
            ├── E1-S1 (TextParser)
            │       │
            │       ├── E1-S2 (TextViewer)
            │       ├── E1-S3 (TreatiseList)
            │       └── E1-S4 (PassageSelection)
            │               │
            │               └── E2-S1 (ApiClient base)
            │                       │
            │                       ├── E2-S2 (ClaudeClient)
            │                       ├── E2-S3 (ImagenClient)
            │                       └── E4-S1 (ElevenLabsClient)
            │
            ├── E2-S4 (PromptBuilder)
            │       │
            │       └── E3-S1 (GnosticEntities)
            │               │
            │               └── E3-S2 (MythicClassifier)
            │
            ├── E2-S5 (ImageViewer)
            │
            └── E6-S1 (Database)
                    │
                    ├── E6-S2 (ProjectRepo)
                    └── E6-S3 (PassageRepo, ImageRepo, AudioRepo)

E2-S6 (Pipeline) ← E2-S2, E2-S3, E2-S4, E2-S5
E3-S3 (Classification integration) ← E3-S2, E2-S6
E4-S2 (AudioPlayer) ← E4-S1
E4-S4 (AudioButton) ← E4-S2
E5-S1 (Slideshow) ← E2-S5
E5-S2 (AudioSync) ← E5-S1, E4-S2
E5-S3 (SlideshowControls) ← E5-S2
E6-S4 (ProjectUI) ← E6-S2, E6-S3
```

---

## Sprint Suggéré (MVP)

### Sprint 1: Fondations
- E0-S1: CMake setup
- E0-S2: Config/Logger
- E0-S3: SecureStorage
- E0-S4: MainWindow

### Sprint 2: Textes
- E1-S1: TextParser
- E1-S2: TextViewer
- E1-S3: TreatiseList
- E1-S4: PassageSelection

### Sprint 3: Pipeline Core
- E2-S1: ApiClient
- E2-S2: ClaudeClient
- E2-S3: ImagenClient
- E2-S4: PromptBuilder

### Sprint 4: Pipeline UI
- E2-S5: ImageViewer
- E2-S6: Pipeline orchestration
- E6-S1: Database

### Sprint 5: Classification + Persistance
- E3-S1: GnosticEntities
- E3-S2: MythicClassifier
- E3-S3: Classification integration
- E6-S2: ProjectRepo
- E6-S3: Other repos

### Sprint 6: Audio
- E4-S1: ElevenLabsClient
- E4-S2: AudioPlayer
- E4-S3: Voice selection
- E4-S4: Audio button

### Sprint 7: Diaporama + Polish
- E5-S1: SlideshowWidget
- E5-S2: AudioSync
- E5-S3: Controls
- E6-S4: ProjectUI

---

*Epics & Stories créés le 2026-01-08*
*Basés sur le PRD v1.0 et l'Architecture v1.0*
