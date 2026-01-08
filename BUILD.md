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
