@echo off
cd /d "%~dp0"
if exist "build\OverdriveCore.exe" (
    echo ========================================================
    echo  Launching Overdrive Core (OpenXR PCVR ^& Desktop Mode)
    echo ========================================================
    cd build
    OverdriveCore.exe
    cd ..
) else (
    echo [ERROR] build\OverdriveCore.exe not found. Please run build.bat first.
    pause
)
