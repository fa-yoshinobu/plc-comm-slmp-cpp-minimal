@echo off
setlocal

set "RUN_PLATFORMIO=0"
set "VERSION="
set "JSON_VERSION="

:parse
if "%~1"=="" goto main
if /i "%~1"=="--help" goto usage
if /i "%~1"=="--with-platformio" (
    set "RUN_PLATFORMIO=1"
    shift
    goto parse
)
>&2 echo Unknown argument: %~1
goto usage

:main
for /f "tokens=1,2 delims==" %%A in (library.properties) do (
    if /I "%%A"=="version" set "VERSION=%%B"
)
if not defined VERSION (
    echo [ERROR] Failed to read version from library.properties.
    exit /b 1
)

for /f "usebackq delims=" %%V in (`powershell -NoProfile -Command "(Get-Content library.json -Raw | ConvertFrom-Json).version"`) do set "JSON_VERSION=%%V"
if not defined JSON_VERSION (
    echo [ERROR] Failed to read version from library.json.
    exit /b 1
)

echo ===================================================
echo [RELEASE] SLMP minimal C++ release check
echo ===================================================

echo [1/4] Updating canonical SLMP profile JSON...
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\update_slmp_profile_jsons.ps1 -FailIfChanged
if %errorlevel% neq 0 (
    echo [ERROR] Canonical SLMP profile JSON check failed.
    exit /b %errorlevel%
)

echo [2/5] Checking C++ device range catalog parity...
python scripts\check_device_range_catalog_parity.py
if %errorlevel% neq 0 (
    echo [ERROR] Device range catalog parity check failed.
    exit /b %errorlevel%
)

echo [3/5] Checking manifest versions...
if not "%VERSION%"=="%JSON_VERSION%" (
    echo [ERROR] library.properties version %VERSION% does not match library.json version %JSON_VERSION%.
    exit /b 1
)

echo [4/5] Running host CI gate...
call run_ci.bat
if %errorlevel% neq 0 (
    echo [ERROR] CI failed.
    exit /b %errorlevel%
)

if "%RUN_PLATFORMIO%"=="1" (
    echo [platformio] Checking registry duplicate and embedded samples...
    powershell -NoProfile -ExecutionPolicy Bypass -File scripts\check_registry_duplicate.ps1 -Registry platformio -Package fa-yoshinobu/slmp-connect-cpp-minimal -VersionSource library-properties -ManifestPath library.properties -CompareSource library-json -CompareManifestPath library.json
    if %errorlevel% neq 0 (
        echo [ERROR] PlatformIO registry check failed.
        exit /b %errorlevel%
    )

    call run_ci.bat --with-platformio
    if %errorlevel% neq 0 (
        echo [ERROR] PlatformIO sample gate failed.
        exit /b %errorlevel%
    )
)

echo [5/5] Building tracked release archive...
if not exist release-artifacts mkdir release-artifacts
git archive --format=zip --output "release-artifacts\slmp-connect-cpp-minimal-v%VERSION%.zip" HEAD
if %errorlevel% neq 0 (
    echo [ERROR] Failed to build release archive.
    exit /b %errorlevel%
)

echo ===================================================
echo [SUCCESS] Release check passed.
echo ===================================================
endlocal
exit /b 0

:usage
echo usage: %~nx0 [--with-platformio]
exit /b 2
