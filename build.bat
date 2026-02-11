@echo off
setlocal

set QT_BIN_DIR=E:\qt\6.10.2\mingw_64\bin
set PATH=%QT_BIN_DIR%;%PATH%

if not exist build mkdir build
cd build

echo [INFO] Configuring...
cmake -G "Ninja" ..
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    exit /b %ERRORLEVEL%
)

echo [INFO] Building...
cmake --build . --config Debug
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    exit /b %ERRORLEVEL%
)

echo [INFO] Deploying Qt dependencies...
if exist brush-synth.exe (
    "%QT_BIN_DIR%\windeployqt.exe" brush-synth.exe --no-translations --no-system-d3d-compiler
) else (
    echo [WARNING] brush-synth.exe not found in build directory. Skipping deployment.
)

echo [INFO] Build successful.
cd ..
endlocal
