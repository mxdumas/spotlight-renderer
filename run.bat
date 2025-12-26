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
    cmake --build build --config %CONFIG%
    if errorlevel 1 exit /b 1
)
.\build\%CONFIG%\SpotlightRenderer.exe
