@echo off
setlocal
set "DOCS_OUTPUT_DIR=docs"
set "DOXYGEN_OUTPUT_DIR=%DOCS_OUTPUT_DIR%\doxygen"
set "TEMP_DOXYFILE=.tmp_doxyfile"

echo ===================================================
echo [DOCS] Publishing Doxygen Documentation...
echo [DOCS] Output: %DOCS_OUTPUT_DIR%
echo ===================================================

where doxygen >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] doxygen command not found. Please install Doxygen and add it to PATH.
    exit /b 1
)

if exist "%DOXYGEN_OUTPUT_DIR%" (
    rmdir /s /q "%DOXYGEN_OUTPUT_DIR%"
)

powershell -NoProfile -Command "$content = Get-Content 'Doxyfile'; $content = $content -replace '^OUTPUT_DIRECTORY\\s*=.*$', 'OUTPUT_DIRECTORY       = \"%DOXYGEN_OUTPUT_DIR%\"'; Set-Content -Path '%TEMP_DOXYFILE%' -Value $content"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to prepare temporary Doxygen configuration.
    exit /b 1
)

doxygen %TEMP_DOXYFILE%

if %errorlevel% equ 0 (
    echo ===================================================
    copy /y docsrc\index.html "%DOCS_OUTPUT_DIR%\index.html" >nul
    del /q %TEMP_DOXYFILE% >nul 2>&1
    echo [SUCCESS] Documentation published at: %DOCS_OUTPUT_DIR%/doxygen/html/index.html
    echo ===================================================
    exit /b 0
) else (
    del /q %TEMP_DOXYFILE% >nul 2>&1
    echo [ERROR] Doxygen generation failed.
    exit /b %errorlevel%
)

endlocal
