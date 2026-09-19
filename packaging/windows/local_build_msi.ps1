#******************************************************************************
# **  @file   local_build_msi.ps1
# **  @author slspencer
# **  @date   August 28, 2026
# **
# **  @brief
# **  Local, developer-machine build of the Windows x64 MSI: builds seamly2d,
# **  seamlyme and SeamlyLayout release binaries, runs the Qt unit tests, then
# **  runs smsi.ps1 against the binaries. packaging\windows\README.md records the project's
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
    Uses jom instead of nmake for parallel compilation, with 4 CPUs by default.
    Requires jom to be installed and available in PATH.
    jom is typically found at Qt\Tools\jom within the Qt installation directory.
    Currently this script is set to use 4 CPUs for jom.

.DESCRIPTION
     Stamps the version, builds seamly2d, seamlyme, and SeamlyLayout, runs the
     Qt tests unless -SkipTests is specified, and packages the three apps into
     an MSI with packaging\windows\smsi.ps1. Test binaries remain in
     src\test\*\bin and are not staged.

    The version stamp touches git-tracked files
    (src\libs\vmisc\projectversion.{h,cpp}, packaging\macos\seamly2d\Info.plist,
    packaging\macos\seamlyme\Info.plist). A successful build reverts them via
    `git checkout`, unless those files already carried uncommitted changes
    before the run, in which case they are left alone so unrelated work is
    not discarded.

    Requires an elevated-free MSVC + Qt + Rust dev machine: Visual Studio 18
    Community (VC\Auxiliary\Build\vcvars64.bat), a Qt 6.11.1+ msvc2022_64 kit
    under C:\Qt with qtmultimedia, qtwebengine, qtwebchannel, qtpositioning and
    qtserialport installed, and a stable Rust toolchain on PATH. WiX v6 is installed
    automatically if missing.

.PARAMETER Version
    Project version as YY.M.D.MMMM, stamped into seamly2d/seamlyme via
    scripts\version.sh and used as the MSI's DisplayVersion/ProductVersion.
    Default: computed from the current local time, the same formula ci.yml's
    version job uses.

.PARAMETER SkipValidation
    Passed through to smsi.ps1 - skip the `wix msi validate` ICE pass.

.PARAMETER SkipTests
    Build with CONFIG+=noTests and do not run `nmake check`. Use it only when
    you need the MSI itself and the code under test has not changed - the Qt
    suites have no other local runner, so skipping defers them to CI.

.EXAMPLE
    .\packaging\windows\local_build_msi.ps1
    Full local x64 MSI build with an auto-computed version, unit tests included.

.EXAMPLE
    .\packaging\windows\local_build_msi.ps1 -SkipTests
    Same build without the unit tests, for a packaging-only change.
#>

param(
    [string]$Version,
    [switch]$SkipValidation,
    [switch]$SkipTests
)

Write-Host " ===== running local_build_msi.ps1 ====="

$ErrorActionPreference = 'Stop'
# The script lives in <repo-root>\packaging\windows\, so the repo root
# is two directories up.
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

#------------------------------------------------------------------------------
# @brief Run a native program and judge success by its exit code.
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

# scripts\version.sh writes the version YY.MM.DD.mmmm into these git-tracked files, the
# same way every ci.yml build job does. The stamp is reverted (via
# `git checkout`) once the build succeeds, unless one of these files already
# carried uncommitted changes before this run (then it is left alone, to not
# discard unrelated work).
$versionStampedFiles = @(
    'src\libs\vmisc\projectversion.cpp',
    'src\libs\vmisc\projectversion.h',
    'packaging\macos\seamly2d\Info.plist',
    'packaging\macos\seamlyme\Info.plist'
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
# Newest msvc2022_64 kit under C:\Qt at or above 6.11.1, so 
# seamly2d/seamlyme/SeamlyLayout all deploy # against the one Qt runtime 
# the MSI ships.
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
$MissingQtModules = @('Qt6WebEngineQuick', 'Qt6WebChannel', 'Qt6Positioning', 'Qt6SerialPort') |
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

# --- Build seamly2d, seamlyme and seamlyLayout under vcvars64 -------------------
# One cmd.exe batch: vcvars64 sets VCToolsRedistDir (which smsi.ps1 needs) in 
# this process's environment, so smsi.ps1 must run as a child of the SAME batch, 
# not a separate PowerShell.
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

# `qmake -r` regenerates every subdirectory Makefile, not just the top one.
# This matters only locally: leftover generated Makefiles can keep an old
# src\Makefile without the `test` subdirectory, so unit tests would never be
# built. CI starts from a fresh checkout, so it avoids this issue.
#
# Without CONFIG+=noTests, src.pro adds the `test` subdirectory; `CONFIG +=
# testcase` in each test .pro provides the `check` target nmake uses below.
#
# QT_QPA_PLATFORM=offscreen matches the CI Windows test job, so local and CI
# runs exercise the same widget-test backend without stealing focus.
#
# NoDefaultCurrentDirectoryInExePath is cleared so qmake's generated `check`
# target can resolve test executables from the build directory correctly in this
# cmd.exe process.
#
# CONFIG+=deferDeploy skips seamly2d.pro/seamlyme.pro's own windeployqt
# post-link step (see common.pri's deployQtRuntime()), so nmake below does not
# deploy seamly2d.exe/seamlyme.exe before SeamlyLayout's cmake build even
# starts. smsi.ps1 deploys all three exes together, once, after every exe below
# exists.
if ($SkipTests) {
    $QmakeConfig = '-config release CONFIG+=noTests CONFIG+=deferDeploy'
    $TestSection = @'

echo.
echo === unit tests: SKIPPED (-SkipTests) ===
'@
} else {
    $QmakeConfig = '-config release CONFIG+=deferDeploy'
    $TestSection = @'

echo.
echo === run nmake check: Seamly2D, Collection, Parser, Translations unit tests ===
set "QT_QPA_PLATFORM=offscreen"
set "NoDefaultCurrentDirectoryInExePath="
nmake check
if errorlevel 1 exit /b 1
set "QT_QPA_PLATFORM="
'@
}

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
echo === run qmake : seamly2d + seamlyme$(if (-not $SkipTests) { ' + unit tests' }) 
qmake Seamly.pro -r $QmakeConfig
if errorlevel 1 exit /b 1
echo === run nmake : build seamly2d.exe & seamlyme.exe ===
nmake
if errorlevel 1 exit /b 1
$TestSection

echo.
echo === run cmake : set SeamlyLayout release type & cmake path ===
cd /d "$layoutFrontendDir"
cmake --preset release -DCMAKE_PREFIX_PATH="$QtPath"
if errorlevel 1 exit /b 1
echo === run cmake : build seamlyLayout.exe ===
cmake --build --preset release
if errorlevel 1 exit /b 1

echo.
echo === smsi.ps1: stage + wix --> build MSI ===
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
if ($SkipTests) {
    Write-Host "unit tests: SKIPPED (-SkipTests) - Seamly2DTest, CollectionTest, ParserTest and TranslationsTest are deferred to CI."
} else {
    Write-Host "unit tests: PASSED (nmake check)"
}
Write-Host "MSI OK: packaging\windows\seamly-msi\x64\seamly-x64.msi"
if ($versionFilesWereClean) {
    Write-Host "reverting the version stamp (projectversion.cpp/.h, both Info.plist) - scripts\version.sh regenerates it on demand, nothing to keep..."
    & git -C $repoRoot checkout -- $versionStampedFiles
} else {
    Write-Host "projectversion.cpp/.h and/or Info.plist carried uncommitted changes before this build - not auto-reverting them; check 'git status' for what to keep."
}
