@echo off
setlocal enabledelayedexpansion

echo ========================================
echo  Codex Nag Hammadi BD - Build Script
echo ========================================
echo.

:: Configuration - Modifier ces chemins selon votre installation
set "QT_PATH=C:\Qt\6.5.3\msvc2019_64"
set "CMAKE_PATH=C:\Program Files\CMake\bin"
set "BUILD_TYPE=Release"
set "BUILD_DIR=build"

:: Chercher Qt automatiquement si pas trouve
if not exist "%QT_PATH%" (
    echo [INFO] Qt non trouve a %QT_PATH%, recherche automatique...
    for /d %%i in (C:\Qt\6.*) do (
        for /d %%j in (%%i\msvc*_64) do (
            if exist "%%j\bin\qmake.exe" (
                set "QT_PATH=%%j"
                echo [OK] Qt trouve: !QT_PATH!
                goto :qt_found
            )
        )
        for /d %%j in (%%i\mingw*_64) do (
            if exist "%%j\bin\qmake.exe" (
                set "QT_PATH=%%j"
                echo [OK] Qt trouve: !QT_PATH!
                goto :qt_found
            )
        )
    )
    echo [ERREUR] Qt6 non trouve. Installez Qt6 depuis https://www.qt.io/download
    echo          Puis modifiez QT_PATH dans ce script.
    pause
    exit /b 1
)
:qt_found

:: Chercher CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    if exist "%CMAKE_PATH%\cmake.exe" (
        set "PATH=%CMAKE_PATH%;%PATH%"
    ) else (
        echo [ERREUR] CMake non trouve. Installez CMake depuis https://cmake.org/download
        pause
        exit /b 1
    )
)
echo [OK] CMake trouve

:: Verifier le compilateur (MSVC ou MinGW)
where cl >nul 2>&1
if %errorlevel% equ 0 (
    set "GENERATOR=Visual Studio 17 2022"
    set "COMPILER=MSVC"
    echo [OK] Compilateur MSVC trouve
) else (
    where g++ >nul 2>&1
    if %errorlevel% equ 0 (
        set "GENERATOR=MinGW Makefiles"
        set "COMPILER=MinGW"
        echo [OK] Compilateur MinGW trouve
    ) else (
        echo [ERREUR] Aucun compilateur trouve.
        echo          - Pour MSVC: Lancez ce script depuis "Developer Command Prompt for VS"
        echo          - Ou installez MinGW-w64
        pause
        exit /b 1
    )
)

echo.
echo Configuration:
echo   Qt:          %QT_PATH%
echo   Compilateur: %COMPILER%
echo   Build Type:  %BUILD_TYPE%
echo   Build Dir:   %BUILD_DIR%
echo.

:: Argument: clean, configure, build, run, all
set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=all"

:: Clean
if "%ACTION%"=="clean" goto :clean
if "%ACTION%"=="all" goto :configure

:clean
echo [CLEAN] Suppression du dossier build...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if "%ACTION%"=="clean" goto :end

:configure
echo [CONFIGURE] Configuration CMake...
if "%COMPILER%"=="MSVC" (
    cmake -B "%BUILD_DIR%" -S . -G "%GENERATOR%" -A x64 ^
        -DCMAKE_PREFIX_PATH="%QT_PATH%" ^
        -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
) else (
    cmake -B "%BUILD_DIR%" -S . -G "%GENERATOR%" ^
        -DCMAKE_PREFIX_PATH="%QT_PATH%" ^
        -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
)
if %errorlevel% neq 0 (
    echo [ERREUR] La configuration CMake a echoue.
    pause
    exit /b 1
)
echo [OK] Configuration terminee
if "%ACTION%"=="configure" goto :end

:build
echo.
echo [BUILD] Compilation en cours...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel
if %errorlevel% neq 0 (
    echo [ERREUR] La compilation a echoue.
    pause
    exit /b 1
)
echo [OK] Compilation terminee
if "%ACTION%"=="build" goto :end

:run
echo.
set "EXE_PATH=%BUILD_DIR%\%BUILD_TYPE%\codex-nag-hammadi.exe"
if not exist "%EXE_PATH%" (
    set "EXE_PATH=%BUILD_DIR%\codex-nag-hammadi.exe"
)
if exist "%EXE_PATH%" (
    echo [RUN] Lancement de l'application...
    echo Executable: %EXE_PATH%
    start "" "%EXE_PATH%"
) else (
    echo [INFO] Executable non trouve. Compilation terminee.
    echo        Cherchez codex-nag-hammadi.exe dans %BUILD_DIR%
)

:end
echo.
echo ========================================
echo  Termine!
echo ========================================
if "%ACTION%"=="all" pause
endlocal
