# Product Requirements Document (PRD)
# Codex Nag Hammadi BD

**Version:** 1.0
**Date:** 2026-01-08
**Auteur:** Hpcmao
**Statut:** Draft

---

## 1. Aperçu du Produit

### 1.1 Vision
Application desktop C++/Qt pour transformer les textes gnostiques de Nag Hammadi en BD photoréaliste immersive générée par IA, avec narration audio sacrée.

### 1.2 Objectif Principal
Créer un **pipeline fonctionnel** capable de traiter n'importe quel traité du Codex Nag Hammadi et de générer une expérience visuelle/audio cohérente.

### 1.3 Utilisateur Cible
Usage personnel pour exploration spirituelle et création artistique.

---

## 2. Stack Technique

| Composant | Technologie | Justification |
|-----------|-------------|---------------|
| Langage | C++17/20 | Performance, contrôle mémoire |
| Framework GUI | Qt 6 | Cross-platform, widgets riches, multimedia |
| Base de données | SQLite | Légère, embarquée, portable |
| HTTP Client | Qt Network / libcurl | Appels APIs |
| JSON | nlohmann/json ou Qt JSON | Parsing configs et réponses API |
| Audio | Qt Multimedia | Lecture narration |
| Build | CMake | Standard cross-platform |

### 2.1 APIs Externes

| Service | Usage | Coût estimé |
|---------|-------|-------------|
| Claude API (Anthropic) | Enrichissement prompts | ~$5/mois |
| Imagen 3 (Google Cloud) | Génération images | ~$6/mois |
| Veo 3.1 (Google Cloud) | Génération vidéo | ~$30/mois |
| ElevenLabs | Narration audio | ~$10/mois |

### 2.2 Plateformes Cibles
- Windows 10/11
- macOS 12+
- Linux (Ubuntu 22.04+, Fedora)

---

## 3. Périmètre Fonctionnel

### 3.1 In-Scope (MVP)

| ID | Fonctionnalité | Priorité |
|----|----------------|----------|
| F1 | Chargement et affichage des traités Markdown | P0 |
| F2 | Sélection de passages texte | P0 |
| F3 | Pipeline génération : Texte → Prompt → Image | P0 |
| F4 | Affichage des images générées | P0 |
| F5 | Classification mythique automatique | P1 |
| F6 | Application palette visuelle par catégorie | P1 |
| F7 | Génération narration audio (ElevenLabs) | P1 |
| F8 | Mode diaporama avec audio synchronisé | P1 |
| F9 | Sauvegarde projets en SQLite | P1 |
| F10 | Export images HD | P2 |

### 3.2 Out-of-Scope (Post-MVP)

| Fonctionnalité | Raison |
|----------------|--------|
| Export vidéo (Veo 3) | Complexité, coût |
| Ambiance sonore 432Hz | Nice-to-have |
| Géométrie sacrée overlay | Nice-to-have |
| Mode méditation | Nice-to-have |
| Multi-utilisateurs | Usage personnel |
| Application mobile | Desktop first |

---

## 4. User Stories

### Epic 1: Gestion des Textes

#### US-1.1: Charger les traités
**En tant qu'** utilisateur
**Je veux** charger les fichiers Markdown du Codex
**Afin de** parcourir les 52 traités disponibles

**Critères d'acceptation:**
- [ ] Lecture de fichiers .md (UTF-8)
- [ ] Parsing structure (titres, pages, paragraphes)
- [ ] Liste des traités dans un panneau latéral
- [ ] Recherche par nom de traité

#### US-1.2: Sélectionner un passage
**En tant qu'** utilisateur
**Je veux** sélectionner un passage de texte
**Afin de** le transformer en image

**Critères d'acceptation:**
- [ ] Sélection par clic-glisser ou délimiteurs
- [ ] Prévisualisation du texte sélectionné
- [ ] Indication du nombre de caractères/mots
- [ ] Possibilité de modifier la sélection

---

### Epic 2: Pipeline de Génération

#### US-2.1: Analyser le texte
**En tant qu'** système
**Je veux** analyser le texte sélectionné
**Afin d'** identifier les entités gnostiques et le contexte

**Critères d'acceptation:**
- [ ] Détection entités : Éons, Archontes, Sophia, Plérôme, etc.
- [ ] Classification catégorie mythique (9 catégories)
- [ ] Extraction mots-clés visuels
- [ ] Détermination palette couleur

#### US-2.2: Générer le prompt enrichi
**En tant qu'** système
**Je veux** enrichir le texte via Claude API
**Afin de** créer un prompt optimal pour Imagen

**Critères d'acceptation:**
- [ ] Appel Claude API avec contexte gnostique
- [ ] Extraction : scène, émotion, composition
- [ ] Application template style Villeneuve
- [ ] Prompt final < 1000 caractères

#### US-2.3: Générer l'image
**En tant qu'** utilisateur
**Je veux** générer une image à partir du prompt
**Afin de** visualiser le texte gnostique

**Critères d'acceptation:**
- [ ] Appel Imagen 3 API (Google Cloud)
- [ ] Affichage progression (loading)
- [ ] Affichage image résultat
- [ ] Option régénération si insatisfait
- [ ] Sauvegarde automatique en local

---

### Epic 3: Classification Mythique

#### US-3.1: Classifier automatiquement
**En tant qu'** système
**Je veux** détecter la catégorie mythique du traité
**Afin d'** appliquer la palette visuelle appropriée

**Critères d'acceptation:**
- [ ] 9 catégories : Plérôme, Sophia, Démiurge, Gnose, Ascension, Liturgie, Hermétique, Narratif, Fragments
- [ ] Mapping traité → catégorie prédéfini
- [ ] Override manuel possible
- [ ] Affichage catégorie dans l'UI

#### US-3.2: Appliquer la palette
**En tant qu'** système
**Je veux** injecter les paramètres visuels de la catégorie
**Afin d'** avoir une cohérence stylistique

**Critères d'acceptation:**
- [ ] Palette couleurs par catégorie
- [ ] Keywords style par catégorie
- [ ] Intégration dans le prompt template

---

### Epic 4: Narration Audio

#### US-4.1: Générer l'audio
**En tant qu'** utilisateur
**Je veux** générer une narration audio du texte
**Afin d'** avoir une expérience immersive

**Critères d'acceptation:**
- [ ] Appel ElevenLabs API
- [ ] Voix masculine, grave, contemplative
- [ ] Langue française
- [ ] Sauvegarde fichier audio local

#### US-4.2: Synchroniser audio/images
**En tant qu'** utilisateur
**Je veux** voir les images défiler avec l'audio
**Afin d'** avoir une expérience multimédia cohérente

**Critères d'acceptation:**
- [ ] Calcul durée par passage
- [ ] Transition images synchronisée
- [ ] Contrôles lecture (play/pause/stop)
- [ ] Barre de progression

---

### Epic 5: Mode Diaporama

#### US-5.1: Lancer le diaporama
**En tant qu'** utilisateur
**Je veux** visualiser une séquence d'images en plein écran
**Afin de** contempler le traité de manière immersive

**Critères d'acceptation:**
- [ ] Mode plein écran
- [ ] Transitions fluides (fondu)
- [ ] Audio synchronisé
- [ ] Contrôles overlay (discrets)
- [ ] Touche Échap pour quitter

---

### Epic 6: Persistance

#### US-6.1: Sauvegarder un projet
**En tant qu'** utilisateur
**Je veux** sauvegarder mon travail
**Afin de** le reprendre plus tard

**Critères d'acceptation:**
- [ ] Structure SQLite : projets, passages, images, audio
- [ ] Sauvegarde automatique
- [ ] Sauvegarde manuelle
- [ ] Liste projets récents

#### US-6.2: Exporter les images
**En tant qu'** utilisateur
**Je veux** exporter les images générées
**Afin de** les utiliser ailleurs

**Critères d'acceptation:**
- [ ] Export PNG/JPEG haute résolution
- [ ] Export par lot (tout le projet)
- [ ] Nommage automatique cohérent

---

## 5. Exigences Non-Fonctionnelles

### 5.1 Performance

| Métrique | Objectif |
|----------|----------|
| Temps démarrage app | < 3 secondes |
| Chargement traité | < 1 seconde |
| Réponse UI | < 100ms |
| Génération image | < 30 secondes (API) |

### 5.2 Qualité

| Aspect | Exigence |
|--------|----------|
| Stabilité | Pas de crash sur usage normal |
| Mémoire | < 500 MB RAM en usage standard |
| Logs | Journalisation erreurs API |

### 5.3 Sécurité

| Aspect | Exigence |
|--------|----------|
| Clés API | Stockage sécurisé (pas en clair) |
| Données | Locales uniquement |

### 5.4 Utilisabilité

| Aspect | Exigence |
|--------|----------|
| Langue UI | Français |
| Accessibilité | Raccourcis clavier principaux |
| Feedback | Indicateurs de progression clairs |

---

## 6. Architecture Préliminaire

```
┌─────────────────────────────────────────────────────────────────┐
│                         APPLICATION Qt                           │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │   MainWindow │  │  TextViewer │  │ ImageViewer │             │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘             │
│         │                │                │                      │
│  ┌──────┴────────────────┴────────────────┴──────┐              │
│  │              CONTROLLERS                        │              │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐       │              │
│  │  │ Project  │ │ Pipeline │ │ Playback │       │              │
│  │  │Controller│ │Controller│ │Controller│       │              │
│  │  └────┬─────┘ └────┬─────┘ └────┬─────┘       │              │
│  └───────┼────────────┼────────────┼─────────────┘              │
│          │            │            │                             │
│  ┌───────┴────────────┴────────────┴─────────────┐              │
│  │                  SERVICES                       │              │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐       │              │
│  │  │ TextParse│ │ APIClient│ │AudioPlayer│      │              │
│  │  │ Service  │ │ Service  │ │ Service  │       │              │
│  │  └──────────┘ └──────────┘ └──────────┘       │              │
│  └───────────────────────────────────────────────┘              │
│                           │                                      │
│  ┌────────────────────────┴──────────────────────┐              │
│  │                   DATA LAYER                    │              │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐       │              │
│  │  │ SQLite   │ │  Files   │ │  Cache   │       │              │
│  │  │ Database │ │ (images) │ │ (API)    │       │              │
│  │  └──────────┘ └──────────┘ └──────────┘       │              │
│  └───────────────────────────────────────────────┘              │
└─────────────────────────────────────────────────────────────────┘
```

### 6.1 Modules Principaux

| Module | Responsabilité |
|--------|----------------|
| `core/` | Logique métier, modèles de données |
| `services/` | APIs externes, parsing, audio |
| `ui/` | Widgets Qt, vues |
| `db/` | SQLite, migrations |
| `utils/` | Helpers, logging, config |

---

## 7. Modèle de Données (SQLite)

```sql
-- Projets
CREATE TABLE projects (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    treatise_code TEXT,        -- ex: "I-1" pour Prière Apôtre Paul
    category TEXT,             -- Catégorie mythique
    created_at DATETIME,
    updated_at DATETIME
);

-- Passages sélectionnés
CREATE TABLE passages (
    id INTEGER PRIMARY KEY,
    project_id INTEGER REFERENCES projects(id),
    text_content TEXT NOT NULL,
    start_position INTEGER,
    end_position INTEGER,
    order_index INTEGER,
    created_at DATETIME
);

-- Images générées
CREATE TABLE images (
    id INTEGER PRIMARY KEY,
    passage_id INTEGER REFERENCES passages(id),
    prompt_used TEXT,
    file_path TEXT,
    generation_params TEXT,    -- JSON: model, seed, etc.
    created_at DATETIME
);

-- Audio générés
CREATE TABLE audio_files (
    id INTEGER PRIMARY KEY,
    passage_id INTEGER REFERENCES passages(id),
    file_path TEXT,
    duration_ms INTEGER,
    voice_id TEXT,
    created_at DATETIME
);

-- Configuration
CREATE TABLE config (
    key TEXT PRIMARY KEY,
    value TEXT
);
```

---

## 8. Configuration API

```json
{
  "apis": {
    "claude": {
      "endpoint": "https://api.anthropic.com/v1/messages",
      "model": "claude-sonnet-4-20250514",
      "max_tokens": 1000
    },
    "imagen": {
      "endpoint": "https://generativelanguage.googleapis.com/v1beta/models/imagen-3.0-generate-001",
      "aspect_ratio": "16:9"
    },
    "elevenlabs": {
      "endpoint": "https://api.elevenlabs.io/v1/text-to-speech",
      "voice_id": "TO_CONFIGURE",
      "model_id": "eleven_multilingual_v2"
    }
  },
  "style": {
    "base_template": "Cinematic, photoreal, Denis Villeneuve style, dramatic lighting, desaturated colors, monumental scale, contemplative atmosphere"
  }
}
```

---

## 9. Dictionnaire des Entités Gnostiques

```json
{
  "entities": {
    "Plérôme": {
      "description": "Plénitude divine, totalité des émanations",
      "visual_keywords": ["infinite light", "cosmic void", "golden radiance"],
      "palette": ["#FFD700", "#FFFFFF", "#4169E1"]
    },
    "Éons": {
      "description": "Émanations divines, entités célestes",
      "visual_keywords": ["luminous beings", "ethereal figures", "light forms"],
      "palette": ["#E6E6FA", "#FFD700", "#87CEEB"]
    },
    "Sophia": {
      "description": "Sagesse divine, figure féminine tragique",
      "visual_keywords": ["weeping figure", "falling light", "blue tears"],
      "palette": ["#191970", "#C0C0C0", "#4682B4"]
    },
    "Archontes": {
      "description": "Dirigeants du monde matériel, forces hostiles",
      "visual_keywords": ["dark rulers", "shadowy figures", "oppressive presence"],
      "palette": ["#8B0000", "#000000", "#2F4F4F"]
    },
    "Démiurge": {
      "description": "Créateur du monde matériel, Yaldabaoth",
      "visual_keywords": ["lion-headed serpent", "fire and darkness", "throne of ignorance"],
      "palette": ["#B22222", "#000000", "#CD853F"]
    },
    "Christ/Sauveur": {
      "description": "Figure de rédemption, porteur de gnose",
      "visual_keywords": ["radiant figure", "descending light", "gentle illumination"],
      "palette": ["#FFFAF0", "#FFD700", "#F0E68C"]
    }
  }
}
```

---

## 10. Risques et Mitigations

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| APIs indisponibles | Moyenne | Haut | Cache local, mode offline partiel |
| Coûts API dépassent budget | Moyenne | Moyen | Compteur usage, alertes |
| Style Villeneuve non capturé | Moyenne | Moyen | Itérations prompts, tests A/B |
| Complexité C++/Qt | Faible | Moyen | Documentation, patterns établis |
| Cross-platform issues | Moyenne | Moyen | CI/CD multi-plateforme, tests |

---

## 11. Métriques de Succès

| Métrique | Objectif MVP |
|----------|--------------|
| Pipeline fonctionnel | 100% des étapes exécutables |
| Traité pilote complété | Prière Apôtre Paul en images |
| Temps génération | < 2 min par passage |
| Qualité visuelle | Style Villeneuve reconnaissable |
| Stabilité | 0 crash sur workflow standard |

---

## 12. Jalons

| Jalon | Livrable | Dépendances |
|-------|----------|-------------|
| M1 | Squelette Qt + chargement Markdown | - |
| M2 | Intégration Claude API | M1 |
| M3 | Intégration Imagen 3 | M2 |
| M4 | Classification mythique + palettes | M3 |
| M5 | Intégration ElevenLabs | M4 |
| M6 | Mode diaporama | M5 |
| M7 | Persistance SQLite | M1 |
| M8 | MVP complet | M6, M7 |

---

## 13. Prochaine Étape

**Workflow suivant:** `/bmad:bmm:workflows:create-architecture`

L'architecture détaillera :
- Structure des modules C++
- Patterns de conception
- Interfaces entre composants
- Stratégie de build CMake

---

*PRD créé le 2026-01-08*
*Basé sur le Product Brief v1.0*
