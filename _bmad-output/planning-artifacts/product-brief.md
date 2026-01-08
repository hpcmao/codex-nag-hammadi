# Product Brief - Codex Nag Hammadi BD

**Version:** 1.0
**Date:** 2026-01-08
**Auteur:** Hpcmao
**Statut:** Draft

---

## 1. Vision

> **Créer une expérience immersive des textes gnostiques de Nag Hammadi sous forme de BD photoréaliste générée par IA.**

L'application transforme les 52 traités du Codex Nag Hammadi en planches visuelles cinématographiques style Denis Villeneuve, accompagnées d'une narration audio sacrée.

### Trois Piliers de l'Expérience

| Pilier | Description |
|--------|-------------|
| **Visuel Mystique** | Images photoréalistes épiques, monumentalité, lumière dramatique |
| **Narration Sacrée** | Voix masculine grave et contemplative (ElevenLabs) |
| **Ambiance Sonore** | Enveloppement sensoriel complet |

---

## 2. Problème Résolu

### Constat
Les textes gnostiques de Nag Hammadi sont :
- **Abstraits** - Concepts cosmologiques difficiles à visualiser
- **Inaccessibles** - Langage ancien, contexte perdu
- **Fragmentés** - 52 traités sans fil conducteur visuel

### Solution
Transformer ces textes en **expérience sensorielle immersive** :
- Chaque passage devient une image cinématographique
- La narration audio crée la connexion émotionnelle
- La classification mythique guide la palette visuelle

---

## 3. Utilisateur Cible

**Profil:** Usage personnel - exploration spirituelle et création artistique

| Aspect | Description |
|--------|-------------|
| Motivation | Connexion profonde aux textes anciens |
| Usage | Sessions contemplatives, méditation visuelle |
| Attentes | Qualité cinématographique, contrôle créatif |

---

## 4. Forme du Produit

**Format principal:** Application desktop + exports diaporama/vidéo

### Composants

```
┌─────────────────────────────────────────────────────────────┐
│                    APPLICATION DESKTOP                       │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │  SÉLECTION  │  │ GÉNÉRATION  │  │  DIAPORAMA  │         │
│  │   TEXTES    │→ │   IMAGES    │→ │    AUDIO    │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
│                                                              │
│  Exports: Images HD | Vidéos (Veo 3) | Audio MP3            │
└─────────────────────────────────────────────────────────────┘
```

---

## 5. Direction Artistique

### Style Visuel: Denis Villeneuve

| Caractéristique | Application |
|-----------------|-------------|
| Monumentalité | Éons, Plérôme = espaces cosmiques infinis |
| Lumière dramatique | Contre-jours, rayons traversant la poussière |
| Échelle cosmique | Figures gnostiques face à l'immensité |
| Couleurs désaturées | Ocres, gris bleutés, dorés sourds |
| Références | Dune, Arrival, Blade Runner 2049 |

### Classification Mythique des Traités

| Catégorie | Traités | Palette Visuelle |
|-----------|---------|------------------|
| Plérôme (Lumière divine) | 7 | Or, blanc pur, bleu céleste |
| Sophia (Chute) | 7 | Bleu profond, argent, mélancolie |
| Démiurge (Création) | 7 | Rouge sombre, noir, feu |
| Gnose (Révélation) | 12 | Ocre → or, rayon perçant |
| Ascension (Retour) | 8 | Or croissant, transparence |
| Liturgie | 3 | Variable selon invocation |
| Hermétique | 6 | Tons neutres, intellectuels |
| Narratif | 4 | Terrestre avec touches lumineuses |
| Fragments | 2 | - |

---

## 6. Pipeline Technique

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│  PRÉ-ANALYSE    │────▶│  LLM ENRICHMENT │────▶│ TEMPLATE        │
│  (Python local) │     │  (Claude API)   │     │ VILLENEUVE      │
└─────────────────┘     └─────────────────┘     └────────┬────────┘
                                                         │
Détection entités       Contexte narratif               Prompt
gnostiques              Émotion, composition            Imagen 3
```

### Technologies

| Composant | Technologie | Coût estimé/mois |
|-----------|-------------|------------------|
| Génération images | Imagen 3 (Google Cloud) | ~$6 |
| Animation vidéo | Veo 3.1 Fast | ~$30 |
| Narration audio | ElevenLabs | ~$10 |
| Enrichissement prompts | Claude API | ~$5 |
| **TOTAL** | | **~$50** |

---

## 7. Objectif Court Terme

**But:** Pipeline fonctionnel capable de traiter n'importe quel traité

### Critères de Succès

- [ ] Pipeline texte → image opérationnel
- [ ] Détection automatique de la catégorie mythique
- [ ] Palette visuelle appliquée selon catégorie
- [ ] Génération d'images style Villeneuve validé
- [ ] Narration audio synchronisée

### Traité Pilote

**Prière de l'Apôtre Paul** (Codex I-1)
- Court (1 page)
- Catégorie: Liturgie/Prière
- Contenu: Invocation au Plérôme
- Idéal pour valider le pipeline complet

---

## 8. Fonctionnalités Clés

### MVP (Minimum Viable Product)

| Priorité | Fonctionnalité | Description |
|----------|----------------|-------------|
| P0 | Sélection de texte | Charger et afficher les traités |
| P0 | Pipeline génération | Texte → Prompt → Imagen 3 |
| P0 | Affichage images | Visualiser les résultats |
| P1 | Classification auto | Détecter catégorie mythique |
| P1 | Narration audio | ElevenLabs + synchronisation |
| P1 | Mode diaporama | Lecture séquentielle |

### Post-MVP

| Fonctionnalité | Description |
|----------------|-------------|
| Export vidéo | Conversion via Veo 3 |
| Ambiance sonore | Fréquences sacrées 432Hz |
| Géométrie sacrée | Overlays sur images |
| Mode méditation | Transitions ultra lentes |

---

## 9. Questions Ouvertes

### À Résoudre Pendant le Développement

| Question | Options | Impact |
|----------|---------|--------|
| Cohérence personnages | Même visage vs variation mystique | Prompts, seed Imagen |
| Découpage textes | Manuel vs automatique | UX, complexité |
| Images par page | 1, 3, 5? | Rythme, coût |

---

## 10. Contraintes et Risques

### Contraintes

| Type | Contrainte |
|------|------------|
| Budget | ~$50/mois pour APIs |
| Technique | Dépendance APIs externes (Google, Anthropic, ElevenLabs) |
| Contenu | Qualité variable selon passages abstraits |

### Risques

| Risque | Mitigation |
|--------|------------|
| Imagen 3 ne capture pas le style Villeneuve | Tests itératifs sur prompts, templates |
| Coûts API dépassent budget | Génération sélective, cache |
| Textes trop abstraits | Skip ou représentation symbolique |

---

## 11. Ressources Existantes

| Fichier | Description |
|---------|-------------|
| `codex-nag-hammadi.md` | Textes source (1.3 MB, 52 traités) |
| `Gnose Nag Hammadi.md` | Format alternatif (1.1 MB) |
| `brainstorming-session-2026-01-07.md` | Décisions architecturales |

---

## 12. Prochaine Étape

**Workflow suivant:** `/bmad:bmm:workflows:create-prd`

Le PRD détaillera :
- Spécifications fonctionnelles complètes
- User stories
- Critères d'acceptation
- Architecture technique préliminaire

---

*Product Brief créé le 2026-01-08*
*Basé sur la session de brainstorming du 2026-01-07*
