#******************************************************************************
# **  @file   smsi.ps1
# **  @author slspencer
# **  @date   July 22, 2026
# **
# **  @brief
# **  "seamly msi" — stage the built Seamly app suite (seamly2d, seamlyme,
# **  SeamlyLayout) and build the Windows MSI installer from
# **  packaging\windows\smsi.wxs with the WiX toolset.
# **  Driven by the ci.yml windows-msi job against the in-source CI build
# **  output. Every input is named on the command line; the script detects
# **  nothing from the machine it runs on.
# **
# ** @copyright
# **  This source code is part of the Seamly project, a suite of apparel CAD
# **  software.
# **  Copyright (C) 2026 Seamly2D Project
# **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
# **
# **  @license
# **  Seamly2D/SeamlyMe is free software: you can redistribute it and/or modify
# **  it under the terms of the GNU General Public License as published by
# **  the Free Software Foundation, either version 3 of the License, or
# **  (at your option) any later version.
# **
# **  Seamly2D/SeamlyMe is distributed in the hope that it will be useful,
# **  but WITHOUT ANY WARRANTY; without even the implied warranty of
# **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# **  GNU General Public License for more details.
# **
# **  You should have received a copy of the GNU General Public License
# **  along with Seamly2D/SeamlyMe.  If not, see <http://www.gnu.org/licenses/>.
# **
# ** SeamlyLayout is licensed under the MIT license.
#******************************************************************************

<#
.SYNOPSIS
    Build the Windows MSI installer for the Seamly app suite.

.DESCRIPTION
    Stages the already-built apps into
    <repo>\packaging\windows\seamly-msi\<arch>\ and runs `wix build` on
    packaging\windows\smsi.wxs to produce
    packaging\windows\seamly-msi\<arch>\seamly-<arch>.msi. Only the .msi
    is written — the .wixpdb symbol database is suppressed with `-pdbtype none`
    (it is used only for wix patch/melt diffing, not by the installer).

    Staging layout (mirrors what the MSI installs):
      parent\  the ONE Qt runtime every app in the package shares: seamly2d +
               seamlyme + seamlylayout's windeployqt output (Qt DLLs, plugins, QML modules,
               QtWebEngineProcess.exe, xerces-c, ...)
      exes\    seamly2d.exe, seamlyme.exe, seamlylayout.exe (authored
               explicitly in the .wxs so shortcuts/associations can reference
               them; kept out of the wildcard-harvested tree above)

    All three apps build against Qt 6.11.1, so one Qt runtime serves them all.

    PREREQUISITES (the script fails early naming whatever is missing):
      * release builds of seamly2d/seamlyme with windeployqt output in the
        bin directories named by -Seamly2DBin / -SeamlyMeBin
      * a release build of seamlylayout (src\app\seamlylayout\qt_frontend\
        build\Release)
      * the WiX .NET tool:      dotnet tool install --global wix
        and its UI extension:   wix extension add --global WixToolset.UI.wixext
      * the MSVC developer environment, which sets VCToolsRedistDir (ci.yml
        uses ilammy/msvc-dev-cmd)

    The MSI ProductVersion cannot carry the project's 4-part YY.M.D.MMMM
    scheme (MSI ignores the 4th field for upgrade comparisons), so the script
    derives a monotonic 3-part numeric version:  YY.M.((D-1)*1440 + MMMM)  —
    strictly increasing across builds, so MajorUpgrade always upgrades in
    place. The full project version is embedded as DisplayVersion.

.PARAMETER Arch
    Target architecture of the MSI: x64 (default) or arm64.

.PARAMETER Version
    Project version as YY.M.D.MMMM (the ci.yml scheme), where MMMM is the
    minute of the day. Required.

.PARAMETER Seamly2DBin
    Directory holding seamly2d.exe plus its windeployqt output. Required.

.PARAMETER SeamlyMeBin
    Directory holding seamlyme.exe plus its windeployqt output. Required.

.PARAMETER SeamlyLayoutBuildDir
    Directory holding the release seamlylayout.exe.
    Default: <repo>\src\app\seamlylayout\qt_frontend\build\Release — where
    ci.yml's `cmake --build --preset release` step writes it.

.PARAMETER WinDeployQt
    Full path of windeployqt.exe from the Qt kit SeamlyLayout was built
    against. Required. The caller names the kit, so no Qt version is hard-coded
    and none is detected. seamly2d, seamlyme and seamlylayout must all come
    from that one Qt 6.11.1 kit.

.PARAMETER SkipValidation
    Skip the `wix msi validate` (ICE) pass after the build.

.EXAMPLE
    .\packaging\windows\smsi.ps1 -Arch x64 -Version 26.8.15.570 `
        -Seamly2DBin src\app\seamly2d\bin -SeamlyMeBin src\app\seamlyme\bin `
        -WinDeployQt "$env:QT_ROOT_DIR\bin\windeployqt.exe"
    The invocation ci.yml's windows-msi job runs, for either architecture.

.NOTES
    "smsi" = seamly msi. This script only packages; it never builds. The
    scripts that produced its input trees locally (sb.ps1, sd.ps1) were
    deleted in August 2026, so either build the trees by hand or let ci.yml's
    windows-msi job do the whole job. Output and staging live in
    packaging\windows\seamly-msi\ (or whatever -OutputDirName names), which
    .gitignore lists by name.
#>

param(
    # Target MSI architecture. Must match the staged binaries.
    [ValidateSet('x64', 'arm64')]
    [string]$Arch = 'x64',

    # Project version YY.M.D.MMMM. Derives the MSI ProductVersion.
    [Parameter(Mandatory = $true)]
    [string]$Version,

    # Built seamly2d tree, windeployqt output included.
    [Parameter(Mandatory = $true)]
    [string]$Seamly2DBin,

    # Built seamlyme tree, windeployqt output included.
    [Parameter(Mandatory = $true)]
    [string]$SeamlyMeBin,

    # SeamlyLayout's CMake release output. Defaults to ci.yml's build path.
    [string]$SeamlyLayoutBuildDir,

    # windeployqt.exe from the Qt kit that built SeamlyLayout.
    [Parameter(Mandatory = $true)]
    [string]$WinDeployQt,

    # Skip the ICE validation pass.
    [switch]$SkipValidation,

    # Staging/output directory name under packaging\windows\. Keep in sync
    # with .gitignore and ci.yml's windows-msi artifact/signing paths.
    [string]$OutputDirName = 'seamly-msi'
)

# Stop on any PowerShell-level error; native tool failures are checked via
# exit codes after each call.
$ErrorActionPreference = 'Stop'

# The script lives in <repo-root>\packaging\windows\, so the repo root
# is two directories up.
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

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
    # Native tools can write stderr on success (e.g. windeployqt's optional
    # Qt6SerialPort warning). PS 5.1 would turn that into a terminating error
    # under 'Stop', so relax it here and judge success by exit code alone.
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
# MSI ProductVersion allows major<=255, minor<=255, build<=65535, and ignores
# the 4th field for upgrade comparisons. The project's YY.M.D.MMMM scheme
# needs a 3-part mapping: YY.M.((D-1)*1440 + MMMM). The third field encodes
# day+time as minutes-of-month (max 44639 < 65535), so it strictly increases
# build over build and MajorUpgrade always sees newer builds as newer.
#
# This mapping matches the earlier YYYY.M.D.HHMM scheme, so old and new
# packages still upgrade each other.
#
# @param  ProjectVersion  version string YY.M.D.MMMM
# @return the derived x.y.z MSI version string
#------------------------------------------------------------------------------
function ConvertTo-MsiVersion {
    param([string]$ProjectVersion)

    $parts = $ProjectVersion.Split('.')
    if ($parts.Count -ne 4 -or ($parts | Where-Object { $_ -notmatch '^\d+$' })) {
        throw "Version '$ProjectVersion' is not in the expected YY.M.D.MMMM form."
    }
    $year    = [int]$parts[0]
    $month   = [int]$parts[1]
    $day     = [int]$parts[2]
    $minutes = [int]$parts[3]
    if ($year -gt 255 -or $month -lt 1 -or $month -gt 12 -or $day -lt 1 -or $day -gt 31) {
        throw "Version '$ProjectVersion' has out-of-range date fields."
    }
    if ($minutes -gt 1439) {
        throw "Version '$ProjectVersion' has an out-of-range minute-of-day field."
    }

    $minutesOfMonth = (($day - 1) * 1440) + $minutes
    return "$year.$month.$minutesOfMonth"
}

#------------------------------------------------------------------------------
# @brief  Locate the MSVC CRT redistributable DLL directory for an architecture.
#
# Reads VCToolsRedistDir, set by the MSVC developer environment (vcvars, or
# ilammy/msvc-dev-cmd in ci.yml). Returns the Microsoft.VC*.CRT folder holding
# msvcp140.dll, vcruntime140.dll, etc., which the script copies app-locally
# (no merge modules, no vc_redist.exe chaining — see
# packaging\windows\README.md). This keeps the shipped CRT matched to the
# toolset that compiled the exes.
#
# @param  Architecture  'x64' or 'arm64'
# @return Full path of the CRT DLL directory.
#------------------------------------------------------------------------------
function Find-CrtDirectory {
    param([string]$Architecture)

    if (-not $env:VCToolsRedistDir) {
        throw "VCToolsRedistDir is not set - run this script inside the MSVC developer environment for '$Architecture'."
    }
    $archDir = Join-Path ($env:VCToolsRedistDir).TrimEnd('\') $Architecture
    if (Test-Path $archDir) {
        $crt = Get-ChildItem $archDir -Directory -Filter 'Microsoft.VC*.CRT' -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($crt) { return $crt.FullName }
    }
    throw "MSVC CRT redistributable for '$Architecture' not found under '$archDir' (VCToolsRedistDir = '$env:VCToolsRedistDir')."
}

# --- Resolve inputs and tools (fail early with clear messages) ----------------
if (-not $SeamlyLayoutBuildDir) { $SeamlyLayoutBuildDir = Join-Path $repoRoot 'src\app\seamlylayout\qt_frontend\build\Release' }

foreach ($required in @(
        @{ Path = (Join-Path $Seamly2DBin 'seamly2d.exe'); What = 'seamly2d.exe (-Seamly2DBin must name a completed release build)' },
        @{ Path = (Join-Path $Seamly2DBin 'platforms');    What = "seamly2d's windeployqt output (platforms\ plugin dir)" },
        @{ Path = (Join-Path $SeamlyMeBin 'seamlyme.exe'); What = 'seamlyme.exe (-SeamlyMeBin must name a completed release build)' })) {
    if (-not (Test-Path $required.Path)) {
        throw "Missing $($required.What): '$($required.Path)'."
    }
}
if (-not (Test-Path (Join-Path $SeamlyLayoutBuildDir 'seamlylayout.exe'))) {
    throw "Missing seamlylayout.exe in '$SeamlyLayoutBuildDir' - run the CMake release build first, or point -SeamlyLayoutBuildDir at its output."
}

# The .wxs builds its dialogs on WixUI_Common, from the WiX UI extension.
# Pinned to v6: WiX v7 refuses to run until its Open Source Maintenance Fee
# EULA is accepted (error WIX7015). The UI extension version must match the
# installed core tool version.
if (-not (Get-Command wix -ErrorAction SilentlyContinue)) {
    throw "The WiX toolset is not installed - run: dotnet tool install --global wix --version '6.*'"
}
$installedExtensions = (& wix extension list --global 2>$null)
if (-not ($installedExtensions -match 'WixToolset\.UI\.wixext')) {
    throw "The WiX UI extension is missing - run: wix extension add --global WixToolset.UI.wixext/<wix version, e.g. 6.0.2>"
}
# Util provides RemoveFolderEx, used to remove the old NSIS install's directory
# tree and Start Menu folder. Both paths resolve at install time, so the fixed
# RemoveFile/RemoveFolder rows can't express them or delete recursively.
if (-not ($installedExtensions -match 'WixToolset\.Util\.wixext')) {
    throw "The WiX Util extension is missing - run: wix extension add --global WixToolset.Util.wixext/<wix version, e.g. 6.0.2>"
}

if (-not (Test-Path $WinDeployQt)) {
    throw "windeployqt not found at '$WinDeployQt'."
}
$crtDir = Find-CrtDirectory -Architecture $Arch
$msiVersion = ConvertTo-MsiVersion -ProjectVersion $Version

Write-Host "arch        : $Arch"
Write-Host "version     : $Version  (MSI ProductVersion $msiVersion)"
Write-Host "seamly2d    : $Seamly2DBin"
Write-Host "seamlyme    : $SeamlyMeBin"
Write-Host "seamlylayout: $SeamlyLayoutBuildDir"
Write-Host "windeployqt : $WinDeployQt"
Write-Host "msvc crt    : $crtDir"

# --- Stage ---------------------------------------------------------------------
# Fresh staging tree per run, anchored under $PSScriptRoot:
# <repo>\packaging\windows\<OutputDirName>\<arch>\{parent,exes}
# 'parent' is the one shared runtime tree for every app in the package.
# A new -OutputDirName needs a matching .gitignore entry and CI artifact-path
# update, or the ~165 MB package becomes committable.
$stageRoot = Join-Path $PSScriptRoot (Join-Path $OutputDirName $Arch)
if (Test-Path $stageRoot) {
    Remove-Item $stageRoot -Recurse -Force
}
$parentDir = Join-Path $stageRoot 'parent'
$exesDir   = Join-Path $stageRoot 'exes'
New-Item -ItemType Directory -Force -Path $parentDir, $exesDir | Out-Null

# seamly2d + seamlyme merge into one tree; they share the same Qt release,
# so the overlapping DLLs are identical.
Write-Host "staging seamly2d + seamlyme runtime..."
Copy-Item -Path (Join-Path $Seamly2DBin '*') -Destination $parentDir -Recurse
Copy-Item -Path (Join-Path $SeamlyMeBin '*') -Destination $parentDir -Recurse -Force

# Exes are authored explicitly in the .wxs (shortcuts/associations reference
# them), so move them out of the wildcard-harvested tree.
Move-Item -Path (Join-Path $parentDir 'seamly2d.exe') -Destination $exesDir
Move-Item -Path (Join-Path $parentDir 'seamlyme.exe') -Destination $exesDir

Write-Host "staging SeamlyLayout runtime (windeployqt) into the shared tree..."

# Deploy against a staged copy to keep the build tree pristine.
# --qmldir resolves the app's QML imports.
Copy-Item -Path (Join-Path $SeamlyLayoutBuildDir 'seamlylayout.exe') -Destination $parentDir
$qmlDir = Join-Path $repoRoot 'src\app\seamlylayout\qt_frontend\qml'
Invoke-Tool -Description 'windeployqt' -Exe $WinDeployQt -Arguments @(
    '--qmldir', $qmlDir, '--release', (Join-Path $parentDir 'seamlylayout.exe'))

# Package default settings for SeamlyLayout's first run and legacy migration.
# Exclude preferences.json because it contains user-specific paths.
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

Move-Item -Path (Join-Path $parentDir 'seamlylayout.exe') -Destination $exesDir

# App-local CRT deployment: DLL resolution is per-directory, so the shared
# install directory needs only one copy for all three apps.
Write-Host "staging MSVC CRT runtime..."
Copy-Item -Path (Join-Path $crtDir '*.dll') -Destination $parentDir -Force

# --- Build the MSI -------------------------------------------------------------
# Build EVERY .wxs in this directory, not just smsi.wxs. Authoring is split
# into smsi.wxs (the Package) plus one fragment per area — smsi_ui,
# smsi_legacy, smsi_files, smsi_shortcuts, smsi_registry. `wix build` links
# whatever it is given and stays silent about a missing fragment, so the MSI
# would install wrong without complaint. Globbing picks up new fragments with
# no change here.
$wxsFiles = @(Get-ChildItem -Path $PSScriptRoot -Filter '*.wxs' | Sort-Object Name | ForEach-Object { $_.FullName })
if ($wxsFiles.Count -eq 0) {
    throw "No .wxs source files found in '$PSScriptRoot'."
}
Write-Host "authoring    : $((($wxsFiles | Split-Path -Leaf)) -join ', ')"
$msi = Join-Path $stageRoot "seamly-$Arch.msi"

$wixArguments = @('build') + $wxsFiles + @(
    '-arch', $Arch,
    # Suppress the .wixpdb symbol database: only used for patch/melt diffing,
    # not by the shipped installer.
    '-pdbtype', 'none',
    '-ext', 'WixToolset.UI.wixext',
    '-ext', 'WixToolset.Util.wixext',
    '-d', "ProductVersion=$msiVersion",
    '-d', "DisplayVersion=$Version",
    '-d', "RepoRoot=$repoRoot",
    # SeamlyLayout's runtime is merged into ParentStagingDir, so the .wxs
    # harvests one runtime tree plus one exe tree.
    '-d', "ParentStagingDir=$parentDir",
    '-d', "ExeStagingDir=$exesDir",
    '-o', $msi
)

Write-Host "running wix build..."
Invoke-Tool -Description 'wix build' -Exe 'wix' -Arguments $wixArguments

if (-not (Test-Path $msi)) {
    throw "wix build reported success but '$msi' is missing."
}

# --- Trim overflowing dialog lines ---------------------------------------------
# WixUI's stock dialogs give every BannerLine/BottomLine a width 3 installer
# units past the dialog's right edge, so Windows Installer logs Error 2826 per
# control. These rows come from WixToolset.UI.wixext and can't be changed from
# the .wxs, so the fix runs on the built package — before validation, so the
# ICE pass and authoring check both see the shipped package.
Write-Host "trimming overflowing dialog lines..."
& (Join-Path $PSScriptRoot 'smsi_fix_dialog_lines.ps1') -Msi $msi
if ($LASTEXITCODE -ne 0) {
    throw "dialog line trim failed (exit code $LASTEXITCODE) - see output above."
}

# --- Validate (ICE checks) -----------------------------------------------------
# Two ICEs are suppressed, both false positives from the optional
# desktop-shortcut components:
#
#   ICE43  "non-advertised shortcut ... KeyPath should fall under HKCU"
#   ICE57  "per-user and per-machine data with a per-machine KeyPath"
#
# Both assume DesktopFolder sits in the installing user's profile, true only
# for a per-user install. This package is Scope="perMachine" with ALLUSERS=1,
# so DesktopFolder resolves to the common desktop and the HKLM key path is
# correct. Following the ICEs would break the package: the per-machine install
# runs elevated as LocalSystem, so an HKCU key path would land in the SYSTEM
# hive, where component detection can never find it again, and every launch
# would trigger installer self-repair. The shortcuts can't be advertised
# instead — an advertised shortcut must live in the component owning its
# target file, which would stop them being optional.
#
# ICE61 stays visible and is expected: a known consequence of
# MajorUpgrade/@AllowSameVersionUpgrades.
if (-not $SkipValidation) {
    Write-Host "running wix msi validate (ICE checks)..."
    Invoke-Tool -Description 'wix msi validate' -Exe 'wix' -Arguments @(
        'msi', 'validate', $msi, '-sice', 'ICE43', '-sice', 'ICE57')
}

# --- Check the install-time authoring  --------------------------------
# ICE checks say the package is well formed; this checks it still has the
# shortcuts, associations, registry rows, elevation, upgrade detection and
# dialogs the project expects. Runs on every build: a WixUI or WiX change
# that drops a row would otherwise produce an MSI that installs cleanly and
# silently does the wrong thing.
Write-Host "checking install-time authoring..."
# A hashtable, not an array: @array splats positionally, @hashtable by name.
$checkArguments = @{ Msi = $msi; Arch = $Arch }
& (Join-Path $PSScriptRoot 'smsi_check_authoring.ps1') @checkArguments
if ($LASTEXITCODE -ne 0) {
    throw "install-time authoring check failed (exit code $LASTEXITCODE) - see output above."
}

Write-Host "checking user-data seeding..."
& (Join-Path $PSScriptRoot 'smsi_ensure_user_data_test.ps1')
if ($LASTEXITCODE -ne 0) {
    throw "user-data seeding check failed (exit code $LASTEXITCODE) - see output above."
}

$msiSize = [math]::Round((Get-Item $msi).Length / 1MB, 1)
Write-Host ''
Write-Host "MSI OK: $msi ($msiSize MB)"
