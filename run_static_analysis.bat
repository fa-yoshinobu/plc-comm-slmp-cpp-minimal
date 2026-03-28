@echo off
setlocal

set "PLATFORMIO_CORE_DIR=%CD%\.platformio-home"
if not exist "%PLATFORMIO_CORE_DIR%" mkdir "%PLATFORMIO_CORE_DIR%"

echo ===================================================
echo [CHECK] Starting PlatformIO Static Analysis...
echo ===================================================

where pio >nul 2>&1
if %errorlevel% neq 0 (
    if exist "%USERPROFILE%\.platformio\penv\Scripts" (
        set "PATH=%USERPROFILE%\.platformio\penv\Scripts;%PATH%"
    )
)

pio check -e esp32-devkitc-low-level -e esp32-devkitc-high-level --severity medium --fail-on-defect medium
if %errorlevel% neq 0 (
    echo [ERROR] Static analysis found issues.
    exit /b %errorlevel%
)

echo ===================================================
echo [SUCCESS] PlatformIO static analysis passed!
echo ===================================================
endlocal
