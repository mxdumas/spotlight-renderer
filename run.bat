@echo off
setlocal

set CONFIG=Debug
set DO_BUILD=0

:parse_args
if "%1"=="" goto run
if "%1"=="--release" set CONFIG=Release
if "%1"=="--build" set DO_BUILD=1
shift
goto parse_args

:run
if %DO_BUILD%==1 (
    echo Building project...
    call build.bat %*
    if errorlevel 1 (
        echo Build failed. Aborting run.
        exit /b 1
    )
)

:: Try to find the executable in different possible locations
set EXE_PATH=

:: 1. Ninja / Single-config location
if exist "build\SpotlightRenderer.exe" (
    set EXE_PATH=build\SpotlightRenderer.exe
)
:: 2. Visual Studio / Multi-config location
if exist "build\%CONFIG%\SpotlightRenderer.exe" (
    set EXE_PATH=build\%CONFIG%\SpotlightRenderer.exe
)

if "%EXE_PATH%"=="" (
    echo Error: Could not find SpotlightRenderer.exe. Did you build the project?
    exit /b 1
)

echo Running %EXE_PATH%...
%EXE_PATH%