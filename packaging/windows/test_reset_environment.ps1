#******************************************************************************
# **  @file   test_reset_environment.ps1
# **  @author slspencer
# **  @date   August 24, 2026
# **
# **  @brief
# **  Return this machine to Case 1 of TEST_INSTALLER_WIN_X64.md ("Not
# **  installed") before running an installer test case (1a/2a/3a/4a).
# **
# **  Removes everything the product MSI's own uninstall deliberately leaves
# **  behind, plus everything a fresh test run needs gone: every detected
# **  Seamly product, %PROGRAMDIR%, %DATAROOT% and its contents,
# **  %LOCALAPPDATA%\Seamly, %APPDATA%\Seamly, leftover
# **  %APPDATA%\Unknown Organization(.ini) from the empty-organization-name
# **  defect, the stray %LOCALAPPDATA%\SeamlyLayout tree left by builds
# **  before 26.9.2, and the Seamly registry keys under both HKLM and HKCU.
# **
# **  This is test-support only. It is deliberately more destructive than the
# **  shipped uninstall (packaging/windows/smsi_registry.wxs), which
# **  keeps %DATAROOT% on purpose - a real user's uninstall must never delete
# **  their patterns and measurements. This script exists precisely to undo
# **  that safety for a test machine between test-matrix runs.
# **
# **  @copyright
# **  This source code is part of the Seamly project, a suite of apparel CAD
# **  software.
# **  Copyright (C) 2026 Seamly2D Project
# **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
# **
# **  @licensing
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
    Wipe every trace of Seamly from this machine, for a clean installer test run.
.DESCRIPTION
    Reads HKLM\SOFTWARE\Seamly\Seamly2D\DataRoot before removing it, so a data
    root outside the two default names (SeamlyData, Seamly) is still found and
    removed. Requires elevation for the product uninstall and for HKLM.
.PARAMETER WhatIf
    List what would be removed without removing anything.
#>
[CmdletBinding(SupportsShouldProcess)]
param()

$ErrorActionPreference = 'Stop'

function Uninstall-SeamlyProducts
{
    $entries = Get-ItemProperty 'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*' -ErrorAction SilentlyContinue |
        Where-Object { $_.DisplayName -like '*Seamly*' -and $_.UninstallString -match 'MsiExec' }

    foreach ($entry in $entries)
    {
        if ($entry.UninstallString -notmatch '\{[0-9A-Fa-f-]{36}\}')
        {
            Write-Warning "Skipping '$($entry.DisplayName)': no ProductCode in UninstallString."
            continue
        }
        $productCode = $Matches[0]
        Write-Host "Uninstalling $($entry.DisplayName) $($entry.DisplayVersion) ($productCode)"
        if ($PSCmdlet.ShouldProcess($productCode, 'msiexec /x /quiet'))
        {
            $log = Join-Path $env:TEMP "seamly_reset_uninstall_$productCode.log"
            $proc = Start-Process msiexec.exe -ArgumentList '/x', $productCode, '/quiet', '/norestart', '/l*v', "`"$log`"" `
                -Verb RunAs -Wait -PassThru
            if ($proc.ExitCode -ne 0)
            {
                Write-Warning "msiexec /x $productCode exited $($proc.ExitCode). See $log."
            }
        }
    }
}

function Remove-PathIfPresent
{
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return }
    # A defective install can record a mangled path (for example one holding a
    # quote). Test-Path throws on such a value; skip it instead of aborting.
    if ($Path.IndexOfAny([System.IO.Path]::GetInvalidPathChars()) -ge 0)
    {
        Write-Warning "Skipping invalid recorded path: $Path"
        return
    }
    if (Test-Path -LiteralPath $Path)
    {
        Write-Host "Removing $Path"
        if ($PSCmdlet.ShouldProcess($Path, 'Remove-Item -Recurse -Force'))
        {
            Remove-Item -LiteralPath $Path -Recurse -Force -Confirm:$false
        }
    }
}

function Remove-RegistryKeyIfPresent
{
    param([string]$Path)
    if (Test-Path $Path)
    {
        Write-Host "Removing registry key $Path"
        if ($PSCmdlet.ShouldProcess($Path, 'Remove-Item -Recurse -Force'))
        {
            Remove-Item -Path $Path -Recurse -Force -Confirm:$false
        }
    }
}

# Read the recorded data root(s) BEFORE any registry removal, so a custom
# location (SEAMLYDATAROOT=E:\Patterns, or a location chosen in Preferences
# and adopted from a settings file) is still found even though it does not
# match either default name.
$recordedDataRoots = @()
$installKey = Get-ItemProperty 'HKLM:\SOFTWARE\Seamly\Seamly2D' -ErrorAction SilentlyContinue
if ($installKey -and $installKey.DataRoot)
{
    $recordedDataRoots += $installKey.DataRoot
}
# Task SettingsFiles.1 moved the shared common settings file to Local; the
# Roaming file may still exist on a machine the apps last ran on before the move.
$commonIniCandidates = @(
    (Join-Path $env:LOCALAPPDATA 'Seamly\qt6_common.ini'),
    (Join-Path $env:APPDATA 'Seamly\qt6_common.ini')
)
foreach ($commonIni in $commonIniCandidates)
{
    if (-not (Test-Path $commonIni)) { continue }
    $match = Select-String -Path $commonIni -Pattern '^dataRoot=(.+)$' | Select-Object -First 1
    if ($match)
    {
        $recordedDataRoots += $match.Matches[0].Groups[1].Value -replace '/', '\'
    }
}

Write-Host '=== 1. Uninstalling detected Seamly products ==='
Uninstall-SeamlyProducts

Write-Host '=== 2. Removing %PROGRAMDIR% ==='
Remove-PathIfPresent 'C:\Program Files\SeamlyApps'
Remove-PathIfPresent 'C:\Program Files (x86)\Seamly2D'

Write-Host '=== 3. Removing %DATAROOT% (default names and any recorded/configured location) ==='
$documents = [Environment]::GetFolderPath('MyDocuments')
Remove-PathIfPresent (Join-Path $documents 'SeamlyData')
Remove-PathIfPresent (Join-Path $documents 'Seamly')
foreach ($root in $recordedDataRoots | Select-Object -Unique)
{
    Remove-PathIfPresent $root
}

Write-Host '=== 4. Removing %LOCALAPPDATA%\Seamly and %APPDATA%\Seamly ==='
Remove-PathIfPresent (Join-Path $env:LOCALAPPDATA 'Seamly')
Remove-PathIfPresent (Join-Path $env:APPDATA 'Seamly')

Write-Host '=== 5. Removing leftover "Unknown Organization" artifacts (empty-organization-name defect) ==='
Remove-PathIfPresent (Join-Path $env:APPDATA 'Unknown Organization.ini')
Remove-PathIfPresent (Join-Path $env:APPDATA 'Unknown Organization')
# Layout.10: builds before 26.9.2 opened the log file before main() set the
# organization name, so SeamlyLayout wrote %LOCALAPPDATA%\SeamlyLayout\output.
# Section 4 does not reach it, because it sits outside the Seamly folder.
Remove-PathIfPresent (Join-Path $env:LOCALAPPDATA 'SeamlyLayout')

Write-Host '=== 6. Removing Seamly registry keys ==='
Remove-RegistryKeyIfPresent 'HKLM:\SOFTWARE\Seamly'
Remove-RegistryKeyIfPresent 'HKCU:\Software\Seamly'

Write-Host '=== Done. Verify with: ==='
Write-Host '  Test-Path "C:\Program Files\SeamlyApps"'
Write-Host '  Test-Path "HKLM:\SOFTWARE\Seamly"'
