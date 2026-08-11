#******************************************************************************
# **  @file   test_msi_install.ps1
# **  @author slspencer
# **  @date   July 29, 2026
# **
# **  @brief
# **  Verify an INSTALLED Seamly2D MSI on a real machine (Task 51): the files
# **  that landed, the Start Menu and desktop shortcuts, the install-info
# **  registry rows, the Add/Remove Programs entry, the file associations, that
# **  each app actually starts, and — after uninstall — that every one of those
# **  is gone while the user's own data is still there.
# **
# **  This is the runtime half of Task 51's verification. Its sibling
# **  test_msi_authoring.ps1 reads the .msi database and checks what the PACKAGE
# **  CONTAINS; this script checks what a real elevated install actually DID.
# **  Neither can replace the other: authoring passes on a package whose exes
# **  cannot start, and this one cannot run without a machine to install on.
# **
# **  The script is deliberately standalone — no repository files, no modules,
# **  no build tree — so it can be copied to a clean test machine beside the
# **  .msi and run there. Windows PowerShell 5.1 is enough.
# **
# **  HOW IT IS USED
# **    Four phases, run in order around the msiexec commands, sharing a state
# **    file so each phase can compare against the ones before it:
# **
# **      .\test_msi_install.ps1 -Phase Baseline      <- BEFORE installing
# **      msiexec /i Seamly-x64-older.msi
# **      .\test_msi_install.ps1 -Phase Installed -ExpectSeamlyLayout
# **      msiexec /i Seamly-x64-newer.msi           <- upgrade over the top
# **      .\test_msi_install.ps1 -Phase Upgraded -ExpectSeamlyLayout
# **      msiexec /x Seamly-x64-newer.msi
# **      .\test_msi_install.ps1 -Phase Removed
# **
# **    Run it elevated. Reading HKLM and enumerating Program Files works
# **    unelevated, but the phases are run either side of msiexec, which is
# **    elevated anyway, and a non-admin run can silently miss registry rows.
# **
# **  WHAT IT STILL CANNOT SEE, and which therefore stays in README.md's manual
# **  checklist: what the UAC prompt looks like, whether the wizard pages appear
# **  in the right order with the right wording, and whether Explorer paints the
# **  right icons. Those need human eyes.
# **
# **  @copyright
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
# **  SeamlyLayout is licensed under the MIT license.
#******************************************************************************

<#
.SYNOPSIS
    Check what an installed Seamly2D MSI actually did to the machine (Task 51).

.DESCRIPTION
    Asserts one expectation at a time, printing "ok", "FAILED" or "note" per
    check and a summary at the end. Exits 1 if any check failed.

    State (the user-data inventory, the install path, the version installed) is
    carried between phases in a JSON file so the Removed phase can prove that
    uninstall took away the program and left the user's work alone.

.PARAMETER Phase
    Baseline   before installing anything: records the machine's starting state
               and asserts the product is not already installed.
    Installed  after the first install.
    Upgraded   after installing a newer build over the first one.
    Removed    after uninstalling.

.PARAMETER ExpectSeamlyLayout
    Assert SeamlyLayout.exe and its Start Menu shortcut are part of the install.
    Omit for the arm64 package, which ships the two parent apps only.

.PARAMETER NoDesktopShortcuts
    Assert desktop shortcuts are ABSENT — for the run where the Shortcuts page
    checkbox was unticked, or an install with SEAMLYDESKTOPSHORTCUTS=0.

.PARAMETER PatternFile
    Optional .sm2d file. When given, the Installed and Upgraded phases open it
    through the shell association (the same path Explorer takes on a
    double-click) and check seamly2d starts.

.PARAMETER SkipLaunch
    Do not start the applications. Launching is the only check that proves the
    deployed Qt runtime is complete, so skip it only when the machine cannot
    show a GUI.

.PARAMETER StateFile
    Where the cross-phase state is kept. Defaults to
    %LOCALAPPDATA%\seamly-msi-install-test\state.json — deliberately NOT under
    %LOCALAPPDATA%\Seamly, which is one of the trees being checked.

.EXAMPLE
    .\test_msi_install.ps1 -Phase Installed -ExpectSeamlyLayout -PatternFile C:\test\shirt.sm2d
#>

param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Baseline', 'Installed', 'Upgraded', 'Removed')]
    [string]$Phase,

    [switch]$ExpectSeamlyLayout,

    [switch]$NoDesktopShortcuts,

    [string]$PatternFile,

    [switch]$SkipLaunch,

    [string]$StateFile = (Join-Path $env:LOCALAPPDATA 'seamly-msi-install-test\state.json')
)

$ErrorActionPreference = 'Stop'

# The family UpgradeCode from seamly-family.wxs. Fixed for the lifetime of the
# product and shared by x64 and arm64, so it is the one reliable way to find
# "our" installed product and tell it apart from the old NSIS Seamly2D, which
# carries the same DisplayName.
$script:upgradeCode = '{CBF4B5F1-C32C-4DBB-B385-3EE4A7B30658}'

# Every failed check is recorded rather than thrown, so one run reports
# everything that is wrong instead of only the first problem.
$script:failures = @()

#------------------------------------------------------------------------------
# @brief  Record the outcome of one expectation and print it.
#
# @param  Name       what is being checked, in the imperative
# @param  Succeeded  result of the check
# @param  Detail     extra text shown when the check failed
#------------------------------------------------------------------------------
function Assert-That {
    param(
        [string]$Name,
        [bool]$Succeeded,
        [string]$Detail = ''
    )
    if ($Succeeded) {
        Write-Host "  ok      $Name"
    } else {
        Write-Host "  FAILED  $Name$(if ($Detail) { " - $Detail" })"
        $script:failures += $Name
    }
}

#------------------------------------------------------------------------------
# @brief  Print an observation that is useful to a human but is not pass/fail.
#
# Used for the things that depend on the machine rather than on the package —
# whether the old NSIS install happens to be present, what the effective
# per-user file association resolves to, and so on.
#
# @param  Text  the observation
#------------------------------------------------------------------------------
function Write-Note {
    param([string]$Text)
    Write-Host "  note    $Text"
}

#------------------------------------------------------------------------------
# @brief  Count the files and bytes under a directory tree.
#
# Used to prove that uninstall did not take any user data with it. Access
# errors are ignored rather than thrown: a cloud-synced data root (the
# G:\My Drive\seamlyData case) can hold placeholder files the enumerator
# cannot always stat, and the comparison only ever asks whether the tree
# shrank.
#
# @param  Path  directory to inventory
# @return PSCustomObject with Path, Exists, FileCount and TotalBytes
#------------------------------------------------------------------------------
function Get-TreeInventory {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return [pscustomobject]@{ Path = $Path; Exists = $false; FileCount = 0; TotalBytes = 0 }
    }
    $files = @(Get-ChildItem -LiteralPath $Path -Recurse -File -Force -ErrorAction SilentlyContinue)
    $bytes = 0
    foreach ($file in $files) { $bytes += $file.Length }
    return [pscustomobject]@{
        Path       = $Path
        Exists     = $true
        FileCount  = $files.Count
        TotalBytes = $bytes
    }
}

#------------------------------------------------------------------------------
# @brief  Work out where the user's pattern/measurement data root is.
#
# The apps store it as paths/dataRoot in the shared common settings file; when
# that is absent (a machine where the apps have never run) the Task 34 default
# applies.
#
# @return absolute path of the data root
#------------------------------------------------------------------------------
function Get-DataRootPath {
    $commonIni = Join-Path $env:APPDATA 'Seamly\qt6_common.ini'
    if (Test-Path -LiteralPath $commonIni) {
        foreach ($line in (Get-Content -LiteralPath $commonIni -ErrorAction SilentlyContinue)) {
            if ($line -match '^\s*dataRoot\s*=\s*(.+?)\s*$') {
                # QSettings writes forward slashes even on Windows.
                return ($matches[1] -replace '/', '\')
            }
        }
    }
    return (Join-Path $env:USERPROFILE 'seamlyData')
}

#------------------------------------------------------------------------------
# @brief  Take an inventory of every tree the installer promises not to touch.
#
# The set is deliberately FIXED rather than derived solely from the live
# configuration, for two reasons found during the Task 51 laptop run:
#
#  - On a machine upgrading from the old NSIS build, ~/seamly2d already exists,
#    so VCommonSettings::chooseFirstRunDataRoot() ADOPTS it as the data root
#    instead of using ~/seamlyData. The user's patterns then live in ~/seamly2d
#    and an inventory of ~/seamlyData alone watches an empty directory - exactly
#    the case this check exists to cover.
#  - Get-DataRootPath follows the configured root, which CHANGES the moment the
#    apps first run and write paths/dataRoot. Baseline would then inventory one
#    directory and a later phase a different one; because Assert-UserDataIntact
#    matches on Path, the baseline entry would find no counterpart and report a
#    failure that means nothing.
#
# Listing the configured root, both candidate roots and both settings folders
# keeps the comparison stable across phases whichever root ends up live.
#
# @return array of inventory objects, de-duplicated by path
#------------------------------------------------------------------------------
function Get-UserDataInventory {
    # Documents is resolved through the shell rather than assumed to be
    # %USERPROFILE%\Documents: it is routinely redirected, and a OneDrive-backed
    # profile puts it somewhere else entirely - which is exactly why the app
    # resolves it through QStandardPaths rather than building the path by hand.
    $documents = [Environment]::GetFolderPath('MyDocuments')

    $paths = @(
        (Get-DataRootPath),
        (Join-Path $documents 'Seamly'),          # the Task 60 root
        (Join-Path $env:USERPROFILE 'seamlyData'), # Task 53's root
        (Join-Path $env:USERPROFILE 'seamly2d'),   # the original, still the source of a migration
        (Join-Path $env:LOCALAPPDATA 'Seamly'),
        (Join-Path $env:APPDATA 'Seamly')
    )

    $seen = @{}
    $inventory = @()
    foreach ($path in $paths) {
        $key = $path.TrimEnd('\').ToLowerInvariant()
        if ($seen.ContainsKey($key)) { continue }
        $seen[$key] = $true
        $inventory += (Get-TreeInventory -Path $path)
    }
    return $inventory
}

#------------------------------------------------------------------------------
# @brief  Find the installed MSI product belonging to the Seamly family.
#
# Matched on the fixed UpgradeCode rather than on the display name, because the
# old NSIS installation is also called "Seamly2D" in Apps & features. Falls
# back to a registry scan if the Windows Installer COM API is unavailable.
#
# @return PSCustomObject describing the product, or $null when not installed
#------------------------------------------------------------------------------
function Get-InstalledSeamlyProduct {
    $productCode = $null
    try {
        $installer = New-Object -ComObject WindowsInstaller.Installer
        $related = $installer.GetType().InvokeMember(
            'RelatedProducts', 'GetProperty', $null, $installer, @($script:upgradeCode))
        if ($null -ne $related) {
            $count = $related.GetType().InvokeMember('Count', 'GetProperty', $null, $related, $null)
            if ($count -ge 1) {
                $productCode = [string]$related.GetType().InvokeMember(
                    'Item', 'GetProperty', $null, $related, @(0))
            }
        }
    } catch {
        # No related products, or the COM API refused - fall through to the
        # registry scan below.
        $productCode = $null
    }

    $uninstallRoots = @(
        'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
    )

    if ($productCode) {
        foreach ($root in $uninstallRoots) {
            $key = Join-Path $root $productCode
            if (Test-Path -LiteralPath $key) {
                $properties = Get-ItemProperty -LiteralPath $key
                return [pscustomobject]@{
                    ProductCode = $productCode
                    RegistryKey = $key
                    Properties  = $properties
                }
            }
        }
    }

    # Fallback: an MSI-installed Seamly2D is the one whose UninstallString runs
    # msiexec; the NSIS one runs its own uninstall.exe.
    foreach ($root in $uninstallRoots) {
        $candidates = Get-ChildItem -LiteralPath $root -ErrorAction SilentlyContinue |
            ForEach-Object { Get-ItemProperty -LiteralPath $_.PSPath -ErrorAction SilentlyContinue } |
            Where-Object { $_.DisplayName -eq 'Seamly2D' -and $_.UninstallString -match 'msiexec' }
        if ($candidates) {
            return [pscustomobject]@{
                ProductCode = $candidates[0].PSChildName
                RegistryKey = $candidates[0].PSPath
                Properties  = $candidates[0]
            }
        }
    }
    return $null
}

#------------------------------------------------------------------------------
# @brief  Locate the old NSIS Seamly2D installation, if present.
#
# The NSIS installer is 32-bit and never switches registry views, so its keys
# are always under WOW6432Node — the same reason seamly-family.wxs searches
# with Bitness="always32".
#
# @return install directory, or an empty string when it is not installed
#------------------------------------------------------------------------------
function Get-LegacyInstallDir {
    $key = 'HKLM:\SOFTWARE\WOW6432Node\NSIS_Seamly2D'
    if (Test-Path -LiteralPath $key) {
        $value = (Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue).Install_Dir
        if ($value) { return [string]$value }
    }
    return ''
}

#------------------------------------------------------------------------------
# @brief  Read the install-info key the package writes.
#
# @return PSCustomObject of the key's values, or $null when the key is absent
#------------------------------------------------------------------------------
function Get-InstallInfo {
    $key = 'HKLM:\SOFTWARE\Seamly\Seamly2D'
    if (-not (Test-Path -LiteralPath $key)) { return $null }
    return Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
}

#------------------------------------------------------------------------------
# @brief  Resolve a NON-advertised shortcut's target path.
#
# Only trustworthy for a shortcut that stores a literal path - the desktop ones,
# authored with Target="[INSTALLFOLDER]...". For an advertised shortcut this
# returns the extracted ICON path, not the target, so callers must try
# Get-AdvertisedShortcutTarget first. See the comment there.
#
# @param  LinkPath  full path of the .lnk
# @return target path, or an empty string
#------------------------------------------------------------------------------
function Get-ShortcutTarget {
    param([string]$LinkPath)
    try {
        $shell = New-Object -ComObject WScript.Shell
        return [string]$shell.CreateShortcut($LinkPath).TargetPath
    } catch {
        return ''
    }
}

# msi.dll entry points for resolving advertised shortcuts. Defined once; the
# guard keeps a re-run in the same session from failing on a duplicate type.
if (-not ('Seamly.MsiShortcut' -as [type])) {
    Add-Type -Namespace Seamly -Name MsiShortcut -MemberDefinition @'
[DllImport("msi.dll", CharSet = CharSet.Unicode)]
public static extern uint MsiGetShortcutTarget(string szShortcutPath,
                                               System.Text.StringBuilder szProductCode,
                                               System.Text.StringBuilder szFeatureId,
                                               System.Text.StringBuilder szComponentCode);

[DllImport("msi.dll", CharSet = CharSet.Unicode)]
public static extern int MsiGetComponentPath(string szProduct,
                                             string szComponent,
                                             System.Text.StringBuilder lpPathBuf,
                                             ref uint pcchBuf);
'@
}

#------------------------------------------------------------------------------
# @brief  Resolve an advertised shortcut through the Windows Installer.
#
# The Start Menu shortcuts are advertised: seamly-family.wxs nests each one
# inside its <File KeyPath="yes">, with no Target attribute, which is WiX's
# standard pattern for a shortcut that carries a Darwin descriptor (product,
# feature and component GUIDs) instead of a path. The desktop shortcuts set
# Target="[INSTALLFOLDER]..." explicitly and are ordinary path shortcuts.
#
# The distinction matters because WScript.Shell does NOT report an advertised
# shortcut's target. It hands back the icon Windows Installer extracted to
# %WINDIR%\Installer\{ProductCode}\<name>.ico - a real, non-empty path that
# points nowhere near the install directory. Asserting on it fails every time,
# which is exactly what this script used to do; it assumed an unresolvable
# advertised shortcut would come back EMPTY, and nothing here ever hit that
# branch. Found by the Task 51 install run, where all three Start Menu
# shortcuts "failed" while being perfectly correct.
#
# MsiGetShortcutTarget reads the descriptor, and MsiGetComponentPath turns the
# component GUID into the installed file it currently resolves to - which is
# the thing worth asserting: not merely that a .lnk exists, but that clicking
# it reaches an executable inside this install.
#
# @param  LinkPath  full path of the .lnk
# @return PSCustomObject with ProductCode, ComponentPath and InstallState, or
#         $null when the shortcut is not advertised (use Get-ShortcutTarget)
#------------------------------------------------------------------------------
function Get-AdvertisedShortcutTarget {
    param([string]$LinkPath)

    # GUID buffers: 38 characters plus the terminator.
    $product   = New-Object System.Text.StringBuilder 39
    $feature   = New-Object System.Text.StringBuilder 39
    $component = New-Object System.Text.StringBuilder 39

    try {
        $result = [Seamly.MsiShortcut]::MsiGetShortcutTarget($LinkPath, $product, $feature, $component)
    } catch {
        return $null
    }

    # Anything but ERROR_SUCCESS means "not an advertised shortcut".
    if ($result -ne 0) { return $null }

    $buffer = New-Object System.Text.StringBuilder 1024
    $size   = [uint32]$buffer.Capacity
    $state  = [Seamly.MsiShortcut]::MsiGetComponentPath($product.ToString(), $component.ToString(),
                                                        $buffer, [ref]$size)

    # INSTALLSTATE_LOCAL (4) and INSTALLSTATE_SOURCE (5) are the states that
    # yield a usable path; anything else means the component is not installed.
    $path = if ($state -eq 4 -or $state -eq 5) { $buffer.ToString() } else { '' }

    return [PSCustomObject]@{
        ProductCode   = $product.ToString()
        ComponentPath = $path
        InstallState  = $state
    }
}

#------------------------------------------------------------------------------
# @brief  Start an application, confirm it stays running, then stop it.
#
# This is the only check that exercises the deployed Qt runtime: a missing DLL
# or QML module makes the process die within a second or two, which no amount
# of package inspection can reveal.
#
# @param  ExePath      executable to start
# @param  SettleSeconds  how long it must stay alive to count as started
# @return $true when the process was still running after the wait
#------------------------------------------------------------------------------
function Test-ApplicationStarts {
    param(
        [string]$ExePath,
        [int]$SettleSeconds = 6
    )
    $process = $null
    try {
        $process = Start-Process -FilePath $ExePath -PassThru -ErrorAction Stop
        Start-Sleep -Seconds $SettleSeconds
        $process.Refresh()
        $alive = -not $process.HasExited
        return $alive
    } catch {
        return $false
    } finally {
        if ($null -ne $process) {
            try { if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction Stop } } catch { }
        }
    }
}

#------------------------------------------------------------------------------
# @brief  Load the cross-phase state file.
#
# @return PSCustomObject of saved state, or $null when no phase has run yet
#------------------------------------------------------------------------------
function Read-State {
    if (-not (Test-Path -LiteralPath $StateFile)) { return $null }
    return (Get-Content -LiteralPath $StateFile -Raw | ConvertFrom-Json)
}

#------------------------------------------------------------------------------
# @brief  Save the cross-phase state file, creating its directory if needed.
#
# @param  State  object to persist
#------------------------------------------------------------------------------
function Write-State {
    param($State)
    $directory = Split-Path -Parent $StateFile
    if (-not (Test-Path -LiteralPath $directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
    $State | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $StateFile -Encoding utf8
}

#------------------------------------------------------------------------------
# @brief  Assert that no tree in a saved inventory has shrunk.
#
# "Never shrank" rather than "identical" on purpose: starting the applications
# legitimately creates settings and seeds the data tree, so an exact match
# would fail for the right reasons. What must never happen is a file
# disappearing.
#
# @param  Earlier  inventory recorded by a previous phase
# @param  Now      inventory taken in this phase
# @param  Since    name of the earlier phase, for the message
#------------------------------------------------------------------------------
function Assert-UserDataIntact {
    param($Earlier, $Now, [string]$Since)

    foreach ($before in $Earlier) {
        $after = $Now | Where-Object { $_.Path -eq $before.Path } | Select-Object -First 1
        if ($null -eq $after) {
            Assert-That -Name "user data '$($before.Path)' still inventoried" -Succeeded $false
            continue
        }
        if (-not $before.Exists) {
            Write-Note "'$($before.Path)' did not exist at $Since - nothing to preserve"
            continue
        }
        Assert-That -Name "user data '$($before.Path)' still exists" -Succeeded ([bool]$after.Exists)
        Assert-That -Name "user data '$($before.Path)' kept every file it had at $Since" `
            -Succeeded ($after.FileCount -ge $before.FileCount -and $after.TotalBytes -ge $before.TotalBytes) `
            -Detail "$($before.FileCount) files / $($before.TotalBytes) bytes at $Since, now $($after.FileCount) / $($after.TotalBytes)"
    }
}

#------------------------------------------------------------------------------
# @brief  Run every check that applies to a live installation.
#
# Shared by the Installed and Upgraded phases, which differ only in what they
# compare against — so the install itself is verified identically both times
# and an upgrade cannot quietly degrade the result.
#
# @param  InstallInfo  the HKLM\SOFTWARE\Seamly\Seamly2D values
# @return the resolved install directory
#------------------------------------------------------------------------------
function Invoke-InstalledChecks {
    param($InstallInfo)

    # --- files -----------------------------------------------------------------
    $installFolder = [string]$InstallInfo.InstallPath
    Assert-That -Name 'InstallPath in the registry points at a real directory' `
        -Succeeded (-not [string]::IsNullOrWhiteSpace($installFolder) -and (Test-Path -LiteralPath $installFolder)) `
        -Detail "InstallPath = '$installFolder'"
    if ([string]::IsNullOrWhiteSpace($installFolder) -or -not (Test-Path -LiteralPath $installFolder)) {
        return $installFolder
    }
    Write-Note "install directory: $installFolder"

    $expectedExes = @('seamly2d.exe', 'seamlyme.exe')
    if ($ExpectSeamlyLayout) { $expectedExes += 'SeamlyLayout.exe' }
    foreach ($exe in $expectedExes) {
        Assert-That -Name "$exe is installed" -Succeeded (Test-Path -LiteralPath (Join-Path $installFolder $exe))
    }
    if (-not $ExpectSeamlyLayout) {
        Assert-That -Name 'SeamlyLayout.exe is absent from this package' `
            -Succeeded (-not (Test-Path -LiteralPath (Join-Path $installFolder 'SeamlyLayout.exe')))
    }

    # A representative slice of the runtime rather than a full file list: the Qt
    # core, the platform plugin without which no Qt app shows a window, and the
    # MSVC CRT that is deployed app-locally instead of by a redist installer.
    $runtimeFiles = @('Qt6Core.dll', 'platforms\qwindows.dll', 'msvcp140.dll', 'vcruntime140.dll')
    if ($ExpectSeamlyLayout) {
        # SeamlyLayout's canvas is a WebEngineView, which cannot start without
        # its own helper process and resource pack.
        $runtimeFiles += @('QtWebEngineProcess.exe', 'Qt6WebEngineCore.dll', 'Qt6WebChannel.dll', 'Qt6Positioning.dll')
    }
    foreach ($file in $runtimeFiles) {
        Assert-That -Name "runtime file '$file' was installed" `
            -Succeeded (Test-Path -LiteralPath (Join-Path $installFolder $file))
    }

    # seamly2d resolves its daughter app flat-beside-itself first
    # (SeamlyFamilyPaths::locateSeamlyLayout), so this is what Layout Mode needs.
    if ($ExpectSeamlyLayout) {
        Assert-That -Name 'SeamlyLayout.exe sits beside seamly2d.exe, where Layout Mode looks for it' `
            -Succeeded ((Test-Path -LiteralPath (Join-Path $installFolder 'SeamlyLayout.exe')) -and
                        (Test-Path -LiteralPath (Join-Path $installFolder 'seamly2d.exe')))
    }

    # --- install-info registry rows -------------------------------------------
    Assert-That -Name 'the full project version is recorded in HKLM\SOFTWARE\Seamly\Seamly2D' `
        -Succeeded ([string]$InstallInfo.DisplayVersion -match '^\d{4}\.\d+\.\d+\.\d+$') `
        -Detail "DisplayVersion = '$($InstallInfo.DisplayVersion)'"

    $breadcrumbs = @('DesktopShortcutSeamly2D', 'DesktopShortcutSeamlyMe')
    foreach ($breadcrumb in $breadcrumbs) {
        $present = $null -ne $InstallInfo.PSObject.Properties[$breadcrumb]
        if ($NoDesktopShortcuts) {
            Assert-That -Name "$breadcrumb is absent (desktop shortcuts declined)" -Succeeded (-not $present)
        } else {
            Assert-That -Name "$breadcrumb records that the desktop shortcut was created" -Succeeded $present
        }
    }

    # --- Add/Remove Programs ---------------------------------------------------
    $product = Get-InstalledSeamlyProduct
    Assert-That -Name 'the product is listed in Apps & features' -Succeeded ($null -ne $product)
    if ($null -ne $product) {
        $arp = $product.Properties
        Write-Note "ARP ProductCode $($product.ProductCode), DisplayVersion $($arp.DisplayVersion)"
        Assert-That -Name 'ARP DisplayName is Seamly2D' -Succeeded ($arp.DisplayName -eq 'Seamly2D')
        Assert-That -Name 'ARP Publisher is set' -Succeeded (-not [string]::IsNullOrWhiteSpace($arp.Publisher)) `
            -Detail "Publisher = '$($arp.Publisher)'"
        # ARP can only ever show the numeric MSI ProductVersion (26.y.z): the
        # RegisterProduct standard action rewrites this value after the
        # component-authored registry rows are written.
        Assert-That -Name 'ARP DisplayVersion is the numeric MSI version' `
            -Succeeded ([string]$arp.DisplayVersion -match '^\d+\.\d+\.\d+$') `
            -Detail "DisplayVersion = '$($arp.DisplayVersion)'"
        Assert-That -Name 'ARP Comments carry the full project version' `
            -Succeeded ([string]$arp.Comments -match '\d{4}\.\d+\.\d+\.\d+') `
            -Detail "Comments = '$($arp.Comments)'"
        Assert-That -Name 'ARP estimated size is plausible (> 50 MB)' `
            -Succeeded ([int]$arp.EstimatedSize -gt 51200) `
            -Detail "EstimatedSize = $($arp.EstimatedSize) KB"
        Assert-That -Name 'ARP help link is set' -Succeeded (-not [string]::IsNullOrWhiteSpace($arp.HelpLink))
        Assert-That -Name 'ARP about link is set' -Succeeded (-not [string]::IsNullOrWhiteSpace($arp.URLInfoAbout))
        Assert-That -Name 'ARP icon is set' -Succeeded (-not [string]::IsNullOrWhiteSpace($arp.DisplayIcon))
        Assert-That -Name 'ARP uninstall runs msiexec' -Succeeded ([string]$arp.UninstallString -match 'msiexec')
    }

    # --- shortcuts -------------------------------------------------------------
    # Per-machine install, so both live in the All Users locations.
    $startMenu = Join-Path $env:ProgramData 'Microsoft\Windows\Start Menu\Programs'
    $publicDesktop = Join-Path $env:PUBLIC 'Desktop'

    $expectedShortcuts = @('Seamly2D', 'SeamlyMe')
    if ($ExpectSeamlyLayout) { $expectedShortcuts += 'SeamlyLayout' }
    foreach ($name in $expectedShortcuts) {
        $link = Join-Path $startMenu "$name.lnk"
        $exists = Test-Path -LiteralPath $link
        Assert-That -Name "Start Menu shortcut '$name' exists" -Succeeded $exists -Detail $link
        if ($exists) {
            # Advertised first: these shortcuts carry a Darwin descriptor, and
            # WScript.Shell would report their extracted .ico instead of the
            # target. Get-AdvertisedShortcutTarget returns $null for an ordinary
            # path shortcut, which is what the else branch is for.
            $advertised = Get-AdvertisedShortcutTarget -LinkPath $link
            if ($null -ne $advertised) {
                Write-Note "'$name' is advertised, product $($advertised.ProductCode), install state $($advertised.InstallState)"
                Assert-That -Name "Start Menu shortcut '$name' resolves to an installed file" `
                    -Succeeded (-not [string]::IsNullOrWhiteSpace($advertised.ComponentPath)) `
                    -Detail "MsiGetComponentPath returned install state $($advertised.InstallState)"
                if (-not [string]::IsNullOrWhiteSpace($advertised.ComponentPath)) {
                    Assert-That -Name "Start Menu shortcut '$name' resolves into the install directory" `
                        -Succeeded ($advertised.ComponentPath -like "$installFolder*") `
                        -Detail "resolves to '$($advertised.ComponentPath)'"
                }
            } else {
                $target = Get-ShortcutTarget -LinkPath $link
                Assert-That -Name "Start Menu shortcut '$name' points into the install directory" `
                    -Succeeded ($target -like "$installFolder*") -Detail "target = '$target'"
            }
        }
    }
    if (-not $ExpectSeamlyLayout) {
        Assert-That -Name 'no SeamlyLayout Start Menu shortcut in this package' `
            -Succeeded (-not (Test-Path -LiteralPath (Join-Path $startMenu 'SeamlyLayout.lnk')))
    }

    foreach ($name in @('Seamly2D', 'SeamlyMe')) {
        $link = Join-Path $publicDesktop "$name.lnk"
        $exists = Test-Path -LiteralPath $link
        if ($NoDesktopShortcuts) {
            Assert-That -Name "no desktop shortcut '$name' (checkbox was unticked)" -Succeeded (-not $exists)
        } else {
            Assert-That -Name "desktop shortcut '$name' exists" -Succeeded $exists -Detail $link
            if ($exists) {
                $target = Get-ShortcutTarget -LinkPath $link
                Assert-That -Name "desktop shortcut '$name' targets the installed executable" `
                    -Succeeded ($target -eq (Join-Path $installFolder "$($name.ToLower()).exe")) `
                    -Detail "target = '$target'"
            }
        }
    }
    # Deliberate: a bare SeamlyLayout launch would only ever show an empty
    # canvas, because seamly2d starts it with a .pieces.svg argument.
    Assert-That -Name 'SeamlyLayout has no desktop shortcut' `
        -Succeeded (-not (Test-Path -LiteralPath (Join-Path $publicDesktop 'SeamlyLayout.lnk')))

    # --- file associations -----------------------------------------------------
    foreach ($association in @(
            @{ Extension = '.sm2d'; ProgId = 'Seamly2D.Pattern';                Exe = 'seamly2d.exe' },
            @{ Extension = '.smis'; ProgId = 'SeamlyMe.IndividualMeasurements'; Exe = 'seamlyme.exe' },
            @{ Extension = '.smms'; ProgId = 'SeamlyMe.MultisizeMeasurements';  Exe = 'seamlyme.exe' })) {

        $extensionKey = "HKLM:\SOFTWARE\Classes\$($association.Extension)"
        $registered = ''
        if (Test-Path -LiteralPath $extensionKey) {
            $registered = [string](Get-ItemProperty -LiteralPath $extensionKey -ErrorAction SilentlyContinue).'(default)'
        }
        Assert-That -Name "$($association.Extension) is registered to $($association.ProgId)" `
            -Succeeded ($registered -eq $association.ProgId) -Detail "found '$registered'"

        $commandKey = "HKLM:\SOFTWARE\Classes\$($association.ProgId)\shell\open\command"
        $command = ''
        if (Test-Path -LiteralPath $commandKey) {
            $command = [string](Get-ItemProperty -LiteralPath $commandKey -ErrorAction SilentlyContinue).'(default)'
        }
        Assert-That -Name "$($association.ProgId) opens with the installed $($association.Exe)" `
            -Succeeded ($command -like "*$installFolder*$($association.Exe)*" -and $command -like '*%1*') `
            -Detail "command = '$command'"

        $iconKey = "HKLM:\SOFTWARE\Classes\$($association.ProgId)\DefaultIcon"
        Assert-That -Name "$($association.ProgId) has an Explorer icon" `
            -Succeeded (Test-Path -LiteralPath $iconKey)
    }

    # A per-user UserChoice overrides the machine-wide association, so report
    # the effective winner rather than asserting on it - on a machine where the
    # user has already picked an app for .sm2d, HKLM being right is all the
    # installer can be held to.
    foreach ($extension in @('.sm2d', '.smis', '.smms')) {
        $userChoiceKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\FileExts\$extension\UserChoice"
        if (Test-Path -LiteralPath $userChoiceKey) {
            $choice = (Get-ItemProperty -LiteralPath $userChoiceKey -ErrorAction SilentlyContinue).ProgId
            Write-Note "$extension has a per-user UserChoice of '$choice', which overrides the installer's association"
        }
    }

    # --- do the applications actually start? ----------------------------------
    if ($SkipLaunch) {
        Write-Note 'application launch checks skipped (-SkipLaunch)'
    } else {
        foreach ($exe in $expectedExes) {
            $path = Join-Path $installFolder $exe
            if (Test-Path -LiteralPath $path) {
                Assert-That -Name "$exe starts and stays running" -Succeeded (Test-ApplicationStarts -ExePath $path) `
                    -Detail 'the process exited immediately - usually a missing Qt DLL or QML module'
            }
        }

        if ($PatternFile) {
            if (Test-Path -LiteralPath $PatternFile) {
                # Start-Process on a data file goes through ShellExecute, which is
                # the same route Explorer takes for a double-click.
                $before = @(Get-Process -Name 'seamly2d' -ErrorAction SilentlyContinue).Count
                try {
                    Start-Process -FilePath $PatternFile -ErrorAction Stop
                    Start-Sleep -Seconds 10
                    $after = @(Get-Process -Name 'seamly2d' -ErrorAction SilentlyContinue)
                    Assert-That -Name 'opening a .sm2d through its association starts seamly2d' `
                        -Succeeded ($after.Count -gt $before)
                    foreach ($process in $after) {
                        try { Stop-Process -Id $process.Id -Force -ErrorAction Stop } catch { }
                    }
                } catch {
                    Assert-That -Name 'opening a .sm2d through its association starts seamly2d' `
                        -Succeeded $false -Detail $_.Exception.Message
                }
            } else {
                Write-Note "pattern file '$PatternFile' not found - association launch check skipped"
            }
        } else {
            Write-Note 'no -PatternFile given - the .sm2d association was checked in the registry but not opened'
        }
    }

    return $installFolder
}

# --- run the requested phase --------------------------------------------------
Write-Host "Seamly2D MSI install check - phase: $Phase"
Write-Host "state file: $StateFile"
Write-Host ''

$state = Read-State
$legacyInstallDir = Get-LegacyInstallDir
$userData = Get-UserDataInventory

switch ($Phase) {

    'Baseline' {
        # A run that starts with the product already installed would report
        # someone else's install, so this is a hard stop rather than a note.
        $product = Get-InstalledSeamlyProduct
        Assert-That -Name 'the Seamly family MSI is not already installed' -Succeeded ($null -eq $product) `
            -Detail "found ProductCode $(if ($product) { $product.ProductCode })  - uninstall it before starting"
        Assert-That -Name 'HKLM\SOFTWARE\Seamly\Seamly2D does not exist yet' -Succeeded ($null -eq (Get-InstallInfo))

        if ($legacyInstallDir) {
            Write-Note "the old NSIS installation IS present at '$legacyInstallDir' - the warning dialog's NSIS paragraph should appear during install"
        } else {
            Write-Note 'no old NSIS installation on this machine - the warning dialog should NOT appear on a first install'
        }

        foreach ($tree in $userData) {
            if ($tree.Exists) {
                Write-Note "user data '$($tree.Path)': $($tree.FileCount) files, $([math]::Round($tree.TotalBytes / 1MB, 1)) MB"
            } else {
                Write-Note "user data '$($tree.Path)': does not exist"
            }
        }

        Write-State -State ([pscustomobject]@{
            Phase          = 'Baseline'
            RecordedAt     = (Get-Date).ToString('o')
            BaselineData   = $userData
            LatestData     = $userData
            LegacyInstallDir = $legacyInstallDir
            InstallFolder  = ''
            DisplayVersion = ''
        })
    }

    'Installed' {
        if ($null -eq $state) { throw "No state file at '$StateFile' - run -Phase Baseline before installing." }

        $installInfo = Get-InstallInfo
        Assert-That -Name 'HKLM\SOFTWARE\Seamly\Seamly2D was created by the install' -Succeeded ($null -ne $installInfo)
        $installFolder = ''
        if ($null -ne $installInfo) {
            $installFolder = Invoke-InstalledChecks -InstallInfo $installInfo
        }

        # The installer must not have disturbed the separate NSIS product.
        # Task 51 step 2a INVERTED this expectation. The MSI used to detect the
        # old NSIS product and leave it alone; it now removes it, because the
        # MSI is a strict superset - NSIS ships seamly2d and seamlyme, this
        # package ships both plus SeamlyLayout - so leaving it behind means two
        # copies of each parent app and Start Menu shortcuts that launch the old
        # binaries. All four things the .nsi created must be gone.
        if ($state.LegacyInstallDir) {
            Assert-That -Name "the old NSIS install directory was removed" `
                -Succeeded (-not (Test-Path -LiteralPath $state.LegacyInstallDir)) `
                -Detail "'$($state.LegacyInstallDir)' still exists"
            Assert-That -Name 'the NSIS Install_Dir registry key was removed' `
                -Succeeded ([string]::IsNullOrEmpty((Get-LegacyInstallDir)))
            Assert-That -Name 'the NSIS Apps & features entry was removed' `
                -Succeeded (-not (Test-Path -LiteralPath 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Seamly2D'))

            # Per-user, because the .nsi never calls SetShellVarContext all.
            $nsisStartMenu = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Seamly2D'
            Assert-That -Name "the NSIS Start Menu folder was removed" `
                -Succeeded (-not (Test-Path -LiteralPath $nsisStartMenu)) -Detail $nsisStartMenu

            Assert-That -Name 'the family MSI installed somewhere other than the NSIS directory' `
                -Succeeded ($installFolder -and ($installFolder.TrimEnd('\') -ne ([string]$state.LegacyInstallDir).TrimEnd('\')))
        } else {
            Write-Note 'no NSIS installation was present at Baseline - the removal path was not exercised by this run'
        }

        Assert-UserDataIntact -Earlier $state.BaselineData -Now $userData -Since 'Baseline'

        $state.Phase = 'Installed'
        $state.LatestData = $userData
        # Only overwrite what was actually determined. A failed phase that
        # blanked InstallFolder would silently disable the Removed phase's
        # leftover-file check, turning one failure into a false pass later.
        if ($installFolder) { $state.InstallFolder = $installFolder }
        if ($installInfo) { $state.DisplayVersion = [string]$installInfo.DisplayVersion }
        Write-State -State $state
    }

    'Upgraded' {
        if ($null -eq $state) { throw "No state file at '$StateFile' - run the earlier phases first." }
        if ($state.Phase -eq 'Baseline') { throw 'Run -Phase Installed before -Phase Upgraded.' }

        $installInfo = Get-InstallInfo
        Assert-That -Name 'HKLM\SOFTWARE\Seamly\Seamly2D survived the upgrade' -Succeeded ($null -ne $installInfo)
        $installFolder = ''
        if ($null -ne $installInfo) {
            $installFolder = Invoke-InstalledChecks -InstallInfo $installInfo
        }

        # The whole point of MajorUpgrade: replace, never accumulate.
        $uninstallRoots = @(
            'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall',
            'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
        )
        $msiEntries = @()
        foreach ($root in $uninstallRoots) {
            $msiEntries += Get-ChildItem -LiteralPath $root -ErrorAction SilentlyContinue |
                ForEach-Object { Get-ItemProperty -LiteralPath $_.PSPath -ErrorAction SilentlyContinue } |
                Where-Object { $_.DisplayName -eq 'Seamly2D' -and $_.UninstallString -match 'msiexec' }
        }
        Assert-That -Name 'exactly one MSI entry in Apps & features after the upgrade (no duplicate)' `
            -Succeeded ($msiEntries.Count -eq 1) -Detail "found $($msiEntries.Count)"

        Assert-That -Name 'the upgrade installed a newer build' `
            -Succeeded ($installInfo -and [string]$installInfo.DisplayVersion -ne [string]$state.DisplayVersion) `
            -Detail "was '$($state.DisplayVersion)', now '$(if ($installInfo) { $installInfo.DisplayVersion })'"
        Assert-That -Name 'the upgrade kept the same install directory' `
            -Succeeded ($installFolder.TrimEnd('\') -eq ([string]$state.InstallFolder).TrimEnd('\')) `
            -Detail "was '$($state.InstallFolder)', now '$installFolder'"

        Assert-UserDataIntact -Earlier $state.LatestData -Now $userData -Since 'Installed'

        $state.Phase = 'Upgraded'
        $state.LatestData = $userData
        if ($installInfo) { $state.DisplayVersion = [string]$installInfo.DisplayVersion }
        Write-State -State $state
    }

    'Removed' {
        if ($null -eq $state) { throw "No state file at '$StateFile' - run the earlier phases first." }

        $installFolder = [string]$state.InstallFolder
        Assert-That -Name 'the product is gone from Apps & features' -Succeeded ($null -eq (Get-InstalledSeamlyProduct))
        Assert-That -Name 'HKLM\SOFTWARE\Seamly\Seamly2D was removed' -Succeeded ($null -eq (Get-InstallInfo))

        if ($installFolder) {
            # Windows Installer removes only what it installed, so a stray file
            # a user dropped in the folder legitimately keeps it alive - report
            # what is left rather than demanding the directory be gone.
            if (Test-Path -LiteralPath $installFolder) {
                $leftovers = @(Get-ChildItem -LiteralPath $installFolder -Recurse -File -Force -ErrorAction SilentlyContinue)
                Assert-That -Name 'the install directory holds no leftover files' -Succeeded ($leftovers.Count -eq 0) `
                    -Detail "$($leftovers.Count) file(s) remain in '$installFolder'"
                if ($leftovers.Count -gt 0) {
                    $leftovers | Select-Object -First 10 | ForEach-Object { Write-Note "leftover: $($_.FullName)" }
                }
            } else {
                Assert-That -Name 'the install directory was removed' -Succeeded $true
            }
        }

        $startMenu = Join-Path $env:ProgramData 'Microsoft\Windows\Start Menu\Programs'
        $publicDesktop = Join-Path $env:PUBLIC 'Desktop'
        foreach ($name in @('Seamly2D', 'SeamlyMe', 'SeamlyLayout')) {
            Assert-That -Name "Start Menu shortcut '$name' was removed" `
                -Succeeded (-not (Test-Path -LiteralPath (Join-Path $startMenu "$name.lnk")))
            Assert-That -Name "desktop shortcut '$name' was removed" `
                -Succeeded (-not (Test-Path -LiteralPath (Join-Path $publicDesktop "$name.lnk")))
        }

        foreach ($association in @(
                @{ Extension = '.sm2d'; ProgId = 'Seamly2D.Pattern' },
                @{ Extension = '.smis'; ProgId = 'SeamlyMe.IndividualMeasurements' },
                @{ Extension = '.smms'; ProgId = 'SeamlyMe.MultisizeMeasurements' })) {
            Assert-That -Name "the $($association.ProgId) association was removed" `
                -Succeeded (-not (Test-Path -LiteralPath "HKLM:\SOFTWARE\Classes\$($association.ProgId)"))
            # The extension key itself may survive if something else claims it;
            # what matters is that it no longer points at our ProgId.
            $extensionKey = "HKLM:\SOFTWARE\Classes\$($association.Extension)"
            $registered = ''
            if (Test-Path -LiteralPath $extensionKey) {
                $registered = [string](Get-ItemProperty -LiteralPath $extensionKey -ErrorAction SilentlyContinue).'(default)'
            }
            Assert-That -Name "$($association.Extension) no longer resolves to $($association.ProgId)" `
                -Succeeded ($registered -ne $association.ProgId) -Detail "still '$registered'"
        }

        # The promise the warning dialog makes, checked for real.
        Assert-UserDataIntact -Earlier $state.LatestData -Now $userData -Since 'the last installed phase'
        Assert-UserDataIntact -Earlier $state.BaselineData -Now $userData -Since 'Baseline'

        # The NSIS product was removed during install (step 2a) and is NOT
        # restored by uninstalling this one - Windows Installer only puts back
        # what it installed, and the old product was never ours to reinstate.
        # Asserted so the state is recorded deliberately rather than assumed:
        # a machine that had the old product and then removes this one ends up
        # with neither, which is the intended outcome but worth pinning.
        if ($state.LegacyInstallDir) {
            Assert-That -Name 'the old NSIS installation stays removed after uninstall' `
                -Succeeded (-not (Test-Path -LiteralPath $state.LegacyInstallDir)) `
                -Detail "'$($state.LegacyInstallDir)' reappeared"
        }

        $state.Phase = 'Removed'
        $state.LatestData = $userData
        Write-State -State $state
    }
}

# --- report --------------------------------------------------------------------
Write-Host ''
if ($script:failures.Count -gt 0) {
    Write-Host "MSI install check FAILED at phase '$Phase' - $($script:failures.Count) problem(s):"
    $script:failures | ForEach-Object { Write-Host "  - $_" }
    exit 1
}
Write-Host "MSI install check passed at phase '$Phase'."
# Explicit, so a caller reading $LASTEXITCODE sees 0 rather than whatever the
# previous command left there.
exit 0
