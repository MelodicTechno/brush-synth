@echo off
setlocal

cd build
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build directory not found. Please run build.bat first.
    exit /b 1
)

if exist brush-synth.exe (
    brush-synth.exe
) else (
    echo [ERROR] Executable not found. Please run build.bat first.
    exit /b 1
)

cd ..
endlocal
