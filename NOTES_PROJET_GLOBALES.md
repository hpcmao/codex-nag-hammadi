# NOTES PROJET GLOBALES



---

# Source: 0_Etat-Projet_08-01-2026_00h37.md

---

# État du Projet - Codex Nag Hammadi BD

**Date:** 08-01-2026 00h37
**Phase:** Post-Brainstorming / Pré-Prototypage

---

## Résumé du Projet

**Objectif:** Créer une BD photoréaliste immersive des textes gnostiques de Nag Hammadi en utilisant l'IA générative.

**Vision:** Expérience immersive des textes anciens avec:
- Visuel mystique (style Villeneuve)
- Narration sacrée (voix grave, contemplative)
- Ambiance sonore

---

## Décisions Architecturales Validées

### Style Visuel
- **Direction:** Denis Villeneuve - Photoréalisme épique
- **Références:** Dune, Arrival, Blade Runner 2049
- **Palette:** Désaturée, ocres/gris/dorés sourds
- **Caractéristiques:** Monumentalité, lumière dramatique, échelle cosmique

### Narration Audio
- **Voix:** Masculine, grave, lente, contemplative
- **Technologie:** ElevenLabs (remplace Edge TTS)

### Pipeline Texte → Image
```
Texte → Pré-analyse Python (entités gnostiques) → Claude API (contexte/émotion) → Template Villeneuve → Imagen 3
```

### Classification Mythique (52 traités)
| Catégorie | Traités | Palette |
|-----------|---------|---------|
| Plérôme | 7 | Or/blanc/bleu |
| Sophia | 7 | Bleu/argent |
| Démiurge | 7 | Rouge/noir |
| Gnose | 12 | Ocre→or |
| Ascension | 8 | Or/transparent |
| Liturgie | 3 | Variable |
| Hermétique | 6 | Neutre |
| Narratif | 4 | Terrestre |
| Fragments | 2 | - |

### Budget Estimé
| Service | Coût/mois |
|---------|-----------|
| Imagen 3 | ~$6 |
| Veo 3.1 Fast | ~$30 |
| ElevenLabs | ~$10 |
| Claude API | ~$5 |
| **TOTAL** | **~$50** |

---

## Fichiers Clés

| Fichier | Description |
|---------|-------------|
| `codex-nag-hammadi.md` | Textes source (1.3 MB, 52+ traités) |
| `Gnose Nag Hammadi.md` | Format alternatif (1.1 MB) |
| `_bmad-output/analysis/brainstorming-session-2026-01-07.md` | Session brainstorming complète |

---

## Questions Ouvertes

### Priorité Haute
1. Cohérence personnages - Même visage ou variation mystique?
2. Découpage textes - Manuel ou automatique?
3. Tous les 52 traités ou sélection?

### Priorité Moyenne
4. Ordre de lecture?
5. Nombre d'images par page?
6. Ambiance sonore?

---

## Prochaines Étapes

### Phase 1: Prototype
- [ ] Choisir 1 traité court (Prière Apôtre Paul recommandé)
- [ ] Créer dictionnaire entités gnostiques (JSON)
- [ ] Tester pipeline sur 3-5 passages
- [ ] Valider style Villeneuve avec Imagen 3

### Phase 2: Audio
- [ ] Configurer ElevenLabs
- [ ] Tester voix FR masculine
- [ ] Synchronisation audio/images

### Phase 3: Classification
- [ ] Implémenter détection catégorie mythique
- [ ] Mapper palettes automatiques

### Phase 4: Interface
- [ ] GUI sélection texte
- [ ] Prévisualisation
- [ ] Mode diaporama

---

## Historique des Sessions

| Date | Type | Résultat |
|------|------|----------|
| 2026-01-07 | Brainstorming BMAD | 6 décisions, 12 questions, classification complète |

---

---

## Prochaine Session

**Workflow choisi:** BMM Complet (Option A)

```
Product Brief → PRD → Architecture → Epics/Stories → Dev
```

**À faire:**
1. Lancer `/create-product-brief` ou `/bmad-master`
2. Transformer les décisions du brainstorming en Product Brief
3. Puis créer le PRD

**Commande suggérée pour reprendre:**
```
/bmad:bmm:workflows:create-product-brief
```

---

*Dernière mise à jour: 08-01-2026 00h37*


---

# Source: 0_Etat-Projet_08-01-2026_12h30.md

---

# État du Projet - Codex Nag Hammadi BD

**Date:** 08-01-2026 12h30
**Phase:** Planification Complète / Prêt pour Développement

---

## Résumé du Projet

**Objectif:** Créer une BD photoréaliste immersive des textes gnostiques de Nag Hammadi en utilisant l'IA générative.

**Vision:** Expérience immersive des textes anciens avec:
- Visuel mystique style Denis Villeneuve
- Narration sacrée (voix grave, contemplative) via ElevenLabs
- Ambiance sonore

---

## Stack Technique Validé

| Composant | Technologie |
|-----------|-------------|
| Langage | C++20 |
| Framework GUI | Qt 6.5+ |
| Base de données | SQLite |
| Build | CMake + Presets |
| APIs | Claude, Imagen 3, ElevenLabs |
| Pattern | Signals/Slots Qt + QThread Workers |

---

## Documents de Planification Créés

| Document | Fichier | Statut |
|----------|---------|--------|
| Product Brief | `_bmad-output/planning-artifacts/product-brief.md` | Validé |
| PRD | `_bmad-output/planning-artifacts/prd.md` | Validé |
| Architecture | `_bmad-output/planning-artifacts/architecture.md` | Validé |
| Epics & Stories | `_bmad-output/planning-artifacts/epics-and-stories.md` | Validé |

---

## Epics Planifiés

| ID | Epic | Stories | Points |
|----|------|---------|--------|
| E0 | Infrastructure & Setup | 4 | 13 |
| E1 | Gestion des Textes | 4 | 13 |
| E2 | Pipeline de Génération | 6 | 21 |
| E3 | Classification Mythique | 3 | 8 |
| E4 | Narration Audio | 4 | 13 |
| E5 | Mode Diaporama | 3 | 8 |
| E6 | Persistance | 4 | 13 |
| **TOTAL** | | **28** | **89** |

---

## Budget API Estimé

| Service | Coût/mois |
|---------|-----------|
| Imagen 3 | ~$6 |
| Veo 3.1 Fast | ~$30 |
| ElevenLabs | ~$10 |
| Claude API | ~$5 |
| **TOTAL** | **~$50** |

---

## Prochaines Étapes

### Option A: Lancer le Sprint Planning
```
/bmad:bmm:workflows:sprint-planning
```
Crée un fichier sprint-status.yaml pour tracker l'avancement.

### Option B: Commencer directement à coder
```
/bmad:bmm:workflows:dev-story
```
Développer la première story (E0-S1: CMake setup).

### Option C: Vérification pré-développement
```
/bmad:bmm:workflows:check-implementation-readiness
```
Vérifier que tous les documents sont cohérents avant de coder.

---

## Fichiers Sources

| Fichier | Description |
|---------|-------------|
| `codex-nag-hammadi.md` | Textes source (1.3 MB, 52 traités) |
| `Gnose Nag Hammadi.md` | Format alternatif (1.1 MB) |

---

## Historique des Sessions

| Date | Type | Résultat |
|------|------|----------|
| 2026-01-07 | Brainstorming BMAD | Décisions architecturales |
| 2026-01-08 | Workflow BMM | Product Brief, PRD, Architecture, Epics/Stories |

---

*Dernière mise à jour: 08-01-2026 12h30*


---

# Source: _Etat-Projet_08-01-2026_23h03.md

---

# Etat du Projet Codex Nag Hammadi BD
**Date:** 08 janvier 2026 - 23h03
**Commit:** a323b92 - feat: Add theme system, Edge TTS neural voices, and UI improvements

---

## Resume du Projet

Application Qt6/C++ pour generer des bandes dessinees photorealistes a partir des textes gnostiques de Nag Hammadi, dans le style visuel de Denis Villeneuve.

## Fonctionnalites Implementees

### Interface Utilisateur
- **MainWindow** avec splitters: Liste traites | Viewer texte + Preview | Image + Audio
- **TreatiseListWidget**: Navigation par codex avec 49 traites classes par categorie
- **TextViewerWidget**: Affichage avec numerotation versets (Page:Paragraphe)
- **PassagePreviewWidget**: Selection de passage avec boutons generation (Image/Audio/Video)
- **ImageViewerWidget**: Affichage images generees avec zoom/sauvegarde
- **AudioPlayerWidget**: Lecteur audio integre avec controles
- **SlideshowWidget**: Mode plein ecran pour presentation

### Systeme de Themes (NOUVEAU)
- **ThemeManager**: Singleton gerant themes et styles
- Themes predefinis: Sombre (defaut) et Clair
- Couleur d'accentuation personnalisable (QColorDialog)
- Polices configurables:
  - Police interface (famille + taille 8-16pt)
  - Police texte Codex (famille + taille 8-20pt)
- Onglet "Apparence" dans Parametres avec bouton "Appliquer"
- Persistence des preferences dans config.json

### Generation de Contenu
- **PipelineController**: Orchestration Claude -> Imagen -> Affichage
- **ClaudeClient**: Analyse passages et generation prompts
- **GeminiClient**: Alternative LLM Google
- **ImagenClient**: Generation images via Google Imagen 3
- **VeoClient**: Generation videos via Google Veo 2

### Synthese Vocale
- **ElevenLabsClient**: Voix premium (necessite API key)
- **EdgeTTSClient** (NOUVEAU): Voix neurales Microsoft gratuites
  - Implementation via Python edge-tts (subprocess)
  - Voix disponibles: Henri, Denise, Eloise, Remy (FR), Antoine, Sylvie (CA), Guy, Jenny (US)
  - Selecteur de voix dans toolbar principale
  - Bouton Test pour previsualiser

### Configuration
- **Config**: Singleton JSON avec paths, APIs, apparence
- **SecureStorage**: Stockage securise des cles API (DPAPI/Keychain)
- **SettingsDialog**: 3 onglets (Cles API, Chemins, Apparence)
- Support Google AI Studio et Vertex AI

### Base de Donnees
- SQLite via **Database** singleton
- Repositories: Project, Passage, Image, Audio
- Gestion projets avec auto-sauvegarde

### Utilitaires
- **Logger**: Logs horodates console + fichier
- **MessageBox**: Dialogues avec texte selectionnable/copiable
- **TextParser**: Parsing Markdown du Codex avec detection offset pages

## Architecture des Fichiers

```
src/
├── api/
│   ├── ApiClient.h/.cpp        # Base HTTP client
│   ├── ClaudeClient.h/.cpp     # Anthropic Claude
│   ├── GeminiClient.h/.cpp     # Google Gemini
│   ├── ImagenClient.h/.cpp     # Google Imagen
│   ├── VeoClient.h/.cpp        # Google Veo
│   ├── ElevenLabsClient.h/.cpp # ElevenLabs TTS
│   ├── EdgeTTSClient.h/.cpp    # Edge TTS (Python)
│   └── VertexAuthenticator.h/.cpp # Auth Vertex AI
├── core/
│   ├── controllers/
│   │   └── PipelineController.h/.cpp
│   └── services/
│       ├── TextParser.h/.cpp
│       ├── PromptBuilder.h/.cpp
│       ├── MythicClassifier.h/.cpp
│       └── GnosticEntities.h/.cpp
├── db/
│   ├── Database.h/.cpp
│   └── repositories/
│       ├── ProjectRepository.h/.cpp
│       ├── PassageRepository.h/.cpp
│       ├── ImageRepository.h/.cpp
│       └── AudioRepository.h/.cpp
├── ui/
│   ├── MainWindow.h/.cpp
│   ├── widgets/
│   │   ├── TextViewerWidget.h/.cpp
│   │   ├── ImageViewerWidget.h/.cpp
│   │   ├── TreatiseListWidget.h/.cpp
│   │   ├── PassagePreviewWidget.h/.cpp
│   │   ├── AudioPlayerWidget.h/.cpp
│   │   └── SlideshowWidget.h/.cpp
│   └── dialogs/
│       ├── SettingsDialog.h/.cpp
│       └── ProjectDialog.h/.cpp
└── utils/
    ├── Config.h/.cpp
    ├── Logger.h/.cpp
    ├── SecureStorage.h/.cpp
    ├── ThemeManager.h/.cpp     # NOUVEAU
    └── MessageBox.h            # NOUVEAU
```

## Configuration Actuelle

```json
{
  "apis": {
    "llm_provider": "claude",
    "tts_provider": "edge",
    "google_ai_provider": "aistudio"
  },
  "appearance": {
    "theme": "dark",
    "accent_color": "#094771",
    "font_family": "Segoe UI",
    "font_size": 12,
    "text_font_family": "Consolas",
    "text_font_size": 12
  }
}
```

## Dependances

- **Qt 6.8.3**: Core, Widgets, Network, Multimedia, Sql
- **nlohmann/json**: Parsing JSON (FetchContent)
- **Python 3.x + edge-tts**: Synthese vocale neurale
- **CMake 3.16+**: Build system
- **MSVC 2022 ou MinGW**: Compilateur

## Prochaines Etapes Possibles

1. Ameliorer le pipeline de generation (prompts plus detailles)
2. Ajouter historique des generations
3. Implementer export PDF/video du diaporama
4. Ajouter plus d'options dans ThemeManager (themes personnalises)
5. Optimiser le cache des images generees

## Commandes de Build

```batch
# Build complet
build.bat

# Build seulement
build.bat build

# Lancer l'application
launch.bat
```

## Notes Session

- Systeme de themes complet implemente et teste
- Edge TTS fonctionne avec voix neurales francaises
- Interface toolbar enrichie (voix, dossiers)
- Erreurs copiables dans les dialogues


---

# Source: _Etat-Projet_09-01-2026_03h20.md

---

# Etat du Projet - Codex Nag Hammadi BD

**Date**: 09 Janvier 2026 - 03h20
**Commit**: 9f05935

## Resume de la Session

### Fonctionnalites Ajoutees

1. **Diaporama (SlideshowDialog)**
   - Nouveau dialogue pour generer et lire des diaporamas avec images et audio
   - Generation automatique d'images via Gemini/Imagen
   - Generation audio TTS via EdgeTTS
   - Controles de lecture (play, pause, stop, precedent, suivant)
   - Miniatures a gauche, image principale a droite
   - Overlay de texte sur les images
   - Mode plein ecran
   - Statut: EN COURS DE DEBUG - les signaux ne sont pas correctement recus

2. **Visualisation d'Image**
   - Double-clic pour voir l'image en taille reelle dans une fenetre separee
   - Grille de planche avec generation sequentielle

3. **Interface Utilisateur**
   - Barre de progression dans la barre de statut pour la generation complete
   - Onglets Passage/Prompt au centre de la fenetre principale
   - Scrollbars ameliorees (plus larges, plus visibles)
   - Chemins de sortie par defaut vers la racine du projet (./images, ./videos)

### Fichiers Modifies

| Fichier | Modifications |
|---------|---------------|
| `src/ui/dialogs/SlideshowDialog.cpp/h` | NOUVEAU - Dialogue diaporama complet |
| `src/ui/MainWindow.cpp/h` | Onglets, barre de progression, menu diaporama |
| `src/ui/widgets/ImageViewerWidget.cpp/h` | Double-clic, grille de planche |
| `src/ui/widgets/PassagePreviewWidget.cpp/h` | Refactoring layout |
| `src/utils/Config.cpp` | Chemins par defaut modifies |
| `src/utils/ThemeManager.cpp` | Styles scrollbars ameliores |
| `src/core/controllers/PipelineController.cpp` | Debug logging |

### Correction Appliquee (Commit 03ed9cc)

**Le diaporama utilise maintenant le PipelineController partage:**
- SlideshowDialog recoit le PipelineController de MainWindow au lieu d'en creer un nouveau
- Ajout d'un flag `m_slideshowActive` pour eviter les conflits de signaux
- MainWindow ignore les signaux du pipeline quand le diaporama est actif
- Impossible d'ouvrir le diaporama pendant une generation en cours

### Prochaines Etapes

1. Tester le diaporama avec le PipelineController partage
2. Verifier que les images et l'audio se generent correctement
3. Ajouter des ameliorations UX si necessaire

### Configuration Actuelle

```json
{
    "apis": {
        "llm_provider": "gemini",
        "tts_provider": "edge",
        "google_ai_provider": "vertex"
    },
    "paths": {
        "output_images": "./images",
        "output_audio": "./images",
        "output_videos": "./videos"
    }
}
```

### Notes Techniques

- Qt6 avec QMediaPlayer pour l'audio
- Vertex AI pour Imagen (generation d'images)
- AI Studio pour Gemini (enrichissement de prompts)
- EdgeTTS pour la synthese vocale


---

# Source: BUILD.md

---

# Build Instructions - Codex Nag Hammadi BD

## Prérequis

### 1. CMake 3.21+
```powershell
# Via winget
winget install Kitware.CMake

# Ou télécharger: https://cmake.org/download/
```

### 2. Qt 6.5+
1. Télécharger l'installateur: https://www.qt.io/download-qt-installer
2. Installer les composants:
   - Qt 6.5.x (ou plus récent)
   - MSVC 2019/2022 64-bit **OU** MinGW 64-bit
   - Additional Libraries:
     - Qt Multimedia
     - Qt SQL Drivers (SQLite inclus par défaut)

### 3. Compilateur

**Option A: Visual Studio (recommandé)**
- Installer Visual Studio 2019 ou 2022
- Workload: "Desktop development with C++"

**Option B: MinGW**
- Installé automatiquement avec Qt si sélectionné

## Compilation

### Méthode simple: build.bat

```batch
# Depuis le dossier du projet
build.bat

# Options:
build.bat clean      # Nettoyer
build.bat configure  # Configurer seulement
build.bat build      # Compiler seulement
build.bat all        # Tout (défaut)
```

### Méthode manuelle

```powershell
# 1. Configurer (ajuster le chemin Qt)
cmake -B build -S . -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2019_64"

# 2. Compiler
cmake --build build --config Release --parallel

# 3. Exécuter
./build/Release/codex-nag-hammadi.exe
```

### Avec Visual Studio

```powershell
# Générer solution VS
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2019_64"

# Ouvrir dans VS
start build/codex-nag-hammadi.sln
```

## Structure du projet

```
codex-nag-hammadi/
├── CMakeLists.txt          # Configuration principale
├── CMakePresets.json       # Presets de build
├── build.bat               # Script de compilation
├── src/
│   ├── main.cpp
│   ├── api/                # Clients API (Claude, Imagen, ElevenLabs)
│   ├── core/               # Logique métier
│   ├── db/                 # Base de données SQLite
│   ├── ui/                 # Interface Qt
│   └── utils/              # Utilitaires
└── resources/
    └── data/               # Fichiers JSON de configuration
```

## Dépendances Qt utilisées

- Qt6::Core
- Qt6::Widgets
- Qt6::Network
- Qt6::Multimedia
- Qt6::Sql

## Problèmes courants

### "Qt6 not found"
Vérifiez que `CMAKE_PREFIX_PATH` pointe vers votre installation Qt:
```powershell
cmake -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2019_64"
```

### "MSVC not found"
Lancez la compilation depuis "Developer Command Prompt for VS 2022"

### DLLs manquantes au runtime
Copiez les DLLs Qt ou utilisez `windeployqt`:
```powershell
C:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe build\Release\codex-nag-hammadi.exe
```
