# project: SeamlyLayout
# author: slspencer, copyright 2026
# MIT License: https://opensource.org/licenses/MIT
#
# build_installer.ps1 — Build the SeamlyLayout Windows installer
#
# Steps performed:
#   1. Verify prerequisites (Qt, Inno Setup, LGPL license file)
#   2. Build a Release executable via qr.ps1 (if not already built)
#   3. Run windeployqt6 to gather Qt runtime DLLs alongside the exe
#   4. Run iscc.exe (Inno Setup Compiler) to produce the installer
#
# Output: packaging\windows\Output\SeamlyLayout-0.1.0-win64.exe
#
# Runtime folders (written by the app at first launch, not by this script):
#   %LOCALAPPDATA%\Seamly\SeamlyLayout\settings\      -- layout settings JSON files
#   %LOCALAPPDATA%\Seamly\SeamlyLayout\preferences\   -- user preferences JSON file
#
# Legacy migration note:
#   Task 15 (2026-07): the organization name changed from "Seamly Systems" to the
#   shared "Seamly" (matching seamly2d/seamlyme); the first launch after upgrading
#   copies every settings/preferences file forward from
#   %LOCALAPPDATA%\Seamly Systems\SeamlyLayout\ into the new location automatically.
#   Upgrading from a pre-0.1.0 build: the first launch also copies any files found
#   in "layout-settings" or "layout-preferences" folders to the new canonical
#   folder names.

param(
    [switch]$SkipBuild,   # Skip the cmake/cargo build step (exe already built)
    [switch]$SkipDeploy   # Skip windeployqt6 (DLLs already deployed)
)

$ErrorActionPreference = "Stop"

$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot   = (Resolve-Path (Join-Path $ScriptDir "..\..\..")).Path
$BuildDir   = Join-Path $RepoRoot "qt_frontend\build\Release"
$ExePath    = Join-Path $BuildDir "SeamlyLayout.exe"
$QtBin      = "C:\Qt\6.11.1\msvc2022_64\bin"
$IsccPath   = "C:\Program Files (x86)\Inno Setup 6\iscc.exe"
$IssScript  = Join-Path $ScriptDir "SeamlyLayout.iss"
$LgplFile   = Join-Path $ScriptDir "..\licenses\LGPL-3.0.txt"

# ---------------------------------------------------------------------------
# 1. Verify prerequisites
# ---------------------------------------------------------------------------
Write-Host "1 Verifying prerequisites..."

if (-not (Test-Path $QtBin)) {
    Write-Error "Qt 6.11.1 msvc2022_64 not found at: $QtBin"
    exit 1
}

if (-not (Test-Path $IsccPath)) {
    Write-Error "Inno Setup 6 iscc.exe not found at: $IsccPath`n" +
                "Download from https://jrsoftware.org/isinfo.php"
    exit 1
}

if (-not (Test-Path $LgplFile)) {
    Write-Error "LGPL-3.0.txt not found at: $LgplFile`n" +
                "Download from https://www.gnu.org/licenses/lgpl-3.0.txt`n" +
                "and save as packaging\licenses\LGPL-3.0.txt"
    exit 1
}

# ---------------------------------------------------------------------------
# 2. Build Release executable
# ---------------------------------------------------------------------------
if (-not $SkipBuild) {
    Write-Host "2 Building Release executable..."
    & (Join-Path $RepoRoot "qr.ps1")
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Release build failed (exit $LASTEXITCODE)"
        exit $LASTEXITCODE
    }
} else {
    Write-Host "2 Skipping build (SkipBuild flag set)"
}

if (-not (Test-Path $ExePath)) {
    Write-Error "Executable not found after build: $ExePath"
    exit 1
}

# ---------------------------------------------------------------------------
# 3. Run windeployqt6 to gather Qt runtime DLLs
# ---------------------------------------------------------------------------
if (-not $SkipDeploy) {
    Write-Host "3 Running windeployqt6..."
    $WinDeployQt = Join-Path $QtBin "windeployqt6.exe"
    & $WinDeployQt `
        --qmldir (Join-Path $RepoRoot "qt_frontend\qml") `
        --release `
        $ExePath
    if ($LASTEXITCODE -ne 0) {
        Write-Error "windeployqt6 failed (exit $LASTEXITCODE)"
        exit $LASTEXITCODE
    }
} else {
    Write-Host "3 Skipping windeployqt6 (SkipDeploy flag set)"
}

# ---------------------------------------------------------------------------
# 4. Run Inno Setup Compiler
# ---------------------------------------------------------------------------
Write-Host "4 Running Inno Setup Compiler..."
$OutputDir = Join-Path $ScriptDir "Output"
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

& $IsccPath $IssScript
if ($LASTEXITCODE -ne 0) {
    Write-Error "Inno Setup Compiler failed (exit $LASTEXITCODE)"
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "=== Installer built successfully ===" -ForegroundColor Green
$Installer = Get-ChildItem -Path $OutputDir -Filter "*.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($Installer) {
    Write-Host "Output: $($Installer.FullName)" -ForegroundColor Cyan
}
