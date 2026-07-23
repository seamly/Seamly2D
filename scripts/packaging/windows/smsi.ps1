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
    produce scripts\seamly-build-msi\<arch>\Seamly2D-<arch>.msi.

    Staging layout (mirrors what the MSI installs):
      parent\  seamly2d + seamlyme windeployqt trees merged (Qt runtime,
               plugins, xerces-c, ...) plus the MSVC CRT DLLs, minus the exes
      layout\  SeamlyLayout Qt runtime — windeployqt6 is run here against the
               staged exe — plus packaged default settings\ and licenses\ and
               the MSVC CRT DLLs, minus the exe
      exes\    seamly2d.exe, seamlyme.exe, SeamlyLayout.exe (authored
               explicitly in the .wxs so shortcuts/associations can reference
               them; kept out of the wildcard-harvested trees above)

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
    Full path of windeployqt6.exe from SeamlyLayout's Qt kit. Default: the
    newest C:\Qt\6.10.x\msvc2022_64\bin\windeployqt6.exe (CI passes the
    installed Qt explicitly).

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
# @brief  Locate windeployqt6.exe from the newest Qt 6.10.x msvc2022_64 kit.
#
# Mirrors sd.ps1's Find-QtQmake: scans C:\Qt for 6.10.<patch> version dirs,
# newest first, and returns the first kit that ships the deploy tool
# (falling back to the unsuffixed windeployqt.exe name).
#
# @return Full path of the deploy tool.
#------------------------------------------------------------------------------
function Find-WinDeployQt6 {
    $qtRoot = 'C:\Qt'
    if (Test-Path $qtRoot) {
        $kits = Get-ChildItem $qtRoot -Directory |
            Where-Object { $_.Name -match '^6\.10\.\d+$' } |
            Sort-Object { [version]$_.Name } -Descending
        foreach ($kit in $kits) {
            foreach ($name in @('windeployqt6.exe', 'windeployqt.exe')) {
                $tool = Join-Path $kit.FullName "msvc2022_64\bin\$name"
                if (Test-Path $tool) { return $tool }
            }
        }
    }
    throw "windeployqt6 not found under '$qtRoot\6.10.x\msvc2022_64\bin' - install Qt 6.10.x or pass -WinDeployQt6."
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

if ($includeLayout -and -not $WinDeployQt6) { $WinDeployQt6 = Find-WinDeployQt6 }
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
# <repo>\scripts\seamly-build-msi\<arch>\{parent,layout,exes}
# (the *-build-* .gitignore pattern keeps all of it untracked). The output lives
# at the scripts\ root — a sibling of scripts\seamly2d-build-debug\ from sd.ps1 —
# not beside this script, so it is anchored to $repoRoot.
$stageRoot = Join-Path $repoRoot "scripts\seamly-build-msi\$Arch"
if (Test-Path $stageRoot) {
    Remove-Item $stageRoot -Recurse -Force
}
$parentDir = Join-Path $stageRoot 'parent'
$layoutDir = Join-Path $stageRoot 'layout'
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
    Write-Host "staging SeamlyLayout runtime (windeployqt6)..."
    New-Item -ItemType Directory -Force -Path $layoutDir | Out-Null

    # Deploy the Qt 6.10 runtime against a staged copy of the exe so the build
    # tree stays pristine; --qmldir points windeployqt6 at the QML sources so
    # it can resolve the app's QML module imports.
    Copy-Item -Path (Join-Path $SeamlyLayoutBuildDir 'SeamlyLayout.exe') -Destination $layoutDir
    $qmlDir = Join-Path $repoRoot 'src\app\seamlylayout\qt_frontend\qml'
    Invoke-Tool -Description 'windeployqt6' -Exe $WinDeployQt6 -Arguments @(
        '--qmldir', $qmlDir, '--release', (Join-Path $layoutDir 'SeamlyLayout.exe'))

    # Packaged default settings (read-only legacy-migration source / first-run
    # seed). preferences.json is deliberately excluded: it contains per-user
    # paths (same exclusion as the Inno Setup script SeamlyLayout.iss).
    $settingsSrc = Join-Path $repoRoot 'src\app\seamlylayout\qt_frontend\settings'
    $settingsDst = Join-Path $layoutDir 'settings'
    New-Item -ItemType Directory -Force -Path $settingsDst | Out-Null
    foreach ($file in @('default_settings.json', 'B0.json', 'roll_36in.json', 'roll_48in.json')) {
        Copy-Item -Path (Join-Path $settingsSrc $file) -Destination $settingsDst
    }

    # LGPL compliance notices for the bundled Qt runtime.
    $licensesSrc = Join-Path $repoRoot 'src\app\seamlylayout\packaging\licenses'
    if (Test-Path $licensesSrc) {
        $licensesDst = Join-Path $layoutDir 'licenses'
        New-Item -ItemType Directory -Force -Path $licensesDst | Out-Null
        Copy-Item -Path (Join-Path $licensesSrc '*.txt') -Destination $licensesDst
    }

    Move-Item -Path (Join-Path $layoutDir 'SeamlyLayout.exe') -Destination $exesDir
}

# MSVC CRT app-local deployment: every directory containing an exe gets the
# runtime DLLs, since PATH-independent DLL resolution is per-directory.
Write-Host "staging MSVC CRT runtime..."
Copy-Item -Path (Join-Path $crtDir '*.dll') -Destination $parentDir -Force
if ($includeLayout) {
    Copy-Item -Path (Join-Path $crtDir '*.dll') -Destination $layoutDir -Force
}

# --- Build the MSI -------------------------------------------------------------
$wxs = Join-Path $PSScriptRoot 'seamly-family.wxs'
$msi = Join-Path $stageRoot "Seamly2D-$Arch.msi"

$wixArguments = @(
    'build', $wxs,
    '-arch', $Arch,
    '-ext', 'WixToolset.UI.wixext',
    '-d', "ProductVersion=$msiVersion",
    '-d', "DisplayVersion=$Version",
    '-d', "RepoRoot=$repoRoot",
    '-d', "ParentStagingDir=$parentDir",
    '-d', "ExeStagingDir=$exesDir",
    '-o', $msi
)
if ($includeLayout) {
    $wixArguments += @('-d', 'IncludeSeamlyLayout=1', '-d', "LayoutStagingDir=$layoutDir")
}

Write-Host "running wix build..."
Invoke-Tool -Description 'wix build' -Exe 'wix' -Arguments $wixArguments

if (-not (Test-Path $msi)) {
    throw "wix build reported success but '$msi' is missing."
}

# --- Validate (ICE checks) -----------------------------------------------------
if (-not $SkipValidation) {
    Write-Host "running wix msi validate (ICE checks)..."
    Invoke-Tool -Description 'wix msi validate' -Exe 'wix' -Arguments @('msi', 'validate', $msi)
}

$msiSize = [math]::Round((Get-Item $msi).Length / 1MB, 1)
Write-Host ''
Write-Host "MSI OK: $msi ($msiSize MB)"
