@echo off
setlocal

set "PLATFORMIO_CORE_DIR=%CD%\.platformio-home"
if not exist "%PLATFORMIO_CORE_DIR%" mkdir "%PLATFORMIO_CORE_DIR%"

echo ===================================================
echo [CI] Starting PlatformIO Build...
echo ===================================================

REM Try to add default PlatformIO path to PATH if pio is not found
where pio >nul 2>&1
if %errorlevel% neq 0 (
    echo [INFO] pio not found in PATH. Searching in default location...
    if exist "%USERPROFILE%\.platformio\penv\Scripts" (
        set "PATH=%USERPROFILE%\.platformio\penv\Scripts;%PATH%"
        echo [INFO] Added PlatformIO to PATH.
    )
)

echo [1/1] Building Project (pio run)...
pio run -e esp32-devkitc-low-level -e esp32-devkitc-high-level
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    exit /b %errorlevel%
)

echo ===================================================
echo [SUCCESS] PlatformIO build checks passed!
echo ===================================================
endlocal
