@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > nul
set "PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

if "%1"=="clean" (
    echo Cleaning build directory...
    if exist build rmdir /s /q build
    if exist build_release rmdir /s /q build_release
)

set "BUILD_DIR=build"
set "BUILD_TYPE=Debug"

if "%1"=="release" (
    set "BUILD_DIR=build_release"
    set "BUILD_TYPE=Release"
)

if not exist %BUILD_DIR% (
    echo Configuring project with Ninja - %BUILD_TYPE%...
    cmake -G Ninja -B %BUILD_DIR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_CXX_COMPILER=cl -DCMAKE_C_COMPILER=cl
    if errorlevel 1 exit /b 1
)

echo Building OverdriveCore - %BUILD_TYPE%...
cmake --build %BUILD_DIR%
if errorlevel 1 exit /b 1

echo Build finished successfully!
endlocal
