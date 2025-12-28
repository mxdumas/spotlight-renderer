# Ensure VS environment is loaded (needed for Windows SDK headers)
# On CI, ilammy/msvc-dev-cmd sets this up; locally we need to do it ourselves
if (-not $env:INCLUDE) {
    Write-Host "Loading Visual Studio environment..."
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsPath) {
            $devCmd = Join-Path $vsPath "Common7\Tools\VsDevCmd.bat"
            if (Test-Path $devCmd) {
                # Run VsDevCmd.bat and capture environment variables
                $envVars = cmd /c "`"$devCmd`" -arch=amd64 -no_logo && set"
                foreach ($line in $envVars) {
                    if ($line -match "^([^=]+)=(.*)$") {
                        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
                    }
                }
                Write-Host "VS environment loaded from: $vsPath" -ForegroundColor Green
            }
        }
    }
    if (-not $env:INCLUDE) {
        Write-Host "Warning: Could not load VS environment. Some checks may fail." -ForegroundColor Yellow
    }
}

# Try to find clang tools in PATH first (CI), then fall back to VS paths (local dev)
$clangFormat = Get-Command clang-format -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source

if (-not $clangFormat -or -not $clangTidy) {
    # Fall back to VS installation paths (use vswhere for reliability)
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsPath) {
            $llvmPath = Join-Path $vsPath "VC\Tools\Llvm\x64\bin"
            if (Test-Path (Join-Path $llvmPath "clang-format.exe")) {
                $clangFormat = Join-Path $llvmPath "clang-format.exe"
                $clangTidy = Join-Path $llvmPath "clang-tidy.exe"
            }
        }
    }
}

if (-not $clangFormat -or -not (Test-Path $clangFormat)) {
    Write-Host "Error: clang-format not found" -ForegroundColor Red
    exit 1
}

if (-not $clangTidy -or -not (Test-Path $clangTidy)) {
    Write-Host "Error: clang-tidy not found" -ForegroundColor Red
    exit 1
}

Write-Host "Using clang-format: $clangFormat"
Write-Host "Using clang-tidy: $clangTidy"

if (-not (Test-Path "build\compile_commands.json")) {
    Write-Host "Error: build\compile_commands.json not found. Run cmake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON" -ForegroundColor Red
    exit 1
}

$srcFiles = Get-ChildItem -Path "src" -Recurse -Include *.cpp, *.h, *.hpp | Where-Object { $_.FullName -notmatch "external" }

if ($null -eq $srcFiles) {
    Write-Host "No source files found to lint."
    exit 0
}

Write-Host "Formatting $($srcFiles.Count) files with clang-format..."
& $clangFormat -i $srcFiles.FullName

Write-Host "Analyzing files with clang-tidy..."
Write-Host "PowerShell version: $($PSVersionTable.PSVersion)"

# Header filter: only report diagnostics from our src directory
# Use simple pattern that works on both Windows and Unix paths
$headerFilter = ".*/src/.*"

# Note: clang-tidy might take time and return non-zero exit codes for warnings.
# We use -p build to find compile_commands.json
$jobs = [Environment]::ProcessorCount

# Check if PowerShell 7+ is available for parallel execution
if ($PSVersionTable.PSVersion.Major -ge 7) {
    Write-Host "Running clang-tidy on $($srcFiles.Count) files (parallel, $jobs jobs)..."

    $output = $srcFiles | ForEach-Object -Parallel {
        & $using:clangTidy -p build $_.FullName --quiet --header-filter=$using:headerFilter --system-headers=0 2>&1
    } -ThrottleLimit $jobs | Out-String
} else {
    Write-Host "Running clang-tidy on $($srcFiles.Count) files (sequential, PS $($PSVersionTable.PSVersion.Major))..."
    $output = & $clangTidy -p build $srcFiles.FullName --quiet --header-filter=$headerFilter --system-headers=0 2>&1 | Out-String
}

# Filter clang-tidy output noise
$lines = $output -split "`r?`n"
$filtered = $lines | Where-Object {
    $_ -and
    $_ -notmatch "^\d+ warnings?( and \d+ errors?)? generated" -and
    $_ -notmatch "^error: too many errors emitted" -and
    $_ -notmatch "^Error while processing" -and
    $_ -notmatch "^Suppressed \d+ warnings" -and
    # PowerShell stderr noise
    $_ -notmatch "NativeCommandError|CategoryInfo|FullyQualifiedErrorId"
}

# Extract warnings from our src files
$srcWarnings = $filtered | Where-Object { $_ -match "[/\\]src[/\\].*warning:" }

if ($srcWarnings) {
    Write-Host ($srcWarnings -join "`n")
    Write-Host "`nClang-Tidy found issues in source code." -ForegroundColor Red
    exit 1
} else {
    Write-Host "Linting successful. No issues found." -ForegroundColor Green
}
