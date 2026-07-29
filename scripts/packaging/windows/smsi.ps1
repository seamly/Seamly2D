#******************************************************************************
# **  @file   smsi.ps1
# **  @author slspencer
# **  @date   July 22, 2026
# **
# **  @brief
# **  "seamly msi" — stage the built Seamly app family (seamly2d, seamlyme,
# **  SeamlyLayout) and build the Windows MSI installer from
# **  scripts\packaging\windows\seamly-family.wxs with the WiX toolset
# **  (Task 13).
# **  Used both locally (against the release build trees) and by the
# **  .github\workflows\windows-msi.yml CI workflow (against the in-source
# **  CI build output), following the scripts\sd.ps1 precedent.
# **
# **  @copyright
# **  This source code is part of the Seamly2D project, a pattern making
# **  program, whose allow create and modeling patterns of clothing.
# **  Copyright (C) 2026 Seamly2D Project
# **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
# **
# **  Seamly2D is free software: you can redistribute it and/or modify
# **  it under the terms of the GNU General Public License as published by
# **  the Free Software Foundation, either version 3 of the License, or
# **  (at your option) any later version.
# **
# **  Seamly2D is distributed in the hope that it will be useful,
# **  but WITHOUT ANY WARRANTY; without even the implied warranty of
# **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# **  GNU General Public License for more details.
# **
# **  You should have received a copy of the GNU General Public License
# **  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
# **
#******************************************************************************

<#
.SYNOPSIS
    Build the Windows MSI installer for the Seamly app family (Task 13).

.DESCRIPTION
    Stages the already-built apps into <repo>\scripts\seamly-build-msi\<arch>\
    and runs `wix build` on scripts\packaging\windows\seamly-family.wxs to
    produce scripts\seamly-build-msi\<arch>\Seamly2D-<arch>.msi. Only the .msi
    is written — the .wixpdb symbol database is suppressed with `-pdbtype none`
    (it is used only for wix patch/melt diffing, not by the installer).

    Staging layout (mirrors what the MSI installs):
      parent\  the ONE Qt runtime every app in the package shares: seamly2d +
               seamlyme windeployqt trees merged with SeamlyLayout's
               windeployqt6 output (Qt DLLs, plugins, QML modules,
               QtWebEngineProcess.exe, xerces-c, ...) plus SeamlyLayout's
               packaged settings\ and licenses\ and the MSVC CRT DLLs,
               minus the exes
      exes\    seamly2d.exe, seamlyme.exe, SeamlyLayout.exe (authored
               explicitly in the .wxs so shortcuts/associations can reference
               them; kept out of the wildcard-harvested tree above)

    Task 30 merged what used to be a separate layout\ staging tree (installed
    into a ...\Seamly2D\SeamlyLayout\ subdirectory with its own private Qt
    copy) into parent\. That split existed only because SeamlyLayout was built
    against a different Qt release than the parent apps and Qt's DLL file names
    are identical across releases; all three now build against Qt 6.11.1, so
    one runtime serves them all and the MSI is correspondingly smaller.

    PREREQUISITES (the script fails early naming whatever is missing):
      * release builds of seamly2d/seamlyme with windeployqt output in their
        bin directories (local: qmake release shadow-build in build\;
        CI: in-source src\app\<app>\bin)
      * a release build of SeamlyLayout (src\app\seamlylayout\qt_frontend\
        build\Release), unless -NoSeamlyLayout
      * the WiX .NET tool:      dotnet tool install --global wix
        and its UI extension:   wix extension add --global WixToolset.UI.wixext
      * the MSVC redistributable runtime (VCToolsRedistDir from a VS install)

    The MSI ProductVersion cannot carry the project's YYYY.M.D.HHMM scheme
    (MSI limits the major field to 255), so the script derives a monotonic
    numeric version:  (YYYY-2000).M.((D-1)*1440 + HH*60 + MM)  — strictly
    increasing across builds, so MajorUpgrade always upgrades in place. The
    full project version is embedded as DisplayVersion.

.PARAMETER Arch
    Target architecture of the MSI: x64 (default) or arm64.

.PARAMETER Version
    Project version as YYYY.M.D.HHMM (the ci.yml scheme). Defaults to the
    current date/time.

.PARAMETER Seamly2DBin
    Directory holding seamly2d.exe plus its windeployqt output.
    Default: <repo>\build\src\app\seamly2d\bin (the local release tree).

.PARAMETER SeamlyMeBin
    Directory holding seamlyme.exe plus its windeployqt output.
    Default: <repo>\build\src\app\seamlyme\bin.

.PARAMETER SeamlyLayoutBuildDir
    Directory holding the release SeamlyLayout.exe.
    Default: <repo>\src\app\seamlylayout\qt_frontend\build\Release.

.PARAMETER NoSeamlyLayout
    Build a two-app MSI without SeamlyLayout. Used for the arm64 package until
    SeamlyLayout has an arm64 build (see .github\README-BUILDS.md).

.PARAMETER WinDeployQt6
    Full path of windeployqt6.exe from SeamlyLayout's Qt kit. Default: the kit
    SeamlyLayout was actually built against, read from CMAKE_PREFIX_PATH in its
    build directory's CMakeCache.txt, falling back to the newest
    C:\Qt\<version>\msvc2022_64 kit. (CI passes the installed Qt explicitly.)
    Deliberately NOT pinned to a hard-coded Qt version — Task 31: the old
    ^6\.10\.\d+$ pin made the documented default invocation fail outright once
    the 6.10 kit was uninstalled.

.PARAMETER SkipValidation
    Skip the `wix msi validate` (ICE) pass after the build.

.EXAMPLE
    .\scripts\packaging\windows\smsi.ps1
    Stage from the local release trees and build the x64 MSI.

.EXAMPLE
    .\scripts\packaging\windows\smsi.ps1 -Arch arm64 -NoSeamlyLayout
    Build the arm64 MSI (seamly2d + seamlyme only) from arm64 build trees.

.NOTES
    "smsi" = seamly msi, following sd.ps1 ("seamly2d debug") / st.ps1
    ("seamly2d tests"). Output and staging live in scripts\seamly-build-msi\,
    which the *-build-* .gitignore pattern keeps out of the repository.
#>

param(
    # Target MSI architecture; must match the architecture of the staged binaries.
    [ValidateSet('x64', 'arm64')]
    [string]$Arch = 'x64',

    # Project version YYYY.M.D.HHMM; the MSI ProductVersion is derived from it.
    [string]$Version = (Get-Date -Format 'yyyy.M.d.HHmm'),

    # Built app trees (windeployqt output included).
    [string]$Seamly2DBin,
    [string]$SeamlyMeBin,
    [string]$SeamlyLayoutBuildDir,

    # Omit SeamlyLayout (arm64 packages, until an arm64 SeamlyLayout build exists).
    [switch]$NoSeamlyLayout,

    # windeployqt6.exe of SeamlyLayout's Qt kit (auto-detected under C:\Qt if omitted).
    [string]$WinDeployQt6,

    # Skip the ICE validation pass.
    [switch]$SkipValidation
)

# Stop on any PowerShell-level error; native tool failures are checked via
# exit codes after each call.
$ErrorActionPreference = 'Stop'

# The script lives in <repo-root>\scripts\packaging\windows\, so the repo root
# is three directories up.
$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))

#------------------------------------------------------------------------------
# @brief  Run a native tool and fail the script if its exit code is nonzero.
#
# @param  Description  short label used in the error message
# @param  Exe          tool to run
# @param  Arguments    argument array passed through verbatim
#------------------------------------------------------------------------------
function Invoke-Tool {
    param(
        [string]$Description,
        [string]$Exe,
        [string[]]$Arguments
    )
    # Native tools (windeployqt6, wix) legitimately write warnings to stderr —
    # e.g. windeployqt6 warning about the optional Qt6SerialPort dependency of
    # the NMEA positioning plugin. Under $ErrorActionPreference='Stop',
    # Windows PowerShell 5.1 turns captured stderr lines into terminating
    # errors even when the tool exits 0, so the preference is relaxed for the
    # call (function-local, dynamic scope) and every output line is
    # stringified; success is judged by the exit code alone.
    $ErrorActionPreference = 'Continue'
    & $Exe @Arguments 2>&1 | ForEach-Object { "$_" }
    $ErrorActionPreference = 'Stop'
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed (exit code $LASTEXITCODE) - see output above."
    }
}

#------------------------------------------------------------------------------
# @brief  Derive the numeric MSI ProductVersion from the project version.
#
# MSI ProductVersion fields are limited to major<=255, minor<=255,
# build<=65535, and the 4th field is ignored for upgrade comparisons - the
# project's YYYY.M.D.HHMM scheme therefore cannot be used directly (2026>255).
# Mapping: (YYYY-2000).(M).((D-1)*1440 + HH*60 + MM). The third field encodes
# day+time as minutes-of-month (max 44639 < 65535), so the result increases
# strictly with every build and MajorUpgrade always sees newer builds as newer.
#
# @param  ProjectVersion  version string YYYY.M.D.HHMM
# @return the derived x.y.z MSI version string
#------------------------------------------------------------------------------
function ConvertTo-MsiVersion {
    param([string]$ProjectVersion)

    $parts = $ProjectVersion.Split('.')
    if ($parts.Count -ne 4 -or ($parts | Where-Object { $_ -notmatch '^\d+$' })) {
        throw "Version '$ProjectVersion' is not in the expected YYYY.M.D.HHMM form."
    }
    $year  = [int]$parts[0]
    $month = [int]$parts[1]
    $day   = [int]$parts[2]
    $hhmm  = $parts[3]
    if ($year -lt 2000 -or $year -gt 2255 -or $month -lt 1 -or $month -gt 12 -or $day -lt 1 -or $day -gt 31) {
        throw "Version '$ProjectVersion' has out-of-range date fields."
    }
    # HHMM has no fixed width (ci.yml uses %-H%M): the last two digits are the
    # minutes, whatever precedes them (possibly nothing) is the hour.
    if ($hhmm.Length -lt 2) { $hhmm = $hhmm.PadLeft(2, '0') }
    $minute = [int]$hhmm.Substring($hhmm.Length - 2)
    $hourText = $hhmm.Substring(0, $hhmm.Length - 2)
    if ($hourText -eq '') { $hour = 0 } else { $hour = [int]$hourText }
    if ($hour -gt 23 -or $minute -gt 59) {
        throw "Version '$ProjectVersion' has an out-of-range HHMM field."
    }

    $minutesOfMonth = (($day - 1) * 1440) + ($hour * 60) + $minute
    return "$($year - 2000).$month.$minutesOfMonth"
}

#------------------------------------------------------------------------------
# @brief  Locate the windeployqt6.exe belonging to SeamlyLayout's Qt kit.
#
# Task 31: this used to be hard-pinned to '^6\.10\.\d+$', so the documented
# default invocation threw as soon as the 6.10 kit was uninstalled, and it could
# silently deploy a runtime from a different Qt release than the exe was linked
# against. It now follows the build instead of a fixed version:
#
#   1. Read CMAKE_PREFIX_PATH out of SeamlyLayout's CMakeCache.txt — that is
#      literally the kit the staged SeamlyLayout.exe was compiled and linked
#      against, so its deploy tool always matches the binary.
#   2. Fall back to the newest C:\Qt\<version>\msvc2022_64 kit (any 6.x), so a
#      clean tree with no build cache still resolves.
#
# Either way the unsuffixed windeployqt.exe is accepted as an alternate name.
#
# @param  BuildDir  SeamlyLayout's release build directory (holds CMakeCache.txt)
# @return Full path of the deploy tool.
#------------------------------------------------------------------------------
function Find-WinDeployQt6 {
    param([string]$BuildDir)

    #--- 1. the kit recorded in SeamlyLayout's own CMake cache ----------------
    $cache = Join-Path $BuildDir 'CMakeCache.txt'
    if (Test-Path $cache) {
        # CMAKE_PREFIX_PATH:PATH=C:/Qt/6.11.1/msvc2022_64 (forward slashes, and
        # possibly a ';'-separated list — the Qt kit is the entry that has the
        # deploy tool under bin\).
        $entry = Select-String -LiteralPath $cache -Pattern '^CMAKE_PREFIX_PATH:[A-Z]+=(.+)$' |
            Select-Object -First 1
        if ($entry) {
            foreach ($prefix in $entry.Matches[0].Groups[1].Value.Split(';')) {
                foreach ($name in @('windeployqt6.exe', 'windeployqt.exe')) {
                    $tool = Join-Path $prefix.Trim() "bin\$name"
                    if ($prefix.Trim() -and (Test-Path $tool)) { return (Resolve-Path $tool).Path }
                }
            }
        }
    }

    #--- 2. newest installed msvc2022_64 kit, whatever its version -----------
    $qtRoot = 'C:\Qt'
    if (Test-Path $qtRoot) {
        $kits = Get-ChildItem $qtRoot -Directory -ErrorAction SilentlyContinue |
            ForEach-Object {
                $parsed = $null
                if ([version]::TryParse($_.Name, [ref]$parsed)) {
                    [pscustomobject]@{ Version = $parsed; Dir = $_.FullName }
                }
            } |
            Sort-Object Version -Descending
        foreach ($kit in $kits) {
            foreach ($name in @('windeployqt6.exe', 'windeployqt.exe')) {
                $tool = Join-Path $kit.Dir "msvc2022_64\bin\$name"
                if (Test-Path $tool) { return $tool }
            }
        }
    }
    throw "windeployqt6 not found - no CMAKE_PREFIX_PATH kit in '$cache' and no msvc2022_64 kit under '$qtRoot'. Install Qt (msvc2022_64) or pass -WinDeployQt6."
}

#------------------------------------------------------------------------------
# @brief  Locate the MSVC CRT redistributable DLL directory for an architecture.
#
# Preferred source is the VCToolsRedistDir environment variable (set by
# vcvars/msvc-dev-cmd); otherwise the newest version directory under any
# Visual Studio install's VC\Redist\MSVC is used. The returned directory is
# the Microsoft.VC*.CRT folder holding msvcp140.dll, vcruntime140.dll, etc.,
# which the script copies app-locally (decision recorded in
# scripts\packaging\windows\README.md: no merge modules, no vc_redist.exe
# chaining).
#
# @param  Architecture  'x64' or 'arm64'
# @return Full path of the CRT DLL directory.
#------------------------------------------------------------------------------
function Find-CrtDirectory {
    param([string]$Architecture)

    $candidates = @()
    if ($env:VCToolsRedistDir) {
        $candidates += $env:VCToolsRedistDir.TrimEnd('\')
    }
    # Fall back to scanning installed Visual Studios (any edition/version).
    foreach ($vsRoot in @('C:\Program Files\Microsoft Visual Studio', 'C:\Program Files (x86)\Microsoft Visual Studio')) {
        if (Test-Path $vsRoot) {
            $versionDirs = Get-ChildItem "$vsRoot\*\*\VC\Redist\MSVC" -Directory -ErrorAction SilentlyContinue |
                Get-ChildItem -Directory -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -match '^\d+\.\d+' } |
                Sort-Object { [version]($_.Name -replace '[^\d.].*$', '') } -Descending
            $candidates += ($versionDirs | ForEach-Object { $_.FullName })
        }
    }

    foreach ($base in $candidates) {
        $archDir = Join-Path $base $Architecture
        if (Test-Path $archDir) {
            $crt = Get-ChildItem $archDir -Directory -Filter 'Microsoft.VC*.CRT' -ErrorAction SilentlyContinue |
                Select-Object -First 1
            if ($crt) { return $crt.FullName }
        }
    }
    throw "MSVC CRT redistributable for '$Architecture' not found - run from a VS developer environment (VCToolsRedistDir) or install the VS C++ workload."
}

# --- Resolve inputs and tools (fail early with clear messages) ----------------
if (-not $Seamly2DBin)          { $Seamly2DBin          = Join-Path $repoRoot 'build\src\app\seamly2d\bin' }
if (-not $SeamlyMeBin)          { $SeamlyMeBin          = Join-Path $repoRoot 'build\src\app\seamlyme\bin' }
if (-not $SeamlyLayoutBuildDir) { $SeamlyLayoutBuildDir = Join-Path $repoRoot 'src\app\seamlylayout\qt_frontend\build\Release' }
$includeLayout = -not $NoSeamlyLayout

foreach ($required in @(
        @{ Path = (Join-Path $Seamly2DBin 'seamly2d.exe'); What = 'seamly2d.exe (build the release tree first)' },
        @{ Path = (Join-Path $Seamly2DBin 'platforms');    What = "seamly2d's windeployqt output (platforms\ plugin dir)" },
        @{ Path = (Join-Path $SeamlyMeBin 'seamlyme.exe'); What = 'seamlyme.exe (build the release tree first)' })) {
    if (-not (Test-Path $required.Path)) {
        throw "Missing $($required.What): '$($required.Path)'."
    }
}
if ($includeLayout -and -not (Test-Path (Join-Path $SeamlyLayoutBuildDir 'SeamlyLayout.exe'))) {
    throw "Missing SeamlyLayout.exe in '$SeamlyLayoutBuildDir' - build it (src\app\seamlylayout, release preset) or pass -NoSeamlyLayout."
}

# WiX .NET tool + UI extension (the .wxs uses WixUI_InstallDir). Pinned to v6:
# WiX v7 refuses to run until its Open Source Maintenance Fee EULA is accepted
# (error WIX7015); v6 is the newest line without that gate. The UI extension
# version must match the installed core tool version.
if (-not (Get-Command wix -ErrorAction SilentlyContinue)) {
    throw "The WiX toolset is not installed - run: dotnet tool install --global wix --version '6.*'"
}
$uiExtensionInstalled = (& wix extension list --global 2>$null) -match 'WixToolset\.UI\.wixext'
if (-not $uiExtensionInstalled) {
    throw "The WiX UI extension is missing - run: wix extension add --global WixToolset.UI.wixext/<wix version, e.g. 6.0.2>"
}

if ($includeLayout -and -not $WinDeployQt6) { $WinDeployQt6 = Find-WinDeployQt6 -BuildDir $SeamlyLayoutBuildDir }
$crtDir = Find-CrtDirectory -Architecture $Arch
$msiVersion = ConvertTo-MsiVersion -ProjectVersion $Version

Write-Host "arch        : $Arch"
Write-Host "version     : $Version  (MSI ProductVersion $msiVersion)"
Write-Host "seamly2d    : $Seamly2DBin"
Write-Host "seamlyme    : $SeamlyMeBin"
if ($includeLayout) {
    Write-Host "seamlylayout: $SeamlyLayoutBuildDir"
    Write-Host "windeployqt6: $WinDeployQt6"
} else {
    Write-Host "seamlylayout: EXCLUDED (-NoSeamlyLayout)"
}
Write-Host "msvc crt    : $crtDir"

# --- Stage ---------------------------------------------------------------------
# Fresh staging tree per run:
# <repo>\scripts\seamly-build-msi\<arch>\{parent,exes}
# (the *-build-* .gitignore pattern keeps all of it untracked). The output lives
# at the scripts\ root — a sibling of scripts\seamly2d-build-debug\ from sd.ps1 —
# not beside this script, so it is anchored to $repoRoot.
#
# 'parent' is now the ONE shared runtime tree for every app in the package
# (Task 30); the separate 'layout' tree it used to sit beside is gone.
$stageRoot = Join-Path $repoRoot "scripts\seamly-build-msi\$Arch"
if (Test-Path $stageRoot) {
    Remove-Item $stageRoot -Recurse -Force
}
$parentDir = Join-Path $stageRoot 'parent'
$exesDir   = Join-Path $stageRoot 'exes'
New-Item -ItemType Directory -Force -Path $parentDir, $exesDir | Out-Null

# seamly2d + seamlyme runtimes merged into one tree (they share the same Qt
# release, so the overlapping DLLs are identical), exactly like the NSIS
# packaging step in ci.yml merges the two bin trees.
Write-Host "staging seamly2d + seamlyme runtime..."
Copy-Item -Path (Join-Path $Seamly2DBin '*') -Destination $parentDir -Recurse
Copy-Item -Path (Join-Path $SeamlyMeBin '*') -Destination $parentDir -Recurse -Force

# The executables are authored explicitly in the .wxs (shortcuts/associations
# reference them), so move them out of the wildcard-harvested tree.
Move-Item -Path (Join-Path $parentDir 'seamly2d.exe') -Destination $exesDir
Move-Item -Path (Join-Path $parentDir 'seamlyme.exe') -Destination $exesDir

if ($includeLayout) {
    Write-Host "staging SeamlyLayout runtime (windeployqt6) into the shared tree..."

    # Task 30: deploy SeamlyLayout's Qt runtime into the SAME tree as the parent
    # apps'. All three are built against Qt 6.11.1, so wherever the two
    # windeployqt runs produce the same DLL it is the same file — what
    # SeamlyLayout adds on top is the QML module tree, Qt Quick/WebEngine DLLs
    # and QtWebEngineProcess.exe. Deploying against a staged copy of the exe
    # keeps the build tree pristine; --qmldir points windeployqt6 at the QML
    # sources so it can resolve the app's QML module imports.
    Copy-Item -Path (Join-Path $SeamlyLayoutBuildDir 'SeamlyLayout.exe') -Destination $parentDir
    $qmlDir = Join-Path $repoRoot 'src\app\seamlylayout\qt_frontend\qml'
    Invoke-Tool -Description 'windeployqt6' -Exe $WinDeployQt6 -Arguments @(
        '--qmldir', $qmlDir, '--release', (Join-Path $parentDir 'SeamlyLayout.exe'))

    # Packaged default settings (read-only legacy-migration source / first-run
    # seed), read by SeamlyLayout from <exeDir>\settings\. preferences.json is
    # deliberately excluded: it contains per-user paths (same exclusion as the
    # Inno Setup script SeamlyLayout.iss). The parent apps ship no settings\
    # directory, so nothing collides in the now-shared tree.
    $settingsSrc = Join-Path $repoRoot 'src\app\seamlylayout\qt_frontend\settings'
    $settingsDst = Join-Path $parentDir 'settings'
    New-Item -ItemType Directory -Force -Path $settingsDst | Out-Null
    foreach ($file in @('default_settings.json', 'B0.json', 'roll_36in.json', 'roll_48in.json')) {
        Copy-Item -Path (Join-Path $settingsSrc $file) -Destination $settingsDst
    }

    # LGPL compliance notices for the bundled Qt runtime.
    $licensesSrc = Join-Path $repoRoot 'src\app\seamlylayout\packaging\licenses'
    if (Test-Path $licensesSrc) {
        $licensesDst = Join-Path $parentDir 'licenses'
        New-Item -ItemType Directory -Force -Path $licensesDst | Out-Null
        Copy-Item -Path (Join-Path $licensesSrc '*.txt') -Destination $licensesDst
    }

    Move-Item -Path (Join-Path $parentDir 'SeamlyLayout.exe') -Destination $exesDir
}

# MSVC CRT app-local deployment: the directory holding the exes gets the runtime
# DLLs, since PATH-independent DLL resolution is per-directory. With one shared
# install directory that is a single copy for all three apps.
Write-Host "staging MSVC CRT runtime..."
Copy-Item -Path (Join-Path $crtDir '*.dll') -Destination $parentDir -Force

# --- Build the MSI -------------------------------------------------------------
$wxs = Join-Path $PSScriptRoot 'seamly-family.wxs'
$msi = Join-Path $stageRoot "Seamly2D-$Arch.msi"

$wixArguments = @(
    'build', $wxs,
    '-arch', $Arch,
    # Suppress the .wixpdb symbol database: it is only used for wix patch/melt
    # diffing and post-build inspection, not by the shipped installer, so we keep
    # the build output to just the .msi. WiX v6 equivalent of light.exe -spdb.
    '-pdbtype', 'none',
    '-ext', 'WixToolset.UI.wixext',
    '-d', "ProductVersion=$msiVersion",
    '-d', "DisplayVersion=$Version",
    '-d', "RepoRoot=$repoRoot",
    '-d', "ParentStagingDir=$parentDir",
    '-d', "ExeStagingDir=$exesDir",
    '-o', $msi
)
if ($includeLayout) {
    # No LayoutStagingDir any more — SeamlyLayout's runtime is merged into
    # ParentStagingDir (Task 30), so the .wxs harvests one tree.
    $wixArguments += @('-d', 'IncludeSeamlyLayout=1')
}

Write-Host "running wix build..."
Invoke-Tool -Description 'wix build' -Exe 'wix' -Arguments $wixArguments

if (-not (Test-Path $msi)) {
    throw "wix build reported success but '$msi' is missing."
}

# --- Validate (ICE checks) -----------------------------------------------------
# Two ICEs are suppressed, both raised by the Task 51 optional desktop-shortcut
# components and both false positives for this package:
#
#   ICE43  "non-advertised shortcut ... KeyPath should fall under HKCU"
#   ICE57  "per-user and per-machine data with a per-machine KeyPath"
#
# Each assumes DesktopFolder is inside the installing user's profile, which is
# only true of a per-user install. This package is Scope="perMachine" with
# ALLUSERS=1, so DesktopFolder always resolves to the common (All Users)
# desktop and the HKLM key path is the correct one. Doing what the ICEs ask
# would actively break the package: the server-side sequence of a per-machine
# install runs elevated as LocalSystem, so an HKCU key path would be written
# into the SYSTEM account's hive, where component detection can never find it
# again - every launch would then trigger installer self-repair. The shortcuts
# cannot be advertised instead (an advertised shortcut has to live in the
# component that owns its target file, which would stop them being optional).
#
# ICE61 stays visible and is expected: it is a known consequence of
# MajorUpgrade/@AllowSameVersionUpgrades.
if (-not $SkipValidation) {
    Write-Host "running wix msi validate (ICE checks)..."
    Invoke-Tool -Description 'wix msi validate' -Exe 'wix' -Arguments @(
        'msi', 'validate', $msi, '-sice', 'ICE43', '-sice', 'ICE57')
}

# --- Check the install-time authoring (Task 51) --------------------------------
# The ICE checks say the package is well formed; this says it still contains the
# shortcuts, associations, registry rows, elevation, upgrade detection and
# install-time dialogs the project expects. Runs on every build, including CI,
# because the failure mode it guards against is silent - a WixUI or WiX change
# that drops a row produces an MSI that installs perfectly and does the wrong
# thing.
Write-Host "checking install-time authoring..."
# A hashtable, not an array: @array splats positionally, @hashtable by name.
$checkArguments = @{ Msi = $msi; Arch = $Arch }
if ($includeLayout) { $checkArguments['ExpectSeamlyLayout'] = $true }
& (Join-Path $PSScriptRoot 'test_msi_authoring.ps1') @checkArguments
if ($LASTEXITCODE -ne 0) {
    throw "install-time authoring check failed (exit code $LASTEXITCODE) - see output above."
}

$msiSize = [math]::Round((Get-Item $msi).Length / 1MB, 1)
Write-Host ''
Write-Host "MSI OK: $msi ($msiSize MB)"
