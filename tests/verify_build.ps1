$ErrorActionPreference = "Stop"

$cmakePath = "cmake"
if (-not (Get-Command $cmakePath -ErrorAction SilentlyContinue)) {
    if (Test-Path "C:\Program Files\CMake\bin\cmake.exe") {
        $cmakePath = "C:\Program Files\CMake\bin\cmake.exe"
    } else {
        throw "CMake not found."
    }
}

try {
    if (Test-Path "build") {
        Remove-Item -Path "build" -Recurse -Force | Out-Null
    }
    New-Item -ItemType Directory -Force -Path "build" | Out-Null
    Push-Location "build"
    & $cmakePath -G "Visual Studio 18 2026" -A x64 ..
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed."
    }
    Write-Host "CMake configuration succeeded."
} catch {
    Write-Error $_
    exit 1
} finally {
    Pop-Location
}
