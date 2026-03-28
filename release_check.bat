@echo off
setlocal

set "VERSION="
for /f "tokens=1,2 delims==" %%A in (library.properties) do (
    if /I "%%A"=="version" set "VERSION=%%B"
)
if not defined VERSION (
    echo [ERROR] Failed to read version from library.properties.
    exit /b 1
)

echo ===================================================
echo [RELEASE] SLMP minimal C++ release check
echo ===================================================

echo [1/3] Running CI build...
call run_ci.bat
if %errorlevel% neq 0 (
    echo [ERROR] CI failed.
    exit /b %errorlevel%
)

echo [2/3] Building docs...
call build_docs.bat
if %errorlevel% neq 0 (
    echo [ERROR] Docs build failed.
    exit /b %errorlevel%
)

echo [3/3] Building tracked release archive...
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
