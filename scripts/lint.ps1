# Try to find clang tools in PATH first (CI), then fall back to VS paths (local dev)
$clangFormat = Get-Command clang-format -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source

if (-not $clangFormat -or -not $clangTidy) {
    # Fall back to VS installation paths
    $vsPaths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\Llvm\x64\bin",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin",
        "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin"
    )
    foreach ($path in $vsPaths) {
        if (Test-Path (Join-Path $path "clang-format.exe")) {
            $clangFormat = Join-Path $path "clang-format.exe"
            $clangTidy = Join-Path $path "clang-tidy.exe"
            break
        }
    }
}

if (-not $clangFormat -or -not (Test-Path $clangFormat)) {
    Write-Host "Error: clang-format not found" -ForegroundColor Red
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
# Note: clang-tidy might take time and return non-zero exit codes for warnings.
# We use -p build to find compile_commands.json
$ErrorActionPreference = "SilentlyContinue"
$output = & $clangTidy -p build $srcFiles.FullName --quiet --system-headers=0 2>&1 | Out-String
$ErrorActionPreference = "Continue"

# Filter lines - remove noise from Windows SDK header parsing errors
# Keep actual warnings from our source files
$lines = $output -split "`r?`n"
$filtered = $lines | Where-Object {
    $_ -and
    # Remove progress messages
    $_ -notmatch "^\[\d+/\d+\] Processing file" -and
    # Remove cumulative error counts (these are from Windows SDK parsing failures)
    $_ -notmatch "^\d+ warnings? and \d+ errors? generated" -and
    # Remove "Error while processing" lines (Windows SDK parse failures)
    $_ -notmatch "^Error while processing" -and
    # Remove PowerShell NativeCommandError noise
    $_ -notmatch "NativeCommandError" -and
    $_ -notmatch "CategoryInfo" -and
    $_ -notmatch "FullyQualifiedErrorId" -and
    $_ -notmatch "At .+lint\.ps1:" -and
    # Remove "file not found" errors from Windows SDK headers
    $_ -notmatch "'[^']+' file not found" -and
    # Remove false positives for external library naming
    $_ -notmatch "invalid case style for variable '(Microsoft|std|D3D|DXGI)" -and
    # Exclude class/namespace false positives (misidentified as variables due to parse failures)
    -not ($_ -match "invalid case style for variable" -and $_ -match "class |namespace |struct ")
}

# Remove context lines that follow filtered errors (lines starting with spaces and containing |)
$result = @()
$skipNext = $false
foreach ($line in $filtered) {
    if ($line -match "^\s+\d+\s*\|") {
        # This is a code context line, skip if previous was filtered
        continue
    }
    if ($line -match "^\s+\|") {
        # This is a caret line, skip
        continue
    }
    $result += $line
}

$cleanOutput = ($result | Where-Object { $_.Trim() }) -join "`n"

# Check for actual warnings from our source code
$srcWarnings = $result | Where-Object { $_ -match "src[/\\].*warning:" }

if ($srcWarnings) {
    Write-Host ($srcWarnings -join "`n")
    Write-Host "`nClang-Tidy found issues in source code." -ForegroundColor Red
    exit 1
} elseif ($cleanOutput) {
    Write-Host $cleanOutput
    Write-Host "`nClang-Tidy reported issues (non-blocking)." -ForegroundColor Yellow
} else {
    Write-Host "Linting successful. No issues found." -ForegroundColor Green
}
