@echo off
setlocal enabledelayedexpansion

set CONFIG=Debug
set GENERATOR=
set NINJA_FLAG=0
set EXTRA_ARGS=

:parse_args
if "%1"=="" goto end_parse
if "%1"=="--release" (
    set CONFIG=Release
    shift
    goto parse_args
)
if "%1"=="--ninja" (
    set GENERATOR=-G Ninja
    set NINJA_FLAG=1
    shift
    goto parse_args
)
shift
goto parse_args
:end_parse

:: Detect vcpkg toolchain
set TOOLCHAIN=
if exist "vcpkg\scripts\buildsystems\vcpkg.cmake" (
    set TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE="vcpkg\scripts\buildsystems\vcpkg.cmake"
)

:: For Ninja, we must specify the build type during configuration
if %NINJA_FLAG%==1 (
    set EXTRA_ARGS=-DCMAKE_BUILD_TYPE=%CONFIG% -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

:: Detect VS installation
set "VS_PATH="

:: Try vswhere first
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i\Common7\Tools\VsDevCmd.bat"
    )
)

:: Fallback paths if vswhere fails or returns nothing
if not defined VS_PATH (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    if exist "C:\Program Files\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" set "VS_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
)

if not defined VS_PATH (
    echo Error: VsDevCmd.bat not found. Please ensure Visual Studio is installed.
    exit /b 1
)

if not exist "%VS_PATH%" (
    echo Error: Found path "%VS_PATH%" but file does not exist.
    exit /b 1
)

echo Using Visual Studio environment from: "%VS_PATH%"
call "%VS_PATH%" -arch=amd64 -no_logo

echo Configuring CMake (Config: %CONFIG%, Generator: %GENERATOR%)...
cmake -B build -S . %GENERATOR% %TOOLCHAIN% %EXTRA_ARGS%
if %ERRORLEVEL% neq 0 (
    echo Configuration failed!
    exit /b %ERRORLEVEL%
)

echo.
echo Building %CONFIG%...
cmake --build build --config %CONFIG% --parallel
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo.
echo Build completed successfully!