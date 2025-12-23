$clangPath = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin"
$clangFormat = Join-Path $clangPath "clang-format.exe"
$clangTidy = Join-Path $clangPath "clang-tidy.exe"

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
& $clangTidy -p build $srcFiles.FullName --quiet

if ($LASTEXITCODE -ne 0) {
    Write-Host "Clang-Tidy reported issues." -ForegroundColor Yellow
} else {
    Write-Host "Linting successful." -ForegroundColor Green
}
