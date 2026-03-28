@echo off
setlocal

echo ===================================================
echo [DOCS] Generating Doxygen Documentation...
echo ===================================================

where doxygen >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] doxygen command not found. Please install Doxygen and add it to PATH.
    exit /b 1
)

if exist docs\doxygen (
    rmdir /s /q docs\doxygen
)

doxygen Doxyfile

if %errorlevel% equ 0 (
    echo ===================================================
    copy /y docsrc\index.html docs\index.html >nul
    echo [SUCCESS] Documentation generated at: docs/doxygen/html/index.html
    echo ===================================================
    exit /b 0
) else (
    echo [ERROR] Doxygen generation failed.
    exit /b %errorlevel%
)

endlocal


