@echo off
setlocal

set CONFIG=Debug
if "%1"=="--release" set CONFIG=Release

echo Configuring CMake...
cmake -B build -S .
if %ERRORLEVEL% neq 0 (
    echo Configuration failed!
    exit /b %ERRORLEVEL%
)

echo.
echo Building %CONFIG%...
cmake --build build --config %CONFIG%
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo.
echo Build completed successfully!
