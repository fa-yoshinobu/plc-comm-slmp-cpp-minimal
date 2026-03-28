@echo off
setlocal

set "PLATFORMIO_CORE_DIR=%CD%\.platformio-home"
if not exist "%PLATFORMIO_CORE_DIR%" mkdir "%PLATFORMIO_CORE_DIR%"

set "PATH=%USERPROFILE%\.platformio\penv\Scripts;%PATH%"

set "STAGE=release-artifacts\platformio-stage"
set "OUTDIR=release-artifacts"

if exist "%STAGE%" rmdir /s /q "%STAGE%"
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

mkdir "%STAGE%"
mkdir "%STAGE%\src"
mkdir "%STAGE%\examples"
mkdir "%STAGE%\examples\esp32_devkitc_low_level"
mkdir "%STAGE%\examples\esp32_devkitc_high_level"
mkdir "%STAGE%\examples\high_level_snapshot"

copy /y README.md "%STAGE%\" >nul
copy /y LICENSE "%STAGE%\" >nul
copy /y library.json "%STAGE%\" >nul
copy /y library.properties "%STAGE%\" >nul
copy /y src\*.h "%STAGE%\src\" >nul
copy /y src\*.cpp "%STAGE%\src\" >nul
copy /y examples\README.md "%STAGE%\examples\" >nul
copy /y examples\esp32_devkitc_low_level\* "%STAGE%\examples\esp32_devkitc_low_level\" >nul
copy /y examples\esp32_devkitc_high_level\* "%STAGE%\examples\esp32_devkitc_high_level\" >nul
copy /y examples\high_level_snapshot\* "%STAGE%\examples\high_level_snapshot\" >nul

echo ===================================================
echo [PIO] Packing PlatformIO library from clean stage...
echo ===================================================
pio pkg pack "%STAGE%" -o "%OUTDIR%"
if %errorlevel% neq 0 (
    echo [ERROR] PlatformIO package pack failed.
    exit /b %errorlevel%
)

echo ===================================================
echo [SUCCESS] PlatformIO package created in %OUTDIR%
echo ===================================================
endlocal
