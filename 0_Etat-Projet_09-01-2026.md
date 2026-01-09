# État du Projet - Codex Nag Hammadi BD

**Date:** 09-01-2026
**Phase:** Développement UI / Diaporama

---

## Résumé du Projet

**Objectif:** Créer une BD photoréaliste immersive des textes gnostiques de Nag Hammadi en utilisant l'IA générative.

**Vision:** Expérience immersive des textes anciens avec:
- Visuel mystique (style Villeneuve)
- Narration sacrée (voix neurale Edge TTS)
- Diaporama automatique images + audio

---

## Fonctionnalités Implémentées

### Application Qt6 (C++)
- **Interface principale** avec viewer d'images, liste de traités, éditeur de texte
- **Pipeline de génération** : Texte → Claude/Gemini → Imagen 3
- **Système de thèmes** : Clair/Sombre
- **Gestion des planches** : Grilles d'images (1x1, 2x2, 3x3, etc.)

### Diaporama (Session 09-01-2026)
- **Bouton Play/Stop** unique (toggle)
- **Sélecteur de voix** : 5 voix françaises neurales
  - Henri (homme)
  - Denise (femme)
  - Eloise (douce)
  - Vivienne (femme)
  - Remy (homme)
- **Démarrage automatique** à l'ouverture
- **Lecture dès la 1ère image** prête avec audio
- **Texte affiché** en bas de l'image (overlay)
- **Contrôles** : <<, Play/Stop, >>, slider, volume, boucle, plein écran

### Edge TTS (Text-to-Speech)
- Utilisation de Microsoft Edge Neural voices (gratuit)
- Correction : fichier texte temporaire pour caractères spéciaux
- Voix disponibles validées : fr-FR-HenriNeural, DeniseNeural, EloiseNeural, VivienneMultilingualNeural, RemyMultilingualNeural

---

## Architecture Technique

### Structure du Projet
```
src/
├── api/
│   ├── ClaudeClient.cpp      # API Claude
│   ├── GeminiClient.cpp      # API Gemini
│   ├── ImagenClient.cpp      # API Imagen 3
│   ├── EdgeTTSClient.cpp     # TTS Neural (corrigé)
│   └── ElevenLabsClient.cpp  # TTS alternatif
├── core/
│   ├── PipelineController.cpp # Orchestration génération
│   └── TextParser.cpp        # Analyse texte
├── ui/
│   ├── MainWindow.cpp        # Fenêtre principale
│   └── dialogs/
│       ├── SlideshowDialog.cpp # Diaporama (amélioré)
│       └── SettingsDialog.cpp  # Paramètres
└── utils/
    ├── Config.cpp            # Configuration
    └── Logger.cpp            # Logs
```

### Pipeline Génération
```
Texte sélectionné
    → Classification mythique
    → Enrichissement Claude/Gemini (scène, émotion, mots-clés)
    → Construction prompt Villeneuve
    → Imagen 3 (génération image)
    → Edge TTS (génération audio)
    → Affichage diaporama
```

---

## Commits Récents

| Commit | Description |
|--------|-------------|
| `a694eb2` | feat: Improve slideshow dialog with auto-play, voice selector, and bug fixes |
| `03ed9cc` | fix: Share PipelineController between MainWindow and SlideshowDialog |
| `9f05935` | feat: Add slideshow dialog, image viewer improvements, and UI enhancements |
| `476cd00` | feat: Add Gemini 3 Pro integration, image plates, and folder management |

---

## Problèmes Connus / En Cours

### Diaporama
- [ ] Conflit génération : MainWindow et SlideshowDialog partagent le PipelineController
- [ ] "Une generation est deja en cours" si génération rapide

### Fenêtre Principale
- [ ] Planche n'affiche qu'une seule image si diaporama ouvert pendant génération

---

## Prochaines Étapes

### Priorité Haute
1. Résoudre le conflit de génération MainWindow/Diaporama
2. Permettre génération indépendante pour le diaporama
3. Tester le son du diaporama

### Priorité Moyenne
4. Améliorer la gestion des erreurs TTS
5. Ajouter indicateur de progression audio
6. Sauvegarder la voix sélectionnée dans le diaporama

---

## Configuration Requise

- Qt 6.x avec modules Multimedia
- Python 3.x avec `edge-tts` (`pip install edge-tts`)
- Clés API : Claude, Gemini, Imagen
- Windows 10/11

---

## Notes Techniques

### Edge TTS - Voix Françaises Valides
```
fr-FR-HenriNeural       (homme)
fr-FR-DeniseNeural      (femme)
fr-FR-EloiseNeural      (femme, douce)
fr-FR-VivienneMultilingualNeural (femme)
fr-FR-RemyMultilingualNeural     (homme)
```

### Commande Edge TTS
```bash
edge-tts -v fr-FR-HenriNeural -f input.txt --write-media output.mp3
```
