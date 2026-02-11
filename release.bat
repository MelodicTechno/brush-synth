@echo off
setlocal

set QT_BIN_DIR=E:\qt\6.10.2\mingw_64\bin
set PATH=%QT_BIN_DIR%;%PATH%

echo [INFO] Setting up Release build environment...
if not exist build_release mkdir build_release
cd build_release

echo [INFO] Configuring (Release)...
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

cd ..
if not exist release mkdir release
echo [INFO] Copying executable to release folder...
copy /Y build_release\brush-synth.exe release\brush-synth.exe

cd release
echo [INFO] Deploying Qt dependencies...
:: Clean up previous messy deployments if any
if exist dll rmdir /s /q dll
if exist run.bat del run.bat
if exist qt.conf del qt.conf

:: Deploy directly to the release folder
"%QT_BIN_DIR%\windeployqt.exe" brush-synth.exe --no-translations --no-system-d3d-compiler --no-opengl-sw

echo [INFO] Release build created in 'release' folder.
echo [INFO] You can now directly click brush-synth.exe to run.

cd ..
endlocal
