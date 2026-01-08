@echo off
echo Lancement de Codex Nag Hammadi BD...

:: Ajouter Qt au PATH
set "QT_PATH=C:\Qt\6.8.3\msvc2022_64\bin"
if not exist "%QT_PATH%" (
    echo [ERREUR] Qt non trouve a %QT_PATH%
    pause
    exit /b 1
)
set "PATH=%QT_PATH%;%PATH%"

:: Trouver l'executable
set "EXE_PATH=%~dp0build\bin\Debug\codex-nag-hammadi.exe"

if not exist "%EXE_PATH%" (
    set "EXE_PATH=%~dp0build\bin\Release\codex-nag-hammadi.exe"
)

if not exist "%EXE_PATH%" (
    echo [ERREUR] Executable non trouve.
    echo          Compilez d'abord le projet avec build.bat
    pause
    exit /b 1
)

echo Qt: %QT_PATH%
echo Executable: %EXE_PATH%
start "" "%EXE_PATH%"
