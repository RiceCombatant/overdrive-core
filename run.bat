@echo off
if exist "build\OverdriveCore.exe" (
    echo Launching OverdriveCore...
    start "" "build\OverdriveCore.exe"
) else (
    echo [ERROR] build\OverdriveCore.exe not found. Please run build.bat first.
)
