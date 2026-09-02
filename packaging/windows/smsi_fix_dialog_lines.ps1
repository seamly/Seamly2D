#******************************************************************************
# **  @file   smsi_fix_dialog_lines.ps1
# **  @author slspencer
# **  @date   September 2, 2026
# **
# **  @brief
# **  Trim installer dialog Line controls that end past the right edge of their
# **  dialog, so Windows Installer stops logging Error 2826 (task MSI1b.1).
# **
# **  WixUI authors every BannerLine and BottomLine three installer units wider
# **  than the 370-unit dialog that holds it. Windows Installer logs one
# **  Error 2826 per control as it builds each dialog, scaled to display pixels
# **  - 7 px per control on a 144 DPI screen. The rows come from
# **  WixToolset.UI.wixext, which the project cannot edit and cannot upgrade
# **  past WiX 6, so the correction is made on the built package instead.
# **
# **  Only Line controls are corrected. A Line carries no content, so shortening
# **  it to the dialog edge changes nothing a user sees. Any OTHER overflowing
# **  control is a real authoring mistake - shortening it would clip text or a
# **  button - so this script reports it and fails the build.
# **
# **  Run by smsi.ps1 after the wix build step and before `wix msi validate`, so
# **  the ICE pass and smsi_check_authoring.ps1 both see the corrected package.
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
    Trim overflowing dialog Line controls in a built MSI (task MSI1b.1).

.DESCRIPTION
    Opens the MSI database for update, finds every Control row whose X + Width
    passes the width of its Dialog, and sets a Line control's width to the
    space left in the dialog. Throws when a non-Line control overflows.

.PARAMETER Msi
    Path of the .msi to correct. Modified in place.

.EXAMPLE
    .\smsi_fix_dialog_lines.ps1 -Msi packaging\windows\seamly-msi\x64\seamly-x64.msi
#>

param(
    [Parameter(Mandatory = $true)]
    [string]$Msi
)

$ErrorActionPreference = 'Stop'

#------------------------------------------------------------------------------
# @brief  Run a SQL query against the open MSI database.
#
# The Windows Installer COM objects have no PowerShell-friendly methods, so
# every call goes through reflection (InvokeMember), which works the same in
# Windows PowerShell 5.1 and PowerShell 7. Rows come back as PSCustomObjects
# named after the selected columns, matching smsi_check_authoring.ps1.
#
# @param  Sql      MSI SQL; table and column names must be backtick-quoted
# @param  Columns  names to give the selected columns, in SELECT order
# @return array of PSCustomObjects, one per row (empty when nothing matches)
#------------------------------------------------------------------------------
function Get-MsiRows {
    param(
        [string]$Sql,
        [string[]]$Columns
    )
    $view = $script:database.GetType().InvokeMember('OpenView', 'InvokeMethod', $null, $script:database, @($Sql))
    $view.GetType().InvokeMember('Execute', 'InvokeMethod', $null, $view, $null) | Out-Null
    $rows = @()
    while ($true) {
        $record = $view.GetType().InvokeMember('Fetch', 'InvokeMethod', $null, $view, $null)
        if ($null -eq $record) { break }
        $fields = [ordered]@{}
        for ($column = 1; $column -le $Columns.Count; $column++) {
            $value = $record.GetType().InvokeMember('StringData', 'GetProperty', $null, $record, @($column))
            $fields[$Columns[$column - 1]] = [string]$value
        }
        $rows += [pscustomobject]$fields
    }
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($view) | Out-Null
    # Leading comma: without it PowerShell unrolls the outer array on return.
    return , $rows
}

#------------------------------------------------------------------------------
# @brief  Set the Width of one Control row.
#
# The values go in as query parameters rather than into the SQL text, because
# MSI SQL has no escape for a quote inside a literal.
#
# @param  Dialog   value of the Control table's Dialog_ column
# @param  Control  value of the Control table's Control column
# @param  Width    new width, in installer units
#------------------------------------------------------------------------------
function Set-ControlWidth {
    param(
        [string]$Dialog,
        [string]$Control,
        [int]$Width
    )
    $sql = 'UPDATE `Control` SET `Width`=? WHERE `Dialog_`=? AND `Control`=?'
    $view = $script:database.GetType().InvokeMember('OpenView', 'InvokeMethod', $null, $script:database, @($sql))
    $record = $script:installer.GetType().InvokeMember('CreateRecord', 'InvokeMethod', $null, $script:installer, @(3))
    $record.GetType().InvokeMember('IntegerData', 'SetProperty', $null, $record, @(1, $Width))
    $record.GetType().InvokeMember('StringData', 'SetProperty', $null, $record, @(2, $Dialog))
    $record.GetType().InvokeMember('StringData', 'SetProperty', $null, $record, @(3, $Control))
    $view.GetType().InvokeMember('Execute', 'InvokeMethod', $null, $view, @($record)) | Out-Null
    $view.GetType().InvokeMember('Close', 'InvokeMethod', $null, $view, $null) | Out-Null
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($record) | Out-Null
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($view) | Out-Null
}

if (-not (Test-Path $Msi)) { throw "MSI not found: '$Msi'." }
$msiPath = (Resolve-Path $Msi).Path
Write-Host "trimming overflowing dialog lines in $msiPath"

$script:installer = New-Object -ComObject WindowsInstaller.Installer
# 1 = msiOpenDatabaseModeTransact: changes are held until Commit.
$script:database = $script:installer.GetType().InvokeMember(
    'OpenDatabase', 'InvokeMethod', $null, $script:installer, @($msiPath, 1))

$dialogWidth = @{}
foreach ($row in (Get-MsiRows -Sql 'SELECT `Dialog`, `Width` FROM `Dialog`' -Columns 'Dialog', 'Width')) {
    $dialogWidth[$row.Dialog] = [int]$row.Width
}

$controls = Get-MsiRows -Sql 'SELECT `Dialog_`, `Control`, `Type`, `X`, `Width` FROM `Control`' `
    -Columns 'Dialog', 'Control', 'Type', 'X', 'Width'

$trimmed = 0
$refused = @()
foreach ($control in $controls) {
    $width = $dialogWidth[$control.Dialog]
    # A Control row can name a dialog this package does not define; skip it
    # rather than treat the missing width as zero and report every control.
    if ($null -eq $width) { continue }
    $overflow = ([int]$control.X + [int]$control.Width) - $width
    if ($overflow -le 0) { continue }
    if ($control.Type -ne 'Line') {
        $refused += "$($control.Dialog).$($control.Control) ($($control.Type)) by $overflow"
        continue
    }
    Set-ControlWidth -Dialog $control.Dialog -Control $control.Control -Width ($width - [int]$control.X)
    $trimmed++
}

if ($refused.Count -gt 0) {
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($script:database) | Out-Null
    throw ('These controls pass the right edge of their dialog and are not Line controls, ' +
           'so trimming them would clip what they show. Correct the authoring: ' + ($refused -join '; '))
}

if ($trimmed -gt 0) {
    $script:database.GetType().InvokeMember('Commit', 'InvokeMethod', $null, $script:database, $null) | Out-Null
}
[System.Runtime.InteropServices.Marshal]::ReleaseComObject($script:database) | Out-Null
[System.Runtime.InteropServices.Marshal]::ReleaseComObject($script:installer) | Out-Null
# The database holds the .msi open until its COM object is collected, and the
# caller validates the same file next.
[System.GC]::Collect()
[System.GC]::WaitForPendingFinalizers()

Write-Host "  trimmed $trimmed Line control(s) to the dialog edge."
exit 0
