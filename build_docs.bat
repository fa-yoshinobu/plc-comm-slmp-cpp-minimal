@echo off
setlocal

echo ===================================================
echo [DOCS] Generating Doxygen Documentation...
echo ===================================================

where doxygen >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] doxygen command not found. Please install Doxygen and add it to PATH.
)

doxygen Doxyfile

if %errorlevel% equ 0 (
    echo ===================================================
    copy /y docsrc\index.html docs\index.html >nul
echo [SUCCESS] Documentation generated at: docs/doxygen/html/index.html
    echo ===================================================
) else (
    echo [ERROR] Doxygen generation failed.
)

endlocal


