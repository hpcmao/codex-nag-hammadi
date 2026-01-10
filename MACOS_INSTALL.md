# Installation sur macOS - Codex Nag Hammadi BD

## Compatibilite

| Composant | Status | Notes |
|-----------|--------|-------|
| Qt6 GUI | OK | Cross-platform natif |
| SQLite DB | OK | Fonctionne partout |
| Keychain | OK | Implementation native macOS |
| FFmpeg | A installer | Via Homebrew |
| Chemins | OK | QStandardPaths utilise |

## Prerequis

### 1. Homebrew (gestionnaire de paquets)
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. Qt6
```bash
brew install qt@6
```

### 3. CMake
```bash
brew install cmake
```

### 4. FFmpeg (pour export video)
```bash
brew install ffmpeg
```

### 5. Python 3 + cryptography (optionnel, pour Vertex AI OAuth)
```bash
brew install python3
pip3 install cryptography
```

## Compilation

### 1. Cloner le projet
```bash
git clone https://github.com/hpcmao/codex-nag-hammadi.git
cd codex-nag-hammadi
```

### 2. Configurer CMake
```bash
# Pour Intel Mac
cmake -B build -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)

# Pour Apple Silicon (M1/M2/M3)
cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6
```

### 3. Compiler
```bash
cmake --build build --config Release
```

### 4. Lancer
```bash
./build/bin/codex-nag-hammadi
```

Ou en tant qu'application macOS :
```bash
open build/bin/codex-nag-hammadi.app
```

## Configuration Post-Installation

### Cle API Gemini
1. Aller sur https://aistudio.google.com/apikey
2. Creer une cle API avec Paid Tier active
3. Dans l'app : Parametres > API > Gemini AI

### Chemins de stockage sur macOS
- Configuration : `~/Library/Application Support/Hpcmao/Codex Nag Hammadi BD/`
- Base de donnees : `~/Library/Application Support/Hpcmao/Codex Nag Hammadi BD/codex.db`
- Cles API : Keychain macOS (securise)

## Problemes Connus et Solutions

### FFmpeg non trouve
**Symptome:** Message "FFmpeg requis" lors de l'export video

**Solution:**
```bash
# Verifier l'installation
which ffmpeg

# Si pas installe
brew install ffmpeg

# Verifier les chemins
echo $PATH
```

FFmpeg doit etre dans un de ces chemins :
- `/usr/local/bin/ffmpeg` (Intel Homebrew)
- `/opt/homebrew/bin/ffmpeg` (Apple Silicon Homebrew)
- `/opt/local/bin/ffmpeg` (MacPorts)

### Python non trouve (Vertex AI OAuth)
**Symptome:** Erreur d'authentification Vertex AI

**Solution:**
```bash
# macOS utilise python3 par defaut
python3 --version

# Installer cryptography
pip3 install cryptography
```

### Erreur de signature d'application
**Symptome:** "Application non verifiee" au lancement

**Solution:**
```bash
# Option 1: Autoriser dans Preferences Systeme > Securite
# Option 2: Signer l'application
codesign -s - build/bin/codex-nag-hammadi.app
```

## Differences avec Windows

| Fonctionnalite | Windows | macOS |
|----------------|---------|-------|
| Stockage cles API | DPAPI (fichier chiffre) | Keychain natif |
| Chemin config | `%APPDATA%\Local\...` | `~/Library/Application Support/...` |
| FFmpeg | winget ou manuel | brew install ffmpeg |
| Bundle app | .exe | .app bundle |

## Structure du Bundle macOS

```
codex-nag-hammadi.app/
├── Contents/
│   ├── Info.plist
│   ├── MacOS/
│   │   └── codex-nag-hammadi (executable)
│   ├── Resources/
│   │   └── (icons, resources)
│   └── Frameworks/
│       └── (Qt frameworks si deploye)
```

## Deploiement (Distribution)

Pour creer une application distribuable :

```bash
# 1. Compiler en Release
cmake --build build --config Release

# 2. Deployer les frameworks Qt
$(brew --prefix qt@6)/bin/macdeployqt build/bin/codex-nag-hammadi.app

# 3. Signer (optionnel mais recommande)
codesign --deep -s "Developer ID Application: Votre Nom" build/bin/codex-nag-hammadi.app

# 4. Creer un DMG
hdiutil create -volname "Codex Nag Hammadi" -srcfolder build/bin/codex-nag-hammadi.app -ov -format UDZO codex-nag-hammadi.dmg
```

## Support

En cas de probleme :
1. Verifier les logs dans `~/Library/Application Support/Hpcmao/Codex Nag Hammadi BD/logs/`
2. Ouvrir une issue sur GitHub : https://github.com/hpcmao/codex-nag-hammadi/issues
