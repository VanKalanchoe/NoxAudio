@echo off
setlocal EnableExtensions

for %%I in ("%~dp0..") do set "ROOT_DIR=%%~fI"
set "VENDORS_DIR=%ROOT_DIR%\vendors"

if not exist "%VENDORS_DIR%" mkdir "%VENDORS_DIR%"

echo ============================================================================
echo Installing NoxAudio dependencies into:
echo   %VENDORS_DIR%
echo ============================================================================

:: ----------------------------------------------------------------------------
:: 1. miniaudio (single-header audio I/O + built-in WAV/FLAC/MP3 + stb_vorbis)
:: ----------------------------------------------------------------------------
set "MINIAUDIO_DIR=%VENDORS_DIR%\miniaudio"
if not exist "%MINIAUDIO_DIR%\miniaudio.h" (
    echo [1/3] Cloning miniaudio into "%MINIAUDIO_DIR%"...
    if exist "%MINIAUDIO_DIR%" rmdir /s /q "%MINIAUDIO_DIR%"
    git clone --depth 1 https://github.com/mackron/miniaudio.git "%MINIAUDIO_DIR%"
    if errorlevel 1 (
        echo ERROR: Failed to clone miniaudio.
        goto :fail
    )
) else (
    echo [1/3] miniaudio already present, skipping.
)

:: ----------------------------------------------------------------------------
:: 2. Steam Audio SDK (v4.8.1)
:: ----------------------------------------------------------------------------
set "STEAMAUDIO_VERSION=4.8.1"
set "STEAMAUDIO_URL=https://github.com/ValveSoftware/steam-audio/releases/download/v%STEAMAUDIO_VERSION%/steamaudio_%STEAMAUDIO_VERSION%.zip"
set "STEAMAUDIO_DIR=%VENDORS_DIR%\steamaudio"
set "STEAMAUDIO_ZIP=%TEMP%\noxaudio-steamaudio-%STEAMAUDIO_VERSION%.zip"

if not exist "%STEAMAUDIO_DIR%\include\phonon.h" (
    echo [2/3] Downloading Steam Audio SDK v%STEAMAUDIO_VERSION%...
    curl -fSL "%STEAMAUDIO_URL%" -o "%STEAMAUDIO_ZIP%"
    if errorlevel 1 (
        echo ERROR: Failed to download Steam Audio SDK from %STEAMAUDIO_URL%.
        goto :fail
    )
    if exist "%STEAMAUDIO_DIR%" rmdir /s /q "%STEAMAUDIO_DIR%"
    echo       Extracting Steam Audio SDK into "%VENDORS_DIR%"...
    tar -xf "%STEAMAUDIO_ZIP%" -C "%VENDORS_DIR%"
    if errorlevel 1 (
        if exist "%STEAMAUDIO_ZIP%" del /q "%STEAMAUDIO_ZIP%"
        if exist "%STEAMAUDIO_DIR%" rmdir /s /q "%STEAMAUDIO_DIR%"
        echo ERROR: Failed to extract Steam Audio SDK archive.
        goto :fail
    )
    if exist "%STEAMAUDIO_ZIP%" del /q "%STEAMAUDIO_ZIP%"
    if not exist "%STEAMAUDIO_DIR%\include\phonon.h" (
        echo ERROR: Steam Audio SDK extracted but "%STEAMAUDIO_DIR%\include\phonon.h" was not found.
        goto :fail
    )
) else (
    echo [2/3] Steam Audio SDK already present, skipping.
)

:: ----------------------------------------------------------------------------
:: 3. vcpkg + glm, yaml-cpp (x64-windows)
:: ----------------------------------------------------------------------------
set "VCPKG_ROOT=%VENDORS_DIR%\vcpkg"
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    if not exist "%VCPKG_ROOT%\.git" (
        echo [3/3] Cloning vcpkg into "%VCPKG_ROOT%"...
        if exist "%VCPKG_ROOT%" rmdir /s /q "%VCPKG_ROOT%"
        git clone https://github.com/Microsoft/vcpkg.git "%VCPKG_ROOT%"
        if errorlevel 1 (
            echo ERROR: Failed to clone vcpkg.
            goto :fail
        )
    ) else (
        echo [3/3] vcpkg repository already cloned, bootstrapping...
    )
    pushd "%VCPKG_ROOT%"
    call bootstrap-vcpkg.bat -disableMetrics
    if errorlevel 1 (
        popd
        echo ERROR: Failed to bootstrap vcpkg.
        goto :fail
    )
    popd
) else (
    echo [3/3] vcpkg executable already present.
)

if not exist "%TEMP%\vcpkg-cache" mkdir "%TEMP%\vcpkg-cache"
set "VCPKG_BINARY_SOURCES=clear;files,%TEMP%\vcpkg-cache,readwrite"

echo Ensuring glm and yaml-cpp ^(x64-windows^) are installed via vcpkg...
"%VCPKG_ROOT%\vcpkg.exe" install glm yaml-cpp --triplet=x64-windows
if errorlevel 1 (
    echo ERROR: Failed to install vcpkg packages.
    goto :fail
)

echo.
echo All NoxAudio dependencies are ready in "%VENDORS_DIR%".
if /i not "%NRI_NO_PAUSE%"=="1" if /i not "%NOXAUDIO_NO_PAUSE%"=="1" pause
exit /b 0

:fail
echo.
echo Dependency installation failed.
if /i not "%NRI_NO_PAUSE%"=="1" if /i not "%NOXAUDIO_NO_PAUSE%"=="1" pause
exit /b 1
