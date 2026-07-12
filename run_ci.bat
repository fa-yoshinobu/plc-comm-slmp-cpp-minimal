@echo off
setlocal

set "STATUS=0"
set "RUN_PLATFORMIO=0"
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%.") do set "REPO_ROOT=%%~fI"
set "TEST_EXE=%TEMP%\slmp_minimal_tests.exe"
set "LIVE_READ_EXE=%TEMP%\slmp_live_read_once.exe"
set "MATRIX_VALIDATION_EXE=%TEMP%\slmp_matrix_validation.exe"
set "ARDUINO_TRANSPORT_TEST_EXE=%TEMP%\slmp_arduino_transport_tests.exe"

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
pushd "%REPO_ROOT%" >nul

call :prepend_common_tool_paths
call :find_tool g++.exe CXX_EXE
if errorlevel 1 goto fail
call :find_python
if errorlevel 1 goto fail

echo ===================================================
echo [CI] SLMP minimal C++ local gate
echo ===================================================

echo [1/9] Building host tests...
call "%CXX_EXE%" -std=c++17 -Wall -Wextra -Isrc tests/slmp_minimal_tests.cpp src/slmp_minimal.cpp src/slmp_error_codes.cpp src/slmp_high_level.cpp -o "%TEST_EXE%"
if errorlevel 1 goto fail

echo [2/9] Building Arduino transport tests...
call "%CXX_EXE%" -std=c++17 -Wall -Wextra -Itests\arduino_stubs -Isrc tests\slmp_arduino_transport_tests.cpp -o "%ARDUINO_TRANSPORT_TEST_EXE%"
if errorlevel 1 goto fail

echo [3/9] Running Arduino transport tests...
call "%ARDUINO_TRANSPORT_TEST_EXE%"
if not "%errorlevel%"=="0" goto fail

echo [4/9] Building live read smoke tool...
call "%CXX_EXE%" -std=c++17 -Wall -Wextra -Isrc tests/slmp_live_read_once.cpp src/slmp_minimal.cpp src/slmp_error_codes.cpp src/slmp_high_level.cpp -o "%LIVE_READ_EXE%" -lws2_32
if errorlevel 1 goto fail

echo [5/9] Checking live read smoke tool usage...
call "%LIVE_READ_EXE%" >nul 2>nul
if errorlevel 3 goto fail
if not errorlevel 2 goto fail

echo [6/9] Building matrix validation tool...
call "%CXX_EXE%" -std=c++17 -Wall -Wextra -Isrc tests/slmp_matrix_validation.cpp src/slmp_minimal.cpp src/slmp_error_codes.cpp src/slmp_high_level.cpp -o "%MATRIX_VALIDATION_EXE%" -lws2_32
if errorlevel 1 goto fail

echo [7/9] Checking matrix validation tool usage...
call "%MATRIX_VALIDATION_EXE%" >nul 2>nul
if errorlevel 3 goto fail
if not errorlevel 2 goto fail

echo [8/9] Validating API reference...
call "%PYTHON_EXE%" %PYTHON_ARGS% scripts\generate_api_reference.py --title "SLMP C++ API Reference" --output docsrc\user\API_REFERENCE.md --input src\slmp_minimal.h --input src\slmp_high_level.h --input src\slmp_arduino_transport.h --input src\slmp_error_codes.h --input src\slmp_utility.h --predefine SLMP_MINIMAL_ENABLE_HIGH_LEVEL=1 --predefine SLMP_ENABLE_UDP_TRANSPORT=1 --check
if errorlevel 1 goto fail

echo [9/9] Running host tests...
call "%TEST_EXE%"
if not "%errorlevel%"=="0" goto fail

echo [post] Running socket integration test...
call "%PYTHON_EXE%" %PYTHON_ARGS% scripts\run_socket_integration.py --compiler "%CXX_EXE%"
if errorlevel 1 goto fail

if "%RUN_PLATFORMIO%"=="1" (
    call :run_platformio
    if errorlevel 1 goto fail
)

echo ===================================================
echo [SUCCESS] CI passed.
echo ===================================================
goto finish

:prepend_common_tool_paths
call :prepend_optional_tool_path SLMP_MSYS2_UCRT64_BIN
call :prepend_optional_tool_path SLMP_MSYS2_MINGW64_BIN
call :prepend_optional_tool_path MSYS2_UCRT64_BIN
call :prepend_optional_tool_path MSYS2_MINGW64_BIN
exit /b 0

:prepend_optional_tool_path
set "TOOL_PATH="
call set "TOOL_PATH=%%%~1%%"
if not "%TOOL_PATH%"=="" if exist "%TOOL_PATH%" set "PATH=%TOOL_PATH%;%PATH%"
exit /b 0

:find_tool
set "%~2="
for %%I in (%~1) do if not "%%~$PATH:I"=="" (
    set "%~2=%%~$PATH:I"
    exit /b 0
)
>&2 echo %~1 not found.
exit /b 1

:find_python
set "PYTHON_EXE="
set "PYTHON_ARGS="
py -3 -c "import sys; print(sys.version)" >nul 2>nul
if not errorlevel 1 (
    set "PYTHON_EXE=py"
    set "PYTHON_ARGS=-3"
    exit /b 0
)

python -c "import sys; print(sys.version)" >nul 2>nul
if not errorlevel 1 (
    set "PYTHON_EXE=python"
    exit /b 0
)

>&2 echo Python not found.
exit /b 1

:run_platformio
set "PLATFORMIO_CORE_DIR=%REPO_ROOT%\.platformio-home"
if not exist "%PLATFORMIO_CORE_DIR%" mkdir "%PLATFORMIO_CORE_DIR%"

where pio >nul 2>nul
if errorlevel 1 (
    if exist "%USERPROFILE%\.platformio\penv\Scripts" (
        set "PATH=%USERPROFILE%\.platformio\penv\Scripts;%PATH%"
    )
)

call :find_tool pio.exe PIO_EXE
if errorlevel 1 (
    >&2 echo PlatformIO not found. Install pio or omit --with-platformio.
    exit /b 1
)

echo [platformio] Building ESP32 samples...
call "%PIO_EXE%" run -e esp32-devkitc-low-level -e esp32-devkitc-high-level
if errorlevel 1 exit /b 1

echo [platformio] Running static analysis for all configured environments...
call "%PIO_EXE%" check --severity medium --fail-on-defect medium
if errorlevel 1 exit /b 1

exit /b 0

:fail
set "STATUS=%errorlevel%"

:finish
if exist "%TEST_EXE%" del "%TEST_EXE%" >nul 2>nul
if exist "%LIVE_READ_EXE%" del "%LIVE_READ_EXE%" >nul 2>nul
if exist "%MATRIX_VALIDATION_EXE%" del "%MATRIX_VALIDATION_EXE%" >nul 2>nul
if exist "%ARDUINO_TRANSPORT_TEST_EXE%" del "%ARDUINO_TRANSPORT_TEST_EXE%" >nul 2>nul
if defined REPO_ROOT if exist "%REPO_ROOT%" popd >nul 2>nul
exit /b %STATUS%

:usage
echo usage: %~nx0 [--with-platformio]
set "STATUS=2"
goto finish
