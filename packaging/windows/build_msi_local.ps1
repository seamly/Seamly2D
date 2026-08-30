#******************************************************************************
# **  @file   build_msi_local.ps1
# **  @author slspencer
# **  @date   August 28, 2026
# **
# **  @brief
# **  Local, developer-machine build of the Windows x64 MSI: builds seamly2d,
# **  seamlyme and SeamlyLayout release binaries, then runs smsi.ps1 against
# **  them. packaging\windows\README.md records the project's
# **  documented "CI builds the MSI" decision (a dev-machine default can carry
# **  the wrong Qt/CRT runtime); this script exists because the local Qt kit is
# **  the same 6.11.1 release CI installs, so that risk does not apply here.
# **  Treat any MSI this script produces as a local dev build, not a release
# **  artifact - releases still go through `gh workflow run ci.yml`.
# **
# ** @copyright
# **  Copyright (C) 2026 Seamly2D Project
# **
# **  @license
# **  GPL-3.0-or-later
#******************************************************************************

<#
.SYNOPSIS
    Build the Windows x64 MSI locally, without CI.

.DESCRIPTION
    Runs the same steps as ci.yml's windows-msi (x64) job, in the same order:
      1. scripts\version.sh <Version> - stamps the build's version into
         src\libs\vmisc\projectversion.{h,cpp}, so seamly2d/seamlyme report
         the version this build was made with, same as every ci.yml build
         job (linux-test, appimage, macos, windows-msi all run this
         unconditionally, not only on release builds).
      2. qmake Seamly.pro -config release CONFIG+=noTests && nmake - builds
         seamly2d.exe and seamlyme.exe with windeployqt already run as a
         post-link step.
      3. cmake --preset release && cmake --build --preset release in
         src\app\seamlylayout\qt_frontend - builds SeamlyLayout.exe.
    Then packaging\windows\smsi.ps1 stages all three and runs
    `wix build`, carrying -Version as the MSI's DisplayVersion/ProductVersion
    too, so the MSI and the binaries it contains agree.

    The version stamp touches git-tracked files
    (src\libs\vmisc\projectversion.{h,cpp}, dist\macx\seamly2d\Info.plist,
    dist\macx\seamlyme\Info.plist). A successful build reverts them via
    `git checkout`, unless those files already carried uncommitted changes
    before the run, in which case they are left alone so unrelated work is
    not discarded.

    Requires an elevated-free MSVC + Qt + Rust dev machine: Visual Studio 18
    Community (VC\Auxiliary\Build\vcvars64.bat), a Qt 6.11.1+ msvc2022_64 kit
    under C:\Qt with qtmultimedia, qtwebengine, qtwebchannel and qtpositioning
    installed, and a stable Rust toolchain on PATH. WiX v6 is installed
    automatically if missing.

.PARAMETER Version
    Project version as YY.M.D.MMMM, stamped into seamly2d/seamlyme via
    scripts\version.sh and used as the MSI's DisplayVersion/ProductVersion.
    Default: computed from the current local time, the same formula ci.yml's
    version job uses.

.PARAMETER SkipValidation
    Passed through to smsi.ps1 - skip the `wix msi validate` ICE pass.

.EXAMPLE
    .\packaging\windows\build_msi_local.ps1
    Full local x64 MSI build with an auto-computed version.
#>

param(
    [string]$Version,
    [switch]$SkipValidation
)

$ErrorActionPreference = 'Stop'
# The script lives in <repo-root>\packaging\windows\, so the repo root
# is two directories up.
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

#------------------------------------------------------------------------------
# @brief  Run a native program without letting stderr abort the script.
#
# Windows PowerShell 5.1 wraps a native program's stderr lines in a
# terminating ErrorRecord under $ErrorActionPreference = 'Stop', even when the
# program exits 0 (nmake, cargo and windeployqt all write ordinary progress to
# stderr). Judge success by exit code only.
#------------------------------------------------------------------------------
function Invoke-NativeCommand {
    param([Parameter(Mandatory = $true)][scriptblock]$Command)
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { & $Command } finally { $ErrorActionPreference = $previous }
}

# --- Version ------------------------------------------------------------------
if (-not $Version) {
    $now = Get-Date
    $Version = "$($now.Year - 2000).$($now.Month).$($now.Day).$($now.Hour * 60 + $now.Minute)"
}
Write-Host "version: $Version"

# scripts\version.sh writes the version into these git-tracked files, the
# same way every ci.yml build job does. The stamp is reverted (via
# `git checkout`) once the build succeeds, unless one of these files already
# carried uncommitted changes before this run (then it is left alone, to not
# discard unrelated work).
$versionStampedFiles = @(
    'src\libs\vmisc\projectversion.cpp',
    'src\libs\vmisc\projectversion.h',
    'dist\macx\seamly2d\Info.plist',
    'dist\macx\seamlyme\Info.plist'
)

$bash = Get-Command bash -ErrorAction SilentlyContinue
if (-not $bash) {
    throw "scripts\version.sh needs bash + perl (Git for Windows ships both) - install Git for Windows."
}
$versionFilesWereClean = -not (& git -C $repoRoot status --porcelain -- $versionStampedFiles)
Write-Host "stamping version into projectversion.cpp/.h and Info.plist..."
Invoke-NativeCommand { & bash 'packaging/version.sh' $Version }
if ($LASTEXITCODE -ne 0) { throw "packaging/version.sh failed (exit code $LASTEXITCODE)." }

# --- Locate the Qt kit ---------------------------------------------------------
# Same kit selection as src\app\seamlylayout\build.ps1: newest msvc2022_64 kit
# under C:\Qt at or above 6.11.1, so seamly2d/seamlyme/SeamlyLayout all deploy
# against the one Qt runtime the MSI ships.
$QtMinimumVersion = [version]'6.11.1'
$QtRoot = 'C:\Qt'
$QtPath = $null
if (Test-Path $QtRoot) {
    $QtPath = Get-ChildItem -LiteralPath $QtRoot -Directory -ErrorAction SilentlyContinue |
        ForEach-Object {
            $parsed = $null
            if ([version]::TryParse($_.Name, [ref]$parsed)) {
                [pscustomobject]@{ Version = $parsed; Path = (Join-Path $_.FullName 'msvc2022_64') }
            }
        } |
        Where-Object { $_.Version -ge $QtMinimumVersion -and (Test-Path $_.Path) } |
        Sort-Object Version -Descending |
        Select-Object -First 1 -ExpandProperty Path
}
if (-not $QtPath) {
    throw "No Qt $QtMinimumVersion+ msvc2022_64 kit found under '$QtRoot'."
}
$QtBin = Join-Path $QtPath 'bin'
$WinDeployQt = Join-Path $QtBin 'windeployqt.exe'
if (-not (Test-Path $WinDeployQt)) {
    throw "windeployqt.exe not found at '$WinDeployQt'."
}
$MissingQtModules = @('Qt6WebEngineQuick', 'Qt6WebChannel', 'Qt6Positioning') |
    Where-Object { -not (Test-Path (Join-Path $QtPath "lib\cmake\$_")) }
if ($MissingQtModules) {
    throw "Qt kit at '$QtPath' is missing required module(s): $($MissingQtModules -join ', ') - install them via the Qt Maintenance Tool (Add or remove components -> Additional Libraries)."
}
Write-Host "Qt kit: $QtPath"

# --- WiX v6 + extensions --------------------------------------------------------
if (-not (Get-Command wix -ErrorAction SilentlyContinue)) {
    Write-Host "installing WiX v6..."
    Invoke-NativeCommand { & dotnet tool install --global wix --version '6.*' }
    if ($LASTEXITCODE -ne 0) { throw "dotnet tool install wix failed (exit code $LASTEXITCODE)." }
}
$wixVer = (& wix --version).ToString().Split('+')[0]
$installedExtensions = (& wix extension list --global 2>$null)
foreach ($ext in @('WixToolset.UI.wixext', 'WixToolset.Util.wixext')) {
    if (-not ($installedExtensions -match [regex]::Escape($ext))) {
        Write-Host "installing $ext/$wixVer..."
        Invoke-NativeCommand { & wix extension add --global "$ext/$wixVer" }
        if ($LASTEXITCODE -ne 0) { throw "wix extension add $ext failed (exit code $LASTEXITCODE)." }
    }
}

# --- Build seamly2d, seamlyme and SeamlyLayout under vcvars64 -------------------
# One cmd.exe batch, the same shape as build.ps1's temp-batch approach: vcvars64
# sets VCToolsRedistDir (which smsi.ps1 needs) in this process's environment, so
# smsi.ps1 must run as a child of the SAME batch, not a separate PowerShell.
$VsPath = 'C:\Program Files\Microsoft Visual Studio\18\Community'
$VcVarsAll = "$VsPath\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $VcVarsAll)) {
    throw "VS 18 Community vcvars64.bat not found at: $VcVarsAll"
}

$layoutFrontendDir = Join-Path $repoRoot 'src\app\seamlylayout\qt_frontend'
$smsiScript = Join-Path $PSScriptRoot 'smsi.ps1'
$smsiArgs = @('-Arch', 'x64', '-Version', $Version,
              '-Seamly2DBin', 'src\app\seamly2d\bin',
              '-SeamlyMeBin', 'src\app\seamlyme\bin',
              '-WinDeployQt', $WinDeployQt)
if ($SkipValidation) { $smsiArgs += '-SkipValidation' }
$smsiArgsQuoted = ($smsiArgs | ForEach-Object { "`"$_`"" }) -join ' '

$TempBat = [System.IO.Path]::GetTempFileName() + '.bat'
$BatchContent = @"
@echo off
call "$VcVarsAll" >nul 2>&1
if errorlevel 1 (
    echo Failed to initialize VS 18 x64 environment
    exit /b 1
)

set "QMAKE=$QtBin\qmake.exe"
set "PATH=$QtBin;%PATH%"

cd /d "$repoRoot"

echo.
echo === qmake / nmake: seamly2d + seamlyme ===
qmake Seamly.pro -config release CONFIG+=noTests
if errorlevel 1 exit /b 1
nmake
if errorlevel 1 exit /b 1

echo.
echo === cmake: SeamlyLayout ===
cd /d "$layoutFrontendDir"
cmake --preset release -DCMAKE_PREFIX_PATH="$QtPath"
if errorlevel 1 exit /b 1
cmake --build --preset release
if errorlevel 1 exit /b 1

echo.
echo === smsi.ps1: stage + wix build ===
cd /d "$repoRoot"
powershell -NoProfile -ExecutionPolicy Bypass -File "$smsiScript" $smsiArgsQuoted
if errorlevel 1 exit /b 1
"@
Set-Content -Path $TempBat -Value $BatchContent -Encoding ASCII

try {
    Invoke-NativeCommand { & cmd.exe /c $TempBat }
    if ($LASTEXITCODE -ne 0) {
        throw "local MSI build failed (exit code $LASTEXITCODE) - see output above."
    }
} finally {
    Remove-Item $TempBat -Force -ErrorAction SilentlyContinue
}

Write-Host ''
Write-Host "MSI OK: packaging\windows\seamly-msi\x64\seamly-x64.msi"
if ($versionFilesWereClean) {
    Write-Host "reverting the version stamp (projectversion.cpp/.h, both Info.plist) - scripts\version.sh regenerates it on demand, nothing to keep..."
    & git -C $repoRoot checkout -- $versionStampedFiles
} else {
    Write-Host "projectversion.cpp/.h and/or Info.plist carried uncommitted changes before this build - not auto-reverting them; check 'git status' for what to keep."
}
