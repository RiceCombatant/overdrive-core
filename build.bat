@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > nul
set "PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

if "%1"=="clean" (
    echo Cleaning build directory...
    rmdir /s /q build
)

if not exist build (
    echo Configuring project with Ninja...
    cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
    if errorlevel 1 exit /b 1
)

echo Building OverdriveCore...
cmake --build build
if errorlevel 1 exit /b 1

echo Build finished successfully!
endlocal
