@echo off
setlocal

set QT_BIN_DIR=E:\qt\6.10.2\mingw_64\bin
set PATH=%QT_BIN_DIR%;%PATH%

if not exist build mkdir build
cd build

echo [INFO] Configuring...
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    exit /b %ERRORLEVEL%
)

echo [INFO] Building...
cmake --build .
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    exit /b %ERRORLEVEL%
)

echo [INFO] Deploying Qt dependencies...
if exist brush-synth.exe (
    echo [INFO] Cleaning old nested DLL configurations...
    if exist dll rmdir /s /q dll
    if exist run.bat del run.bat
    if exist qt.conf del qt.conf

    echo [INFO] Running windeployqt...
    :: Deploy directly to the current directory (where brush-synth.exe is)
    "%QT_BIN_DIR%\windeployqt.exe" brush-synth.exe --no-translations --no-system-d3d-compiler --no-opengl-sw
) else (
    echo [WARNING] brush-synth.exe not found in build directory. Skipping deployment.
)

echo [INFO] Build successful. 
echo [INFO] You can run build\brush-synth.exe directly.
cd ..
endlocal
