@echo off
setlocal

echo ===================================================
echo [CI] Starting PlatformIO Build and Static Analysis...
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

echo [1/2] Building Project (pio run)...
pio run
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    exit /b %errorlevel%
)

echo [2/2] Running Static Analysis (pio check)...
pio check
if %errorlevel% neq 0 (
    echo [ERROR] Static analysis found issues.
    exit /b %errorlevel%
)

echo ===================================================
echo [SUCCESS] All C++ CI checks passed!
echo ===================================================
endlocal
