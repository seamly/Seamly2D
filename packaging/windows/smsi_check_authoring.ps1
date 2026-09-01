#******************************************************************************
# **  @file   smsi_check_authoring.ps1
# **  @author slspencer
# **  @date   July 28, 2026
# **
# **  @brief
# **  Assert that a built Seamly2D MSI carries the install-time authoring the
# **  project expects (Task 51): elevation, the ARP entry, Start Menu and
# **  optional desktop shortcuts, file associations, the install-info registry
# **  rows, the previous-installation detection and its warning dialog.
# **
# **  This is the automated half of Task 51's verification. It reads the MSI
# **  database with the Windows Installer COM API, so it checks what the package
# **  actually contains rather than what the .wxs appears to say - it catches a
# **  WiX or WixUI change that silently drops a row. What it CANNOT check is
# **  runtime behaviour on a real machine (does the shortcut launch, does
# **  Explorer show the icon, does Apps & features list the product); that is
# **  the clean-machine checklist in README.md, and it still has to be walked
# **  through by a human.
# **
# **  Run automatically by smsi.ps1 after `wix msi validate`, so it also runs in
# **  CI for both architectures via ci.yml's windows-msi job.
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
    Check the install-time authoring of a built Seamly2D MSI (Task 51).

.DESCRIPTION
    Opens the MSI database read-only and asserts one expectation at a time,
    printing "ok" or "FAILED" per check and a summary at the end. Exits 1 if
    any check failed, so a caller (smsi.ps1, CI) fails the build.

.PARAMETER Msi
    Path of the .msi to inspect.

.PARAMETER Arch
    Architecture the package was built for: x64 (default) or arm64. Checked
    against the summary-information template.

.EXAMPLE
    .\smsi_check_authoring.ps1 -Msi packaging\windows\seamly-msi\x64\seamly-x64.msi
#>

param(
    [Parameter(Mandatory = $true)]
    [string]$Msi,

    [ValidateSet('x64', 'arm64')]
    [string]$Arch = 'x64'
)

$ErrorActionPreference = 'Stop'

# Every failed check is recorded here rather than thrown, so one run reports
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
# @brief  Run a SQL query against the open MSI database.
#
# The Windows Installer COM objects have no PowerShell-friendly methods, so
# every call goes through reflection (InvokeMember), which works the same in
# Windows PowerShell 5.1 and PowerShell 7.
#
# Rows come back as PSCustomObjects named after the selected columns, NOT as
# arrays of field values. That is deliberate: PowerShell unrolls an array that
# passes through a pipeline, so with arrays-as-rows a
# `(... | Where-Object { ... }).Count -eq 1` test counts the matched row's
# FIELDS rather than the rows, and quietly reports 2 for a single two-column
# match. Objects do not unroll, so the obvious test is also the correct one.
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
    # Leading comma: without it PowerShell unrolls the outer array on return, so
    # a no-row or single-row result would not come back as an array.
    #
    # ASSIGN THE RESULT DIRECTLY - do NOT write @(Get-MsiRows ...). The array
    # subexpression collects one stream item, and that item is already the row
    # array, so wrapping gives an array holding an array. A single-row query
    # still works, because PowerShell unwraps a one-element array on a cast or a
    # member access. A multi-row query then fails on the first cast instead.
    return , $rows
}

#------------------------------------------------------------------------------
# @brief  Read one property value from the Property table.
#
# @param  Name  property name
# @return the value, or an empty string when the property is absent
#------------------------------------------------------------------------------
function Get-MsiProperty {
    param([string]$Name)
    $rows = Get-MsiRows -Sql "SELECT ``Value`` FROM ``Property`` WHERE ``Property``='$Name'" -Columns 'Value'
    if ($rows.Count -eq 0) { return '' }
    return $rows[0].Value
}

# --- Open the database ---------------------------------------------------------
if (-not (Test-Path $Msi)) { throw "MSI not found: '$Msi'." }
$msiPath = (Resolve-Path $Msi).Path
Write-Host "checking install-time authoring of $msiPath"

$installer = New-Object -ComObject WindowsInstaller.Installer
# 0 = msiOpenDatabaseModeReadOnly.
$script:database = $installer.GetType().InvokeMember('OpenDatabase', 'InvokeMethod', $null, $installer, @($msiPath, 0))

# --- 1. elevation and platform -------------------------------------------------
# Word Count bit 3 (value 8) is msidbSumInfoSourceTypeLUAPackage: when SET the
# package declares that it does NOT need elevation. It must be clear here, or a
# user double-clicking the .msi gets a per-user install (or a bare failure)
# instead of one UAC prompt.
$summary = $installer.SummaryInformation($msiPath, 0)
$template = [string]$summary.Property(7)
$wordCount = [int]$summary.Property(15)
Assert-That -Name "package targets $Arch" -Succeeded ($template -like "$Arch;*") -Detail "template is '$template'"
Assert-That -Name 'package requires elevation (LUA bit clear)' -Succeeded (($wordCount -band 8) -eq 0) -Detail "word count $wordCount"
Assert-That -Name 'package installs per machine (ALLUSERS=1)' -Succeeded ((Get-MsiProperty -Name 'ALLUSERS') -eq '1')

# --- 2. Add/Remove Programs entry ---------------------------------------------
foreach ($arp in @('ARPPRODUCTICON', 'ARPHELPLINK', 'ARPURLINFOABOUT', 'ARPCOMMENTS')) {
    Assert-That -Name "$arp is set" -Succeeded ((Get-MsiProperty -Name $arp) -ne '')
}
# ARP shows the numeric MSI ProductVersion and that cannot be overridden, so the
# full YY.M.D.MMMM project version has to reach the user another way: the ARP
# comment, and the install-info registry key.
$displayVersion = @(Get-MsiRows -Sql "SELECT ``Value`` FROM ``Registry`` WHERE ``Name``='DisplayVersion'" -Columns 'Value')
Assert-That -Name 'full project version recorded in HKLM\SOFTWARE\Seamly\Seamly2D' `
    -Succeeded ($displayVersion.Count -eq 1 -and $displayVersion[0].Value -match '^\d{2}\.\d+\.\d+\.\d+$') `
    -Detail "found '$(if ($displayVersion.Count) { $displayVersion[0].Value } else { '<nothing>' })'"
Assert-That -Name 'ARPCOMMENTS carries the full project version' `
    -Succeeded ((Get-MsiProperty -Name 'ARPCOMMENTS') -match '\d{2}\.\d+\.\d+\.\d+')

# --- 3. upgrade behaviour ------------------------------------------------------
$upgrade = Get-MsiRows -Sql "SELECT ``UpgradeCode``, ``ActionProperty`` FROM ``Upgrade``" -Columns 'UpgradeCode', 'ActionProperty'
Assert-That -Name 'MajorUpgrade keyed on the fixed suite UpgradeCode' `
    -Succeeded (@($upgrade | Where-Object { $_.UpgradeCode -eq '{CBF4B5F1-C32C-4DBB-B385-3EE4A7B30658}' -and $_.ActionProperty -eq 'WIX_UPGRADE_DETECTED' }).Count -eq 1)

# --- 4. previous-installation detection ---------------------------------------
# The old NSIS installer is 32-bit and never switches the registry view, so both
# of its keys live in the WOW6432Node view. RegLocator Type bit 4 (value 16) is
# msidbLocatorType64bit: it must be CLEAR or an x64 package looks in the 64-bit
# view and never finds them.
$locators = Get-MsiRows -Sql "SELECT ``Signature_``, ``Root``, ``Key``, ``Name``, ``Type`` FROM ``RegLocator``" `
    -Columns 'Signature', 'Root', 'Key', 'Name', 'Type'
$uninstallLocator = @($locators | Where-Object { $_.Name -eq 'UninstallString' })
$installDirLocator = @($locators | Where-Object { $_.Signature -eq 'SeamlyLegacyInstallDirSearch' })
Assert-That -Name 'NSIS UninstallString is searched for under HKLM' `
    -Succeeded ($uninstallLocator.Count -eq 1 -and $uninstallLocator[0].Root -eq '2')
Assert-That -Name 'NSIS UninstallString search reads the 32-bit registry view' `
    -Succeeded ($uninstallLocator.Count -eq 1 -and (([int]$uninstallLocator[0].Type) -band 16) -eq 0) `
    -Detail 'RegLocator Type has the 64-bit flag set'
Assert-That -Name 'NSIS Install_Dir search reads the 32-bit registry view' `
    -Succeeded ($installDirLocator.Count -eq 1 -and (([int]$installDirLocator[0].Type) -band 16) -eq 0)

$appSearch = @(Get-MsiRows -Sql "SELECT ``Property`` FROM ``AppSearch``" -Columns 'Property' | ForEach-Object { $_.Property })
foreach ($searched in @('SEAMLYLEGACYUNINSTALLSTRING', 'SEAMLYLEGACYINSTALLDIR', 'SEAMLYOLDS2DEXE',
                        'SEAMLYOLDMEEXE', 'SEAMLYOLDLAYOUTEXE', 'SEAMLYNEWLAYOUTEXE')) {
    Assert-That -Name "$searched is filled in by AppSearch" -Succeeded ($appSearch -contains $searched)
}

# Public properties only survive the hand-off to the elevated server-side
# sequence when they are listed here.
$secure = Get-MsiProperty -Name 'SecureCustomProperties'
foreach ($property in @('SEAMLYDESKTOPSHORTCUTS', 'SEAMLYLEGACYUNINSTALLSTRING', 'SEAMLYLEGACYINSTALLDIR',
                        'SEAMLYOLDS2DEXE', 'SEAMLYOLDMEEXE', 'SEAMLYOLDLAYOUTEXE',
                        'SEAMLYNEWLAYOUTEXE', 'SEAMLYPREVIOUSDATAROOT')) {
    Assert-That -Name "$property is a secure custom property" -Succeeded ($secure -like "*$property*")
}

# --- 5. the wizard dialog chain (Task InstWinX64.1) ---------------------------
# The package defines its own dialog set, so it owns every page transition. Each
# arrow is a NewDialog row it authors itself.
#
#   WelcomeDlg -> LicenseAgreementDlg -> [SeamlyPreviousInstallDlg] ->
#   InstallDirDlg -> SeamlyDataDirDlg -> SeamlyDataMigrateDlg ->
#   SeamlyShortcutsDlg -> VerifyReadyDlg
#
# This replaced SpawnDialog wiring that WiX 6.0.2 never ran, so the three Seamly
# question pages were in the package and never displayed. A missing arrow leaves
# a page whose button does nothing - the same failure, silently - which is why
# both directions of every step are asserted here.
$script:controlEvents = Get-MsiRows `
    -Sql "SELECT ``Dialog_``, ``Control_``, ``Event``, ``Argument``, ``Condition``, ``Ordering`` FROM ``ControlEvent``" `
    -Columns 'Dialog', 'Control', 'Event', 'Argument', 'Condition', 'Ordering'

#------------------------------------------------------------------------------
# @brief  Assert one page-to-page transition of the wizard.
#
# @param  From            dialog the button is on
# @param  Control         button id, normally Next or Back
# @param  To              dialog the button opens
# @param  ConditionMatch  regex the row's condition must match, when it is
#                         conditional; omit for an unconditional transition
#------------------------------------------------------------------------------
function Assert-Transition {
    param(
        [string]$From,
        [string]$Control,
        [string]$To,
        [string]$ConditionMatch
    )
    $rows = @($script:controlEvents | Where-Object {
        $_.Dialog -eq $From -and $_.Control -eq $Control -and
        $_.Event -eq 'NewDialog' -and $_.Argument -eq $To })
    $succeeded = ($rows.Count -eq 1)
    if ($succeeded -and $ConditionMatch) { $succeeded = ($rows[0].Condition -match $ConditionMatch) }
    Assert-That -Name "$From's $Control opens $To" -Succeeded $succeeded `
        -Detail "$($rows.Count) row(s), condition '$(if ($rows.Count) { $rows[0].Condition } else { '<nothing>' })'"
}

Assert-Transition -From 'WelcomeDlg' -Control 'Next' -To 'LicenseAgreementDlg' -ConditionMatch 'NOT Installed'
$previousInstallCondition = 'SEAMLYOLDS2DEXE.*SEAMLYOLDMEEXE.*SEAMLYOLDLAYOUTEXE.*SEAMLYNEWLAYOUTEXE.*NOT Installed'
Assert-Transition -From 'LicenseAgreementDlg' -Control 'Next' -To 'SeamlyPreviousInstallDlg' `
    -ConditionMatch $previousInstallCondition
Assert-Transition -From 'LicenseAgreementDlg' -Control 'Next' -To 'InstallDirDlg' -ConditionMatch 'NOT \('
Assert-Transition -From 'SeamlyPreviousInstallDlg' -Control 'Next' -To 'InstallDirDlg'
Assert-Transition -From 'InstallDirDlg' -Control 'Next' -To 'SeamlyDataDirDlg'
Assert-Transition -From 'SeamlyDataDirDlg' -Control 'Next' -To 'SeamlyDataMigrateDlg' `
    -ConditionMatch $previousInstallCondition
Assert-Transition -From 'SeamlyDataDirDlg' -Control 'Next' -To 'SeamlyShortcutsDlg' -ConditionMatch 'NOT \('
Assert-Transition -From 'SeamlyDataMigrateDlg' -Control 'Next' -To 'SeamlyShortcutsDlg'
Assert-Transition -From 'SeamlyShortcutsDlg' -Control 'Next' -To 'VerifyReadyDlg'

Assert-Transition -From 'LicenseAgreementDlg' -Control 'Back' -To 'WelcomeDlg'
Assert-Transition -From 'SeamlyPreviousInstallDlg' -Control 'Back' -To 'LicenseAgreementDlg'
Assert-Transition -From 'InstallDirDlg' -Control 'Back' -To 'SeamlyPreviousInstallDlg' `
    -ConditionMatch $previousInstallCondition
Assert-Transition -From 'InstallDirDlg' -Control 'Back' -To 'LicenseAgreementDlg' -ConditionMatch 'NOT \('
Assert-Transition -From 'SeamlyDataDirDlg' -Control 'Back' -To 'InstallDirDlg'
Assert-Transition -From 'SeamlyDataMigrateDlg' -Control 'Back' -To 'SeamlyDataDirDlg'
Assert-Transition -From 'SeamlyShortcutsDlg' -Control 'Back' -To 'SeamlyDataMigrateDlg' `
    -ConditionMatch $previousInstallCondition
Assert-Transition -From 'SeamlyShortcutsDlg' -Control 'Back' -To 'SeamlyDataDirDlg' -ConditionMatch 'NOT \('
Assert-Transition -From 'VerifyReadyDlg' -Control 'Back' -To 'SeamlyShortcutsDlg' -ConditionMatch 'NOT Installed'
# InstWinX64.7.10. The maintenance page is ours, not the stock MaintenanceTypeDlg,
# because WiX cannot add a control to a dialog another fragment defines and the
# page has to name the installed version. Replacing a stock dialog means owning
# every row it used to bring, so assert the ones that fail silently.
Assert-That -Name 'the stock maintenance-type page is replaced, not reused' `
    -Succeeded ((Get-MsiRows -Sql "SELECT ``Dialog`` FROM ``Dialog`` WHERE ``Dialog``='MaintenanceTypeDlg'" `
        -Columns 'Dialog').Count -eq 0)
Assert-That -Name 'dialog SeamlyMaintenanceTypeDlg is present' `
    -Succeeded ((Get-MsiRows -Sql "SELECT ``Dialog`` FROM ``Dialog`` WHERE ``Dialog``='SeamlyMaintenanceTypeDlg'" `
        -Columns 'Dialog').Count -eq 1)
Assert-Transition -From 'MaintenanceWelcomeDlg' -Control 'Next' -To 'SeamlyMaintenanceTypeDlg'
Assert-Transition -From 'SeamlyMaintenanceTypeDlg' -Control 'Back' -To 'MaintenanceWelcomeDlg'
Assert-Transition -From 'SeamlyMaintenanceTypeDlg' -Control 'RepairButton' -To 'VerifyReadyDlg'
Assert-Transition -From 'SeamlyMaintenanceTypeDlg' -Control 'RemoveButton' -To 'VerifyReadyDlg'
Assert-Transition -From 'VerifyReadyDlg' -Control 'Back' -To 'SeamlyMaintenanceTypeDlg' `
    -ConditionMatch 'Installed AND NOT PATCH'
# THE silent failure. VerifyReadyDlg shows its Repair and Remove buttons on
# WixUI_InstallMode alone; the stock page set it and ours must too. Drop these
# two rows and the wizard reaches the ready page with no enabled action button,
# so Repair and Remove do nothing and report nothing.
foreach ($mode in @('Repair', 'Remove', 'Change')) {
    $setMode = @($script:controlEvents | Where-Object {
        $_.Dialog -eq 'SeamlyMaintenanceTypeDlg' -and $_.Control -eq "${mode}Button" -and
        $_.Event -eq '[WixUI_InstallMode]' -and $_.Argument -eq $mode })
    Assert-That -Name "the $($mode.ToLower()) button sets WixUI_InstallMode" -Succeeded ($setMode.Count -eq 1)
}
# The mode must be set before the page changes, or VerifyReadyDlg is created
# while the property still holds the previous answer.
foreach ($mode in @('Repair', 'Remove')) {
    $rows = @($script:controlEvents | Where-Object {
        $_.Dialog -eq 'SeamlyMaintenanceTypeDlg' -and $_.Control -eq "${mode}Button" })
    $setModeOrder = @($rows | Where-Object { $_.Event -eq '[WixUI_InstallMode]' })
    $newDialogOrder = @($rows | Where-Object { $_.Event -eq 'NewDialog' })
    Assert-That -Name "the $($mode.ToLower()) button sets the mode before it advances" `
        -Succeeded ($setModeOrder.Count -eq 1 -and $newDialogOrder.Count -eq 1 -and
                    [int]$setModeOrder[0].Ordering -lt [int]$newDialogOrder[0].Ordering)
}
# Change stays disabled: the package has one feature, so there is nothing to
# select. ARPNOMODIFY is what Apps and features reads too.
$maintenanceConditions = Get-MsiRows `
    -Sql "SELECT ``Control_``, ``Action``, ``Condition`` FROM ``ControlCondition`` WHERE ``Dialog_``='SeamlyMaintenanceTypeDlg'" `
    -Columns 'Control', 'Action', 'Condition'
Assert-That -Name 'the change button is disabled by ARPNOMODIFY' `
    -Succeeded (@($maintenanceConditions | Where-Object {
        $_.Control -eq 'ChangeButton' -and $_.Action -eq 'Disable' -and $_.Condition -match 'ARPNOMODIFY' }).Count -eq 1)
# The version note. Three lines share one slot and their conditions must stay
# disjoint, or two print on top of each other.
$maintenanceControls = Get-MsiRows `
    -Sql "SELECT ``Control``, ``X``, ``Y``, ``Text`` FROM ``Control`` WHERE ``Dialog_``='SeamlyMaintenanceTypeDlg'" `
    -Columns 'Control', 'X', 'Y', 'Text'
foreach ($line in @('SameVersionText', 'OtherVersionText', 'UnknownVersionText')) {
    $shown = @($maintenanceConditions | Where-Object { $_.Control -eq $line -and $_.Action -eq 'Show' })
    Assert-That -Name "$line is shown by condition" -Succeeded ($shown.Count -eq 1)
    Assert-That -Name "$line names a version" `
        -Succeeded (@($maintenanceControls | Where-Object {
            $_.Control -eq $line -and $_.Text -match '\d+\.\d+\.\d+\.\d+' }).Count -eq 1)
}
$sameVersion = @($maintenanceConditions | Where-Object { $_.Control -eq 'SameVersionText' })
Assert-That -Name 'the same-version line compares against the built version' `
    -Succeeded ($sameVersion.Count -eq 1 -and $sameVersion[0].Condition -match 'SEAMLYINSTALLEDVERSION = "\d+\.\d+\.\d+\.\d+"') `
    -Detail "condition '$(if ($sameVersion.Count) { $sameVersion[0].Condition } else { '<nothing>' })'"
$unknownVersion = @($maintenanceConditions | Where-Object { $_.Control -eq 'UnknownVersionText' })
Assert-That -Name 'a machine with no recorded version still gets a line' `
    -Succeeded ($unknownVersion.Count -eq 1 -and $unknownVersion[0].Condition -match 'NOT SEAMLYINSTALLEDVERSION')
# Read from the same HKLM value InstallInfoRegistry writes, 64-bit view, raw
# (type 2 + 16). Apps and features stores only the numeric MSI ProductVersion,
# which is not the version the apps show.
$versionSearch = Get-MsiRows `
    -Sql "SELECT ``Signature_``, ``Root``, ``Key``, ``Name``, ``Type`` FROM ``RegLocator`` WHERE ``Name``='DisplayVersion'" `
    -Columns 'Signature', 'Root', 'Key', 'Name', 'Type'
Assert-That -Name 'the installed version is read from the Seamly install key' `
    -Succeeded ($versionSearch.Count -eq 1 -and
                $versionSearch[0].Root -eq '2' -and
                $versionSearch[0].Key -eq 'SOFTWARE\Seamly\Seamly2D' -and
                $versionSearch[0].Type -eq '18')
Assert-That -Name 'AppSearch fills SEAMLYINSTALLEDVERSION' `
    -Succeeded ((Get-MsiRows -Sql "SELECT ``Property`` FROM ``AppSearch`` WHERE ``Property``='SEAMLYINSTALLEDVERSION'" `
        -Columns 'Property').Count -eq 1)

# The two License Next rows must not both be true and must not both be false,
# or the button either picks an undefined winner or does nothing at all. They
# come from one preprocessor variable, so the test is that the second is the
# negation of the first.
$licenseNext = @($script:controlEvents | Where-Object {
    $_.Dialog -eq 'LicenseAgreementDlg' -and $_.Control -eq 'Next' -and $_.Event -eq 'NewDialog' })
$toPrevious = @($licenseNext | Where-Object { $_.Argument -eq 'SeamlyPreviousInstallDlg' })
$toInstallDir = @($licenseNext | Where-Object { $_.Argument -eq 'InstallDirDlg' })
Assert-That -Name 'the license page has exactly two exits' -Succeeded ($licenseNext.Count -eq 2)
if ($toPrevious.Count -eq 1 -and $toInstallDir.Count -eq 1) {
    $found = [regex]::Escape('((SEAMLYOLDS2DEXE AND SEAMLYOLDMEEXE AND NOT SEAMLYOLDLAYOUTEXE) OR SEAMLYNEWLAYOUTEXE) AND NOT Installed')
    Assert-That -Name 'the previous-install page is skipped on a clean machine' `
        -Succeeded ($toPrevious[0].Condition -match $found -and
                    $toInstallDir[0].Condition -match "NOT \($found\)") `
        -Detail "conditions '$($toPrevious[0].Condition)' and '$($toInstallDir[0].Condition)'"
}

# The install directory must be committed before the next page reads it.
$installDirNext = @($script:controlEvents | Where-Object {
    $_.Dialog -eq 'InstallDirDlg' -and $_.Control -eq 'Next' })
$setTargetPath = @($installDirNext | Where-Object { $_.Event -eq 'SetTargetPath' })
$leaveInstallDir = @($installDirNext | Where-Object { $_.Event -eq 'NewDialog' })
Assert-That -Name 'the chosen program directory is committed before the wizard moves on' `
    -Succeeded ($setTargetPath.Count -eq 1 -and $leaveInstallDir.Count -eq 1 -and
                [int]$setTargetPath[0].Ordering -lt [int]$leaveInstallDir[0].Ordering) `
    -Detail "SetTargetPath at $(if ($setTargetPath.Count) { $setTargetPath[0].Ordering } else { '<nothing>' }), NewDialog at $(if ($leaveInstallDir.Count) { $leaveInstallDir[0].Ordering } else { '<nothing>' })"

# The three sequenced dialogs decide which page opens the wizard, and the first
# one whose condition holds wins. A resumed install must reach ResumeDlg, not
# the welcome page. WiX numbers them from the order of the DialogRef elements in
# smsi.wxs, so this asserts the resulting numbers.
$uiSequence = Get-MsiRows -Sql "SELECT ``Action``, ``Sequence``, ``Condition`` FROM ``InstallUISequence``" `
    -Columns 'Action', 'Sequence', 'Condition'
foreach ($entry in @(@('ResumeDlg', 1296), @('WelcomeDlg', 1297), @('MaintenanceWelcomeDlg', 1298))) {
    $row = @($uiSequence | Where-Object { $_.Action -eq $entry[0] })
    Assert-That -Name "$($entry[0]) is sequenced at $($entry[1])" `
        -Succeeded ($row.Count -eq 1 -and [int]$row[0].Sequence -eq $entry[1]) `
        -Detail "found $(if ($row.Count) { $row[0].Sequence } else { '<nothing>' })"
}
# The previous-install page is a page of the chain now, not a sequenced dialog.
Assert-That -Name 'the previous-installation page is not sequenced separately' `
    -Succeeded (@($uiSequence | Where-Object { $_.Action -eq 'SeamlyPreviousInstallDlg' }).Count -eq 0)

# --- 5a. the "existing installation" warning text -----------------------------

# The wording is load-bearing: Task 51 requires the dialog to tell the user what
# happens to their own work. It no longer names a fixed folder - Task
# InstWinX64.1.2 made the data root a choice made later in Setup, so the page
# points forward to that question instead of naming a path that may be wrong.
$warningText = Get-MsiRows -Sql "SELECT ``Control``, ``Text`` FROM ``Control`` WHERE ``Dialog_``='SeamlyPreviousInstallDlg'" `
    -Columns 'Control', 'Text'
$userDataText = @($warningText | Where-Object { $_.Control -eq 'UserDataText' })
Assert-That -Name 'the warning points at the user-data folder question' `
    -Succeeded ($userDataText.Count -eq 1 -and $userDataText[0].Text -match 'user data folder')
Assert-That -Name 'the warning promises no delete and no overwrite' `
    -Succeeded ($userDataText.Count -eq 1 -and $userDataText[0].Text -match 'never deletes or overwrites')
Assert-That -Name 'the warning states that user data is not removed' `
    -Succeeded ($userDataText.Count -eq 1 -and $userDataText[0].Text -match 'not touched')
$nsisText = @($warningText | Where-Object { $_.Control -eq 'LegacyInstallText' })
Assert-That -Name 'the warning says Setup removes the old NSIS installation' `
    -Succeeded ($nsisText.Count -eq 1 -and $nsisText[0].Text -match 'Setup will remove')
# The removal takes the whole directory, so the page has to warn about anything
# of the user's that happens to be sitting in it.
Assert-That -Name 'the warning tells the user to move their own files out of it first' `
    -Succeeded ($nsisText.Count -eq 1 -and $nsisText[0].Text -match 'move anything of your own out')
Assert-That -Name 'the warning names the directory it found' `
    -Succeeded ($nsisText.Count -eq 1 -and $nsisText[0].Text -match '\[SEAMLYLEGACYINSTALLDIR\]')

# --- 5b. removal of the old NSIS installation ----------------------------------
# Its own uninstall.exe is deliberately never invoked - see smsi.wxs.
# What must be present is the removal of the four things it created.
$removeComponents = Get-MsiRows -Sql "SELECT ``Component``, ``Condition``, ``Attributes`` FROM ``Component``" `
    -Columns 'Component', 'Condition', 'Attributes'
foreach ($component in @('RemoveLegacyProgramFiles', 'RemoveLegacyRegistryKeys')) {
    $row = @($removeComponents | Where-Object { $_.Component -eq $component })
    Assert-That -Name "$component exists and is conditional on finding the NSIS install" `
        -Succeeded ($row.Count -eq 1 -and $row[0].Condition -eq 'SEAMLYLEGACYINSTALLDIR')
}
# msidbComponentAttributes64bit = 256. The NSIS keys live under WOW6432Node
# because that installer was 32-bit and never switched view, so the component
# carrying the RemoveRegistryKey rows must NOT have the 64-bit bit set.
$registryRemoval = @($removeComponents | Where-Object { $_.Component -eq 'RemoveLegacyRegistryKeys' })
Assert-That -Name 'the NSIS registry keys are removed from the 32-bit view' `
    -Succeeded ($registryRemoval.Count -eq 1 -and (([int]$registryRemoval[0].Attributes -band 256) -eq 0)) `
    -Detail "Attributes = $($registryRemoval.Attributes)"

$removeRegistry = Get-MsiRows -Sql "SELECT ``Root``, ``Key`` FROM ``RemoveRegistry``" -Columns 'Root', 'Key'
foreach ($key in @('SOFTWARE\NSIS_Seamly2D', 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Seamly2D')) {
    Assert-That -Name "the NSIS key '$key' is removed on install" `
        -Succeeded (@($removeRegistry | Where-Object { $_.Root -eq '2' -and $_.Key -eq $key }).Count -eq 1)
}

# util:RemoveFolderEx writes its instructions into a table the Util extension
# owns, read at install time by its Wix4RemoveFoldersEx custom action. The name
# is namespaced to the extension's MAJOR version - "Wix4RemoveFolderEx" under
# WiX 6.0.2 - so if these two checks ever fail with an OpenView error rather
# than a missing row, look for a renamed table before suspecting the authoring.
$removeFolderEx = Get-MsiRows -Sql "SELECT ``Component_``, ``Property``, ``InstallMode`` FROM ``Wix4RemoveFolderEx``" `
    -Columns 'Component', 'Property', 'InstallMode'
foreach ($property in @('SEAMLYLEGACYINSTALLDIR', 'SEAMLYLEGACYSTARTMENU')) {
    # InstallMode 1 = remove on install, which is the point: the old product has
    # to be gone before this one takes over its shortcuts and associations.
    Assert-That -Name "'$property' is scheduled for recursive removal on install" `
        -Succeeded (@($removeFolderEx | Where-Object {
            $_.Property -eq $property -and $_.InstallMode -eq '1' -and
            $_.Component -eq 'RemoveLegacyProgramFiles' }).Count -eq 1)
}

# Per-user settings cleanup on a genuine uninstall (2026-08-24, Case 1b-i's
# DataRoot investigation). InstallMode 2 = remove on uninstall, the opposite of
# the legacy pair above - these must NOT fire on install, only when Seamly
# itself is being removed.
$perUserComponents = @{
    SEAMLYLOCALAPPDATA  = 'RemoveLocalAppDataSettings'
    SEAMLYROAMINGAPPDATA = 'RemoveRoamingAppDataSettings'
}
foreach ($property in $perUserComponents.Keys) {
    Assert-That -Name "'$property' is scheduled for recursive removal on uninstall" `
        -Succeeded (@($removeFolderEx | Where-Object {
            $_.Property -eq $property -and $_.InstallMode -eq '2' -and
            $_.Component -eq $perUserComponents[$property] }).Count -eq 1)
}
# NOT UPGRADINGPRODUCTCODE: RemoveExistingProducts also "uninstalls" the old
# ProductCode mid major-upgrade, and Windows Installer sets that property for
# exactly that case. Without the guard a version bump would wipe the settings
# the new version is about to read forward on its own first run.
$perUserConditions = Get-MsiRows `
    -Sql "SELECT ``Component``, ``Condition`` FROM ``Component`` WHERE ``Component``='RemoveLocalAppDataSettings' OR ``Component``='RemoveRoamingAppDataSettings'" `
    -Columns 'Component', 'Condition'
Assert-That -Name 'per-user settings removal is skipped during a major upgrade' `
    -Succeeded ($perUserConditions.Count -eq 2 -and
                @($perUserConditions | Where-Object { $_.Condition -match 'NOT UPGRADINGPRODUCTCODE' }).Count -eq 2)
# The data root itself must never be in this removal list - a real uninstall
# must not delete a user's patterns and measurements.
Assert-That -Name 'the user-data root is never scheduled for removal' `
    -Succeeded (@($removeFolderEx | Where-Object { $_.Property -eq 'SEAMLYDATAROOT' -or $_.Property -eq 'SEAMLYDATAPARENT' }).Count -eq 0)

# Wix4RemoveFoldersEx runs BEFORE CostInitialize, because the RemoveFile rows it
# adds must exist in time for costing. Any property it reads therefore has to be
# set before it. SEAMLYLEGACYSTARTMENU once ran After CostFinalize, which is
# later, so the action read an empty property and the legacy Start Menu folder
# survived. Nothing failed and nothing logged - the folder was simply still
# there. The suffix on the action name is the package architecture (_X64 /
# _A64), so match on the prefix.
$executeSequence = Get-MsiRows -Sql "SELECT ``Action``, ``Sequence`` FROM ``InstallExecuteSequence``" `
    -Columns 'Action', 'Sequence'
$removeFoldersExAction = @($executeSequence | Where-Object { $_.Action -like 'Wix4RemoveFoldersEx*' })
$setStartMenuAction = @($executeSequence | Where-Object { $_.Action -eq 'SetSEAMLYLEGACYSTARTMENU' })
Assert-That -Name 'the legacy Start Menu path is set before RemoveFolderEx reads it' `
    -Succeeded ($removeFoldersExAction.Count -eq 1 -and $setStartMenuAction.Count -eq 1 -and
                [int]$setStartMenuAction[0].Sequence -lt [int]$removeFoldersExAction[0].Sequence) `
    -Detail "SetSEAMLYLEGACYSTARTMENU at $(if ($setStartMenuAction.Count) { $setStartMenuAction[0].Sequence } else { '<nothing>' }), $(if ($removeFoldersExAction.Count) { "$($removeFoldersExAction[0].Action) at $($removeFoldersExAction[0].Sequence)" } else { 'RemoveFoldersEx <nothing>' })"
# A directory property is unresolved that early, so the value must come from the
# environment. [AppDataFolder] there expands to nothing.
# A SetProperty compiles to a type-51 custom action: Source is the property it
# sets, Target is the value. The CustomAction table has no Property or Value
# column - asking for one fails with an OpenView error, not an empty result.
$startMenuValue = Get-MsiRows -Sql "SELECT ``Action``, ``Source``, ``Target`` FROM ``CustomAction`` WHERE ``Action``='SetSEAMLYLEGACYSTARTMENU'" `
    -Columns 'Action', 'Source', 'Target'
Assert-That -Name 'the legacy Start Menu path expands an environment property' `
    -Succeeded ($startMenuValue.Count -eq 1 -and
                $startMenuValue[0].Source -eq 'SEAMLYLEGACYSTARTMENU' -and
                $startMenuValue[0].Target.StartsWith('[%APPDATA]')) `
    -Detail "value '$(if ($startMenuValue.Count) { $startMenuValue[0].Target } else { '<nothing>' })'"

$conditions = Get-MsiRows -Sql "SELECT ``Control_``, ``Action``, ``Condition`` FROM ``ControlCondition`` WHERE ``Dialog_``='SeamlyPreviousInstallDlg'" `
    -Columns 'Control', 'Action', 'Condition'
foreach ($control in @('UpgradeText', 'LegacyInstallText')) {
    Assert-That -Name "$control is shown and hidden by condition" `
        -Succeeded ((@($conditions | Where-Object { $_.Control -eq $control -and $_.Action -eq 'Show' }).Count -eq 1) -and
                    (@($conditions | Where-Object { $_.Control -eq $control -and $_.Action -eq 'Hide' }).Count -eq 1))
}

# --- 6. optional desktop shortcuts --------------------------------------------
Assert-That -Name 'desktop shortcuts default to on' -Succeeded ((Get-MsiProperty -Name 'SEAMLYDESKTOPSHORTCUTS') -eq '1')
$checkBoxes = Get-MsiRows -Sql "SELECT ``Property``, ``Value`` FROM ``CheckBox``" -Columns 'Property', 'Value'
Assert-That -Name 'the shortcuts checkbox sets SEAMLYDESKTOPSHORTCUTS' `
    -Succeeded (@($checkBoxes | Where-Object { $_.Property -eq 'SEAMLYDESKTOPSHORTCUTS' -and $_.Value -eq '1' }).Count -eq 1)

# Where the shortcuts page sits in the wizard is asserted in section 5.

$components = Get-MsiRows -Sql "SELECT ``Component``, ``Condition`` FROM ``Component``" -Columns 'Component', 'Condition'
# All three must be conditional, or unticking the checkbox leaves one behind.
foreach ($component in @('Seamly2DDesktopShortcutComponent', 'SeamlyMeDesktopShortcutComponent', 'SeamlyLayoutDesktopShortcutComponent')) {
    $row = @($components | Where-Object { $_.Component -eq $component })
    Assert-That -Name "$component is conditional on the checkbox" `
        -Succeeded ($row.Count -eq 1 -and $row[0].Condition -eq 'SEAMLYDESKTOPSHORTCUTS')
}

# --- 6b. install location ------------------------------------------------------
# The suite installs into ProgramFiles64Folder\SeamlyApps. Both halves are
# asserted because both have been wrong before in ways nothing else catches: the
# 32-bit tree would be wrong for an all-x64/arm64 package (only the OLD NSIS
# installer belongs there, being 32-bit), and the folder is named for the whole
# suite rather than for seamly2d alone.
$directories = Get-MsiRows -Sql "SELECT ``Directory``, ``Directory_Parent``, ``DefaultDir`` FROM ``Directory``" `
    -Columns 'Directory', 'Parent', 'DefaultDir'
$installFolder = @($directories | Where-Object { $_.Directory -eq 'INSTALLFOLDER' })
Assert-That -Name 'INSTALLFOLDER is defined exactly once' -Succeeded ($installFolder.Count -eq 1)
if ($installFolder.Count -eq 1) {
    # DefaultDir stores "short|long" when the name does not fit 8.3, and
    # "SeamlyApps" (10 characters) does not - so compare the long half.
    $longName = ($installFolder[0].DefaultDir -split '\|')[-1]
    Assert-That -Name 'the install folder is named SeamlyApps' `
        -Succeeded ($longName -eq 'SeamlyApps') -Detail "DefaultDir = '$($installFolder[0].DefaultDir)'"
    Assert-That -Name 'the install folder sits under ProgramFiles64Folder' `
        -Succeeded ($installFolder[0].Parent -eq 'ProgramFiles64Folder') `
        -Detail "parent = '$($installFolder[0].Parent)'"
}

# --- 7. shortcuts --------------------------------------------------------------
$shortcuts = Get-MsiRows -Sql "SELECT ``Shortcut``, ``Directory_``, ``Name``, ``Target``, ``Icon_``, ``WkDir`` FROM ``Shortcut``" `
    -Columns 'Shortcut', 'Directory', 'Name', 'Target', 'Icon', 'WorkingDirectory'
# The Name column holds a filename, so anything longer than 8.3 is stored as
# "shortname|long name" - "SeamlyLayout" is, "Seamly2D" and "SeamlyMe" are not.
# Compare against the long name only.
$shortcuts = $shortcuts | ForEach-Object {
    $_.Name = ($_.Name -split '\|')[-1]
    $_
}
# Every package carries all three apps: the two-app package built with
# smsi.ps1 -NoSeamlyLayout is gone, and both architectures ship SeamlyLayout.
$expectedStartMenu = @('Seamly2D', 'SeamlyMe', 'SeamlyLayout')
foreach ($name in $expectedStartMenu) {
    # Target of an advertised shortcut is the feature it belongs to, not a path.
    $row = @($shortcuts | Where-Object { $_.Directory -eq 'ProgramMenuFolder' -and $_.Name -eq $name })
    Assert-That -Name "Start Menu shortcut '$name' exists, is advertised and has an icon" `
        -Succeeded ($row.Count -eq 1 -and $row[0].Target -eq 'WixDefaultFeature' -and
                    $row[0].Icon -ne '' -and $row[0].WorkingDirectory -eq 'INSTALLFOLDER')
}
# All three, matching what the checkbox on SeamlyShortcutsDlg promises.
# SeamlyLayout opens standalone with no argument, so a desktop launch is a
# supported way to start it.
foreach ($name in @('Seamly2D', 'SeamlyMe', 'SeamlyLayout')) {
    $row = @($shortcuts | Where-Object { $_.Directory -eq 'DesktopFolder' -and $_.Name -eq $name })
    Assert-That -Name "desktop shortcut '$name' targets the installed executable" `
        -Succeeded ($row.Count -eq 1 -and $row[0].Target -like '`[INSTALLFOLDER`]*.exe' -and $row[0].Icon -ne '') `
        -Detail "target is '$(if ($row.Count) { $row[0].Target } else { '<nothing>' })'"
}

$icons = @(Get-MsiRows -Sql "SELECT ``Name`` FROM ``Icon``" -Columns 'Name' | ForEach-Object { $_.Name })
$expectedIcons = @('seamly2d.ico', 'seamlyme.ico', 'seamlylayout.ico')
foreach ($icon in $expectedIcons) {
    Assert-That -Name "icon '$icon' is packaged" -Succeeded ($icons -contains $icon)
}

# Every Icon Id must be distinct. Declaring two <Icon> elements with the same Id
# silently collapses them, so one app ends up wearing another's icon — and the
# app whose Id was overwritten has no identifier left for its shortcut to
# reference. That is exactly how SeamlyLayout's icon was authored as a second
# "seamlyme.ico" and broke the x64 link with WIX0094.
Assert-That -Name 'icon identifiers are unique' `
    -Succeeded ($icons.Count -eq (@($icons | Sort-Object -Unique).Count))

# Every shortcut that names an icon must name one that exists. The MSI linker
# catches a dangling reference in the authoring, but only for shortcuts it
# actually compiles - assert it on the built package so no arch-conditional
# branch can ship a shortcut pointing at a missing Icon row.
foreach ($shortcut in @($shortcuts | Where-Object { $_.Icon -ne '' -and $null -ne $_.Icon })) {
    Assert-That -Name "shortcut '$($shortcut.Shortcut)' references a packaged icon ('$($shortcut.Icon)')" `
        -Succeeded ($icons -contains $shortcut.Icon)
}

# --- 8. file associations and install breadcrumbs ------------------------------
$registry = Get-MsiRows -Sql "SELECT ``Root``, ``Key``, ``Name``, ``Value`` FROM ``Registry``" `
    -Columns 'Root', 'Key', 'Name', 'Value'
foreach ($association in @(
        @{ Extension = '.sm2d'; ProgId = 'Seamly2D.Pattern';                Exe = 'Seamly2DExe' },
        @{ Extension = '.smis'; ProgId = 'SeamlyMe.IndividualMeasurements'; Exe = 'SeamlyMeExe' },
        @{ Extension = '.smms'; ProgId = 'SeamlyMe.MultisizeMeasurements';  Exe = 'SeamlyMeExe' })) {
    Assert-That -Name "$($association.Extension) is registered to $($association.ProgId)" `
        -Succeeded (@($registry | Where-Object { $_.Key -eq $association.Extension -and $_.Value -eq $association.ProgId }).Count -eq 1)
    Assert-That -Name "$($association.ProgId) opens with the installed executable" `
        -Succeeded (@($registry | Where-Object { $_.Key -eq "$($association.ProgId)\shell\open\command" -and $_.Value -like "*`[#$($association.Exe)`]*" }).Count -eq 1)
    Assert-That -Name "$($association.ProgId) has an Explorer icon" `
        -Succeeded (@($registry | Where-Object { $_.Key -like "*$($association.ProgId)\DefaultIcon" }).Count -eq 1)
}
Assert-That -Name 'the install path is recorded in HKLM\SOFTWARE\Seamly\Seamly2D' `
    -Succeeded (@($registry | Where-Object { $_.Root -eq '2' -and $_.Key -eq 'SOFTWARE\Seamly\Seamly2D' -and $_.Name -eq 'InstallPath' }).Count -eq 1)
foreach ($app in @('SeamlyMe', 'SeamlyLayout')) {
    Assert-That -Name "the install breadcrumbs are also recorded in HKLM\SOFTWARE\Seamly\$app" `
        -Succeeded (@($registry | Where-Object {
            $_.Root -eq '2' -and $_.Key -eq "SOFTWARE\Seamly\$app" -and
            $_.Name -in @('InstallPath', 'DisplayVersion', 'DataRoot', 'DataParent') }).Count -eq 4)
}

# --- 9. program folder and user-data root (Task InstWinX64.1.1 / 1.2) ----------
# The program folder name is asserted here because three documents and the
# migration authoring all name it; renaming it silently would leave them out of
# step, which is exactly what happened in 546e9d5def.
$directories = Get-MsiRows -Sql "SELECT ``Directory``, ``Directory_Parent``, ``DefaultDir`` FROM ``Directory``" `
    -Columns 'Directory', 'Parent', 'DefaultDir'
$installFolder = @($directories | Where-Object { $_.Directory -eq 'INSTALLFOLDER' })
Assert-That -Name 'the program folder is SeamlyApps under the 64-bit Program Files' `
    -Succeeded ($installFolder.Count -eq 1 -and
                $installFolder[0].DefaultDir -match 'SeamlyApps' -and
                $installFolder[0].Parent -eq 'ProgramFiles64Folder') `
    -Detail "DefaultDir '$(if ($installFolder.Count) { $installFolder[0].DefaultDir } else { '<nothing>' })', parent '$(if ($installFolder.Count) { $installFolder[0].Parent } else { '<nothing>' })'"

# InstWinX64.1.1.3. A Launch condition, not a dialog check, because a silent
# install has no dialog to press - so this is the only place the rule holds for
# /qn as well as for the wizard.
$launchConditions = @(Get-MsiRows -Sql "SELECT ``Condition`` FROM ``LaunchCondition``" -Columns 'Condition' |
    ForEach-Object { $_.Condition })
$cloudCondition = @($launchConditions | Where-Object { $_ -match 'INSTALLFOLDER' -and $_ -match 'OneDrive' })
Assert-That -Name 'a cloud-synced program folder is rejected' -Succeeded ($cloudCondition.Count -eq 1)
foreach ($service in @('OneDrive', 'Dropbox', 'Google Drive', 'iCloud')) {
    Assert-That -Name "the cloud-folder check covers $service" `
        -Succeeded ($cloudCondition.Count -eq 1 -and $cloudCondition[0] -match [regex]::Escape($service))
}

# InstWinX64.1.2.1 - 1.2.3. The data root is a directory id so it can be browsed
# in the UI and set on the command line for an unattended install. The user
# picks the PARENT and Setup appends a fixed SeamlyData leaf, so choosing E:\
# yields E:\SeamlyData rather than E:\ — the same shape as SeamlyApps under
# ProgramFiles64Folder. Assert the composition, not just the presence: if the
# leaf is ever folded back into the parent, a user who picks a drive root gets
# their patterns loose in the root of that drive.
$dataRoot = @($directories | Where-Object { $_.Directory -eq 'SEAMLYDATAROOT' })
Assert-That -Name 'the user-data root is a settable directory' -Succeeded ($dataRoot.Count -eq 1)
Assert-That -Name 'the data root appends a fixed SeamlyData leaf to a user-chosen parent' `
    -Succeeded ($dataRoot.Count -eq 1 -and
                $dataRoot[0].DefaultDir -match 'SeamlyData' -and
                $dataRoot[0].Parent -eq 'SEAMLYDATAPARENT') `
    -Detail "DefaultDir '$(if ($dataRoot.Count) { $dataRoot[0].DefaultDir } else { '<nothing>' })', parent '$(if ($dataRoot.Count) { $dataRoot[0].Parent } else { '<nothing>' })'"
Assert-That -Name 'the data-root parent is itself replaceable' `
    -Succeeded (@($directories | Where-Object { $_.Directory -eq 'SEAMLYDATAPARENT' -and $_.Parent -eq 'TARGETDIR' }).Count -eq 1)
$dataRootComponent = Get-MsiRows `
    -Sql "SELECT ``Component``, ``Directory_``, ``Condition``, ``Attributes`` FROM ``Component`` WHERE ``Component``='CreateUserDataRoot'" `
    -Columns 'Component', 'Directory', 'Condition', 'Attributes'
Assert-That -Name 'Setup creates the selected user-data root' `
    -Succeeded ($dataRootComponent.Count -eq 1 -and
                $dataRootComponent[0].Directory -eq 'SEAMLYDATAROOT' -and
                $dataRootComponent[0].Condition -eq 'SEAMLYDATACHOSEN')
# msidbComponentAttributesPermanent = 16. User data must survive uninstall.
Assert-That -Name 'the user-data root component is permanent' `
    -Succeeded ($dataRootComponent.Count -eq 1 -and
                (([int]$dataRootComponent[0].Attributes -band 16) -eq 16)) `
    -Detail "Attributes = $(if ($dataRootComponent.Count) { $dataRootComponent[0].Attributes } else { '<nothing>' })"
$createdDataRoots = Get-MsiRows `
    -Sql "SELECT ``Directory_``, ``Component_`` FROM ``CreateFolder`` WHERE ``Component_``='CreateUserDataRoot'" `
    -Columns 'Directory', 'Component'
Assert-That -Name 'the folder component creates SEAMLYDATAROOT' `
    -Succeeded ($createdDataRoots.Count -eq 1 -and $createdDataRoots[0].Directory -eq 'SEAMLYDATAROOT')
foreach ($property in @('SEAMLYDATAROOT', 'SEAMLYDATAPARENT', 'SEAMLYCOPYUSERDATA')) {
    Assert-That -Name "$property is a secure custom property" -Succeeded ($secure -like "*$property*")
}
# The default is computed in BOTH sequences (2026-08-24): the UI sequence for
# an interactive install, and the execute sequence too, so a bare `/qn` install
# with no properties also gets <Documents>\SeamlyData instead of silently
# deferring to each app's own first-run default (Case 1b-i of
# TEST_INSTALLER_WIN_X64.md found the two disagreeing). Accepted tradeoff: a
# genuinely unattended SYSTEM-context deployment (SCCM/Intune, no logged-in
# user) has no user to impersonate, so PersonalFolder there resolves to
# SYSTEM's own profile - see the long comment above SEAMLYDATAPARENT in
# smsi.wxs. A real unattended deployment that cares should pass
# SEAMLYDATAPARENT or SEAMLYDATAROOT explicitly, which this never overrides.
$uiActions = @(Get-MsiRows -Sql "SELECT ``Action`` FROM ``InstallUISequence``" -Columns 'Action' |
    ForEach-Object { $_.Action })
$executeActions = @(Get-MsiRows -Sql "SELECT ``Action`` FROM ``InstallExecuteSequence``" -Columns 'Action' |
    ForEach-Object { $_.Action })
Assert-That -Name 'the data-root default is computed in the UI sequence' `
    -Succeeded ($uiActions -contains 'SetSEAMLYDATAPARENT')
Assert-That -Name 'the data-root default is ALSO computed in the elevated sequence, for a bare /qn install' `
    -Succeeded ($executeActions -contains 'SetSEAMLYDATAPARENTExecute' -and
                $executeActions -contains 'SetSEAMLYDATAPARENTExecuteFallback')
Assert-That -Name 'the chosen data root is recorded for the apps to read' `
    -Succeeded (@($registry | Where-Object { $_.Root -eq '2' -and $_.Key -eq 'SOFTWARE\Seamly\Seamly2D' -and $_.Name -eq 'DataRoot' }).Count -eq 1)
# InstWinX64.00. Three things went wrong together before this: the wizard
# offered C:\Users\<user>\SeamlyData, the apps created <Documents>\Seamly, and
# nothing read what the wizard recorded. Pin all three. (Task SettingsFiles.7
# later aligned the apps' own default to <Documents>\SeamlyData too.)
#
# The default parent is the Documents folder, because that is where users go to
# find the files other applications write. PersonalFolder is preferred over
# %USERPROFILE%\Documents so a redirected Documents - OneDrive Known Folder Move
# - is followed, which is what QStandardPaths::DocumentsLocation does app-side.
$dataParentActions = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Target`` FROM ``CustomAction`` WHERE ``Source``='SEAMLYDATAPARENT'" `
    -Columns 'Action', 'Target'
# Count 2, not 1: the UI and execute-sequence pairs (2026-08-24) share the same
# Source/Target - one PersonalFolder action and one %USERPROFILE% fallback in
# each sequence.
Assert-That -Name 'the data-root default prefers the Documents known folder' `
    -Succeeded (@($dataParentActions | Where-Object { $_.Target -eq '[PersonalFolder]' }).Count -eq 2)
Assert-That -Name 'the data-root default falls back to %USERPROFILE%\Documents' `
    -Succeeded (@($dataParentActions | Where-Object { $_.Target -eq '[%USERPROFILE]\Documents\' }).Count -eq 2) `
    -Detail 'an empty SEAMLYDATAPARENT aborts the wizard with error 2343'
# All four default-computing actions must stand down on a maintenance run.
# Without this a repair recomputes the default parent, SEAMLYDATACHOSEN
# follows, and a user who moved their data root loses it silently.
$uiDataParent = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Condition`` FROM ``InstallUISequence`` WHERE ``Action``='SetSEAMLYDATAPARENT' OR ``Action``='SetSEAMLYDATAPARENTFallback'" `
    -Columns 'Action', 'Condition'
Assert-That -Name 'both UI data-root defaults are skipped on a maintenance run' `
    -Succeeded ($uiDataParent.Count -eq 2 -and
                @($uiDataParent | Where-Object { $_.Condition -match 'NOT Installed' }).Count -eq 2)
$executeDataParent = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Condition`` FROM ``InstallExecuteSequence`` WHERE ``Action``='SetSEAMLYDATAPARENTExecute' OR ``Action``='SetSEAMLYDATAPARENTExecuteFallback'" `
    -Columns 'Action', 'Condition'
Assert-That -Name 'both execute-sequence data-root defaults are skipped on a maintenance run' `
    -Succeeded ($executeDataParent.Count -eq 2 -and
                @($executeDataParent | Where-Object { $_.Condition -match 'NOT Installed' }).Count -eq 2)
# What reaches the registry is SEAMLYDATAROOTRECORDED, never SEAMLYDATAROOT. A
# directory id always resolves, so [SEAMLYDATAROOT] in a silent install with no
# arguments composes onto TARGETDIR and records C:\SeamlyData - which every app
# would then adopt as the user's data root.
$dataRootValue = @($registry | Where-Object {
    $_.Root -eq '2' -and $_.Key -eq 'SOFTWARE\Seamly\Seamly2D' -and $_.Name -eq 'DataRoot' })
Assert-That -Name 'the recorded data root is the guarded property, not the raw directory' `
    -Succeeded ($dataRootValue.Count -eq 1 -and $dataRootValue[0].Value -eq '[SEAMLYDATAROOTRECORDED]') `
    -Detail "value '$(if ($dataRootValue.Count) { $dataRootValue[0].Value } else { '<nothing>' })'"
Assert-That -Name 'SEAMLYDATAROOTRECORDED is a secure custom property' `
    -Succeeded ($secure -like '*SEAMLYDATAROOTRECORDED*')
# Repair and maintenance keep the recorded value: AppSearch prefills it from the
# key the last install wrote, and nothing overwrites it unless this run chose a
# root. Type 18 is a raw registry value read from the 64-bit view (2 + 16).
$recordedSearch = Get-MsiRows `
    -Sql "SELECT ``Signature_``, ``Root``, ``Key``, ``Name``, ``Type`` FROM ``RegLocator`` WHERE ``Signature_``='RecordedDataRootSearch'" `
    -Columns 'Signature_', 'Root', 'Key', 'Name', 'Type'
Assert-That -Name 'the recorded data root is prefilled from the existing install' `
    -Succeeded ($recordedSearch.Count -eq 1 -and
                $recordedSearch[0].Root -eq '2' -and
                $recordedSearch[0].Key -eq 'SOFTWARE\Seamly\Seamly2D' -and
                $recordedSearch[0].Type -eq '18')
$recordedAppSearch = Get-MsiRows `
    -Sql "SELECT ``Property``, ``Signature_`` FROM ``AppSearch`` WHERE ``Property``='SEAMLYDATAROOTRECORDED'" `
    -Columns 'Property', 'Signature_'
Assert-That -Name 'AppSearch fills SEAMLYDATAROOTRECORDED' -Succeeded ($recordedAppSearch.Count -eq 1)
# SEAMLYDATAPARENTRECORDED protects HKLM\...\DataParent the same way
# SEAMLYDATAROOTRECORDED protects DataRoot above - SEAMLYDATAPARENT is also a
# directory id and always resolves once CostFinalize runs, garbage included.
$recordedParentSearch = Get-MsiRows `
    -Sql "SELECT ``Signature_``, ``Root``, ``Key``, ``Name``, ``Type`` FROM ``RegLocator`` WHERE ``Signature_``='RecordedDataParentRecordedSearch'" `
    -Columns 'Signature_', 'Root', 'Key', 'Name', 'Type'
Assert-That -Name 'the recorded data parent is prefilled from the existing install' `
    -Succeeded ($recordedParentSearch.Count -eq 1 -and
                $recordedParentSearch[0].Root -eq '2' -and
                $recordedParentSearch[0].Key -eq 'SOFTWARE\Seamly\Seamly2D' -and
                $recordedParentSearch[0].Type -eq '18')
$recordedParentAppSearch = Get-MsiRows `
    -Sql "SELECT ``Property``, ``Signature_`` FROM ``AppSearch`` WHERE ``Property``='SEAMLYDATAPARENTRECORDED'" `
    -Columns 'Property', 'Signature_'
Assert-That -Name 'AppSearch fills SEAMLYDATAPARENTRECORDED' -Succeeded ($recordedParentAppSearch.Count -eq 1)
Assert-That -Name 'SEAMLYDATAPARENTRECORDED is a secure custom property' `
    -Succeeded ($secure -like '*SEAMLYDATAPARENTRECORDED*')
$previousRootSearch = Get-MsiRows `
    -Sql "SELECT ``Property``, ``Signature_`` FROM ``AppSearch`` WHERE ``Property``='SEAMLYPREVIOUSDATAROOT'" `
    -Columns 'Property', 'Signature_'
Assert-That -Name 'AppSearch preserves the previous data root for relocation' `
    -Succeeded ($previousRootSearch.Count -eq 1)
$dataParentValue = @($registry | Where-Object {
    $_.Root -eq '2' -and $_.Key -eq 'SOFTWARE\Seamly\Seamly2D' -and $_.Name -eq 'DataParent' })
Assert-That -Name 'the recorded data parent is the guarded property, not the raw directory' `
    -Succeeded ($dataParentValue.Count -eq 1 -and $dataParentValue[0].Value -eq '[SEAMLYDATAPARENTRECORDED]') `
    -Detail "value '$(if ($dataParentValue.Count) { $dataParentValue[0].Value } else { '<nothing>' })'"
# InstWinX64.2.11. A major upgrade is a fresh install of a new ProductCode, so it
# re-asks every question - including the program directory. Without a prefill it
# offers the default, and somebody who installed to E:\Programs\SeamlyApps moves
# drive by pressing Next. Both halves of the prefill are asserted here: the
# program directory from InstallPath, the data root from the recorded DataParent.
#
# Type 18 is a raw registry value read from the 64-bit view (2 + 16).
$installPathSearch = Get-MsiRows `
    -Sql "SELECT ``Signature_``, ``Root``, ``Key``, ``Name``, ``Type`` FROM ``RegLocator`` WHERE ``Signature_``='RecordedInstallPathSearch'" `
    -Columns 'Signature', 'Root', 'Key', 'Name', 'Type'
Assert-That -Name 'the program directory is read back from the Seamly install key' `
    -Succeeded ($installPathSearch.Count -eq 1 -and
                $installPathSearch[0].Root -eq '2' -and
                $installPathSearch[0].Key -eq 'SOFTWARE\Seamly\Seamly2D' -and
                $installPathSearch[0].Name -eq 'InstallPath' -and
                $installPathSearch[0].Type -eq '18')
Assert-That -Name 'AppSearch prefills INSTALLFOLDER for an upgrade' `
    -Succeeded ((Get-MsiRows -Sql "SELECT ``Property`` FROM ``AppSearch`` WHERE ``Property``='INSTALLFOLDER'" `
        -Columns 'Property').Count -eq 1)
Assert-That -Name 'AppSearch prefills SEAMLYDATAPARENT for an upgrade' `
    -Succeeded ((Get-MsiRows -Sql "SELECT ``Property`` FROM ``AppSearch`` WHERE ``Property``='SEAMLYDATAPARENT'" `
        -Columns 'Property').Count -eq 1)
# The wizard sets INSTALLFOLDER client-side; a perMachine package runs its
# execute sequence elevated. A public property must be in SecureCustomProperties
# to cross that boundary, and INSTALLFOLDER was not listed before this task.
Assert-That -Name 'INSTALLFOLDER is a secure custom property' -Succeeded ($secure -like '*INSTALLFOLDER*')
# The prefill only wins because AppSearch is earlier than the directory
# resolution that would otherwise compose the authored default.
foreach ($sequence in @('InstallUISequence', 'InstallExecuteSequence')) {
    $rows = Get-MsiRows -Sql "SELECT ``Action``, ``Sequence`` FROM ``$sequence``" -Columns 'Action', 'Sequence'
    $appSearchAt = @($rows | Where-Object { $_.Action -eq 'AppSearch' })
    $costFinalizeAt = @($rows | Where-Object { $_.Action -eq 'CostFinalize' })
    Assert-That -Name "$sequence searches before it resolves directories" `
        -Succeeded ($appSearchAt.Count -eq 1 -and $costFinalizeAt.Count -eq 1 -and
                    [int]$appSearchAt[0].Sequence -lt [int]$costFinalizeAt[0].Sequence)
}
# The program folder must still be authored as a directory under the 64-bit
# Program Files. A Property row of the same name overrides the resolved path; it
# must not replace the Directory row, or a fresh machine gets no default at all.
Assert-That -Name 'the prefill did not replace the program directory row' `
    -Succeeded (@($directories | Where-Object {
        $_.Directory -eq 'INSTALLFOLDER' -and $_.Parent -eq 'ProgramFiles64Folder' }).Count -eq 1)
# Order is the whole mechanism. SEAMLYDATACHOSEN must be decided AFTER the
# execute-sequence defaults run (2026-08-24 - previously BEFORE CostInitialize,
# when only the wizard or the command line could have set SEAMLYDATAPARENT/
# SEAMLYDATAROOT), but still BEFORE CostFinalize resolves the Directory table -
# afterwards a directory id always resolves to something and the test cannot
# tell a real choice from a fallback. The recorded value itself must be
# composed AFTER CostFinalize, when [SEAMLYDATAROOT]/[SEAMLYDATAPARENT] are
# actually resolved.
$executeSequence = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Sequence`` FROM ``InstallExecuteSequence``" -Columns 'Action', 'Sequence'
function Get-SequenceNumber {
    param([string]$Action)
    $row = @($executeSequence | Where-Object { $_.Action -eq $Action })
    if ($row.Count -ne 1) { return -1 }
    return [int]$row[0].Sequence
}
$parentDefaultAt = Get-SequenceNumber -Action 'SetSEAMLYDATAPARENTExecuteFallback'
$chosenAt = Get-SequenceNumber -Action 'SetSEAMLYDATACHOSEN'
$recordedAt = Get-SequenceNumber -Action 'SetSEAMLYDATAROOTRECORDED'
$parentRecordedAt = Get-SequenceNumber -Action 'SetSEAMLYDATAPARENTRECORDED'
$costInitializeAt = Get-SequenceNumber -Action 'CostInitialize'
$costFinalizeAt = Get-SequenceNumber -Action 'CostFinalize'
$writeRegistryAt = Get-SequenceNumber -Action 'WriteRegistryValues'
Assert-That -Name 'the execute-sequence default runs after CostInitialize (needs PersonalFolder)' `
    -Succeeded ($parentDefaultAt -gt 0 -and $costInitializeAt -gt 0 -and $parentDefaultAt -gt $costInitializeAt) `
    -Detail "SetSEAMLYDATAPARENTExecuteFallback at $parentDefaultAt, CostInitialize at $costInitializeAt"
Assert-That -Name 'a chosen data root is detected after the execute-sequence default, before the directories resolve' `
    -Succeeded ($chosenAt -gt 0 -and $parentDefaultAt -gt 0 -and $costFinalizeAt -gt 0 -and
                $chosenAt -gt $parentDefaultAt -and $chosenAt -lt $costFinalizeAt) `
    -Detail "SetSEAMLYDATAPARENTExecuteFallback at $parentDefaultAt, SetSEAMLYDATACHOSEN at $chosenAt, CostFinalize at $costFinalizeAt"
Assert-That -Name 'the recorded data root is composed after the directories resolve' `
    -Succeeded ($recordedAt -gt 0 -and $costFinalizeAt -gt 0 -and $recordedAt -gt $costFinalizeAt) `
    -Detail "SetSEAMLYDATAROOTRECORDED at $recordedAt, CostFinalize at $costFinalizeAt"
Assert-That -Name 'the recorded data parent is composed after the directories resolve' `
    -Succeeded ($parentRecordedAt -gt 0 -and $costFinalizeAt -gt 0 -and $parentRecordedAt -gt $costFinalizeAt) `
    -Detail "SetSEAMLYDATAPARENTRECORDED at $parentRecordedAt, CostFinalize at $costFinalizeAt"
Assert-That -Name 'the recorded data root is composed before it is written' `
    -Succeeded ($recordedAt -gt 0 -and $writeRegistryAt -gt 0 -and $recordedAt -lt $writeRegistryAt)
Assert-That -Name 'the recorded data parent is composed before it is written' `
    -Succeeded ($parentRecordedAt -gt 0 -and $writeRegistryAt -gt 0 -and $parentRecordedAt -lt $writeRegistryAt)

$dialogs = @(Get-MsiRows -Sql "SELECT ``Dialog`` FROM ``Dialog``" -Columns 'Dialog' | ForEach-Object { $_.Dialog })
foreach ($dialog in @('SeamlyDataDirDlg', 'SeamlyDataMigrateDlg', 'SeamlyShortcutsDlg')) {
    Assert-That -Name "dialog '$dialog' is present" -Succeeded ($dialogs -contains $dialog)
}
# Where each question sits in the wizard is asserted in section 5.

# The data-root page browses with the shared BrowseDlg, which edits whatever
# _BrowseProperty names. Setting that property must come first, or Change
# browses the previous page's directory.
$changeFolder = @($script:controlEvents | Where-Object {
    $_.Dialog -eq 'SeamlyDataDirDlg' -and $_.Control -eq 'ChangeFolder' })
$browseProperty = @($changeFolder | Where-Object { $_.Event -eq '[_BrowseProperty]' -and $_.Argument -eq 'SEAMLYDATAPARENT' })
$browseSpawn = @($changeFolder | Where-Object { $_.Event -eq 'SpawnDialog' -and $_.Argument -eq 'BrowseDlg' })
Assert-That -Name 'the data-root page browses the data-root parent' `
    -Succeeded ($browseProperty.Count -eq 1 -and $browseSpawn.Count -eq 1 -and
                [int]$browseProperty[0].Ordering -lt [int]$browseSpawn[0].Ordering) `
    -Detail "_BrowseProperty at $(if ($browseProperty.Count) { $browseProperty[0].Ordering } else { '<nothing>' }), SpawnDialog at $(if ($browseSpawn.Count) { $browseSpawn[0].Ordering } else { '<nothing>' })"
# The path box must bind DIRECTLY. An indirect PathEdit reads its property to
# get the NAME of the property holding the path, so an indirect
# SEAMLYDATAPARENT asks for a property named "C:\Users\<user>\" and aborts the
# install with error 2343 as the page is created. Stock InstallDirDlg is
# indirect only because WIXUI_INSTALLDIR holds the string "INSTALLFOLDER".
$folderControl = @(Get-MsiRows `
    -Sql "SELECT ``Control``, ``Attributes``, ``Property`` FROM ``Control`` WHERE ``Dialog_``='SeamlyDataDirDlg' AND ``Control``='Folder'" `
    -Columns 'Control', 'Attributes', 'Property')
# msidbControlAttributesIndirect 8.
Assert-That -Name 'the data-root path box binds directly, not indirectly' `
    -Succeeded ($folderControl.Count -eq 1 -and
                $folderControl[0].Property -eq 'SEAMLYDATAPARENT' -and
                ([int]$folderControl[0].Attributes -band 8) -eq 0) `
    -Detail "property '$(if ($folderControl.Count) { $folderControl[0].Property } else { '<nothing>' })', attributes $(if ($folderControl.Count) { $folderControl[0].Attributes } else { '<nothing>' })"
# NoPrefix turns accelerator parsing off, so any '&' in a label prints as a
# literal character. The data-root label used to read "Put the &SeamlyData
# folder in:" on screen. msidbControlAttributesNoPrefix is 0x20000.
$labelControls = Get-MsiRows `
    -Sql "SELECT ``Control``, ``Attributes``, ``Text`` FROM ``Control`` WHERE ``Dialog_``='SeamlyDataDirDlg' AND ``Type``='Text'" `
    -Columns 'Control', 'Attributes', 'Text'
$literalAmpersands = @($labelControls | Where-Object {
    $controlAttributes = [int]$_.Attributes
    (($controlAttributes -band 131072) -ne 0) -and ($_.Text -match '&') })
Assert-That -Name 'no NoPrefix label prints a literal ampersand' `
    -Succeeded ($labelControls.Count -gt 0 -and $literalAmpersands.Count -eq 0) `
    -Detail "$($labelControls.Count) text control(s), offending: $(if ($literalAmpersands.Count) { ($literalAmpersands | ForEach-Object { $_.Control }) -join ', ' } else { 'none' })"
# A typed path reaches the Directory table only through SetTargetPath, and it
# has to happen before the next page reads [SEAMLYDATAROOT].
$dataDirNext = @($script:controlEvents | Where-Object {
    $_.Dialog -eq 'SeamlyDataDirDlg' -and $_.Control -eq 'Next' })
$dataDirCommit = @($dataDirNext | Where-Object { $_.Event -eq 'SetTargetPath' -and $_.Argument -eq 'SEAMLYDATAPARENT' })
$dataDirAdvance = @($dataDirNext | Where-Object { $_.Event -eq 'NewDialog' })
Assert-That -Name 'the data-root page commits the path before it advances' `
    -Succeeded ($dataDirCommit.Count -eq 1 -and $dataDirAdvance.Count -eq 2 -and
                @($dataDirAdvance | Where-Object {
                    [int]$dataDirCommit[0].Ordering -ge [int]$_.Ordering }).Count -eq 0) `
    -Detail "SetTargetPath at $(if ($dataDirCommit.Count) { $dataDirCommit[0].Ordering } else { '<nothing>' }), NewDialog rows at $(($dataDirAdvance | ForEach-Object { $_.Ordering }) -join ', ')"
# BrowseDlg's OK must close the dialog and commit the path it browsed to, and it
# must validate only the program directory: the data root is allowed on cloud
# and removable drives that the program-directory rules reject.
$browseOk = @($script:controlEvents | Where-Object { $_.Dialog -eq 'BrowseDlg' -and $_.Control -eq 'OK' })
Assert-That -Name 'browsing commits the folder it was given' `
    -Succeeded (@($browseOk | Where-Object { $_.Event -eq 'SetTargetPath' -and $_.Argument -eq '[_BrowseProperty]' }).Count -eq 1)
Assert-That -Name 'browsing validates the program directory only' `
    -Succeeded (@($browseOk | Where-Object { $_.Event -eq 'CheckTargetPath' -and $_.Condition -match 'INSTALLFOLDER' }).Count -eq 1)

# InstWinX64.1.2.4. The copy must be deferred (it needs the script on disk),
# impersonated (SYSTEM cannot read the user's own folders) and non-fatal (a
# file-copy problem must not roll back a good program install).
$copyAction = @(Get-MsiRows -Sql "SELECT ``Action``, ``Type`` FROM ``CustomAction`` WHERE ``Action``='SeamlyCopyUserData'" `
    -Columns 'Action', 'Type')
Assert-That -Name 'the user-data copy action exists' -Succeeded ($copyAction.Count -eq 1)
if ($copyAction.Count -eq 1) {
    $type = [int]$copyAction[0].Type
    # msidbCustomActionTypeInScript 1024, NoImpersonate 2048, ContinueOnError 64.
    Assert-That -Name 'the copy runs deferred' -Succeeded (($type -band 1024) -ne 0) -Detail "type $type"
    Assert-That -Name 'the copy runs as the user, not SYSTEM' -Succeeded (($type -band 2048) -eq 0) -Detail "type $type"
    Assert-That -Name 'a failed copy does not fail the install' -Succeeded (($type -band 64) -ne 0) -Detail "type $type"
}
# There is deliberately no rollback action: it could only "undo" the copy by
# deleting files out of a folder that may have held the user's work already.
$customActions = @(Get-MsiRows -Sql "SELECT ``Action`` FROM ``CustomAction``" -Columns 'Action' |
    ForEach-Object { $_.Action })
Assert-That -Name 'no rollback action deletes copied user data' `
    -Succeeded (-not ($customActions -contains 'SeamlyCopyUserDataRollback'))
Assert-That -Name 'the copy helper script is packaged' `
    -Succeeded (@(Get-MsiRows -Sql "SELECT ``FileName`` FROM ``File`` WHERE ``Component_``='UserDataCopyScript'" -Columns 'FileName' |
                  Where-Object { $_.FileName -match 'smsi_migrate_user_data\.ps1' }).Count -eq 1)
$migrationCommands = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Target`` FROM ``CustomAction`` WHERE ``Action``='SetSeamlyOldDataMigration' OR ``Action``='SetSeamlyNewDataMigration'" `
    -Columns 'Action', 'Target'
$oldMigrationCommand = @($migrationCommands | Where-Object { $_.Action -eq 'SetSeamlyOldDataMigration' })
$newMigrationCommand = @($migrationCommands | Where-Object { $_.Action -eq 'SetSeamlyNewDataMigration' })
Assert-That -Name 'old Seamly uses the archive migration mode' `
    -Succeeded ($oldMigrationCommand.Count -eq 1 -and $oldMigrationCommand[0].Target -match '-Mode Old')
Assert-That -Name 'new Seamly uses the relocation migration mode' `
    -Succeeded ($newMigrationCommand.Count -eq 1 -and
                $newMigrationCommand[0].Target -match '-Mode New' -and
                $newMigrationCommand[0].Target -match 'SEAMLYPREVIOUSDATAROOT')
$migrationConditions = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Condition`` FROM ``InstallExecuteSequence`` WHERE ``Action``='SetSeamlyOldDataMigration' OR ``Action``='SetSeamlyNewDataMigration'" `
    -Columns 'Action', 'Condition'
$oldMigrationCondition = @($migrationConditions | Where-Object { $_.Action -eq 'SetSeamlyOldDataMigration' })
$newMigrationCondition = @($migrationConditions | Where-Object { $_.Action -eq 'SetSeamlyNewDataMigration' })
Assert-That -Name 'old Seamly requires both parent apps and no SeamlyLayout' `
    -Succeeded ($oldMigrationCondition.Count -eq 1 -and
                $oldMigrationCondition[0].Condition -match 'SEAMLYOLDS2DEXE' -and
                $oldMigrationCondition[0].Condition -match 'SEAMLYOLDMEEXE' -and
                $oldMigrationCondition[0].Condition -match 'NOT SEAMLYOLDLAYOUTEXE')
Assert-That -Name 'new Seamly requires an existing SeamlyLayout executable' `
    -Succeeded ($newMigrationCondition.Count -eq 1 -and
                $newMigrationCondition[0].Condition -match 'SEAMLYNEWLAYOUTEXE')

# SettingsFiles.2. The seeding action mirrors the copy action's contract:
# deferred (needs the script on disk), impersonated (writes the user's own
# %LOCALAPPDATA%), non-fatal (the apps supply defaults at runtime anyway).
$seedAction = @(Get-MsiRows -Sql "SELECT ``Action``, ``Type`` FROM ``CustomAction`` WHERE ``Action``='SeamlySeedUserSettings'" `
    -Columns 'Action', 'Type')
Assert-That -Name 'the settings-seeding action exists' -Succeeded ($seedAction.Count -eq 1)
if ($seedAction.Count -eq 1) {
    $type = [int]$seedAction[0].Type
    # msidbCustomActionTypeInScript 1024, NoImpersonate 2048, ContinueOnError 64.
    Assert-That -Name 'the seeding runs deferred' -Succeeded (($type -band 1024) -ne 0) -Detail "type $type"
    Assert-That -Name 'the seeding runs as the user, not SYSTEM' -Succeeded (($type -band 2048) -eq 0) -Detail "type $type"
    Assert-That -Name 'a failed seeding does not fail the install' -Succeeded (($type -band 64) -ne 0) -Detail "type $type"
}
Assert-That -Name 'the seeding helper script is packaged' `
    -Succeeded (@(Get-MsiRows -Sql "SELECT ``FileName`` FROM ``File`` WHERE ``Component_``='UserSettingsSeedScript'" -Columns 'FileName' |
                  Where-Object { $_.FileName -match 'smsi_seed_user_settings\.ps1' }).Count -eq 1)
$seedCommand = @(Get-MsiRows -Sql "SELECT ``Action``, ``Target`` FROM ``CustomAction`` WHERE ``Action``='SetSeamlySeedUserSettings'" `
    -Columns 'Action', 'Target')
# SEAMLYDATAROOTRECORDED, not SEAMLYDATAROOT: a directory id always resolves,
# so only the recorded property proves this run actually chose a root.
Assert-That -Name 'the seeding command passes the recorded data root' `
    -Succeeded ($seedCommand.Count -eq 1 -and
                $seedCommand[0].Target -match 'SEAMLYDATAROOTRECORDED' -and
                $seedCommand[0].Target -match '-InstallFolder')
# Both path properties can resolve with a trailing backslash. Backslash-quote
# is an escaped quote to PowerShell's command-line parser, so each closing
# quote needs a space before it; the script trims the values.
Assert-That -Name 'the seeding command quotes its path arguments quote-safely' `
    -Succeeded ($seedCommand.Count -eq 1 -and
                $seedCommand[0].Target -match '-DataRoot "\[SEAMLYDATAROOTRECORDED\] "' -and
                $seedCommand[0].Target -match '-InstallFolder "\[INSTALLFOLDER\] "')
$seedSequence = @(Get-MsiRows -Sql "SELECT ``Action``, ``Condition`` FROM ``InstallExecuteSequence`` WHERE ``Action``='SeamlySeedUserSettings'" `
    -Columns 'Action', 'Condition')
Assert-That -Name 'the seeding runs on first install only, with a recorded root' `
    -Succeeded ($seedSequence.Count -eq 1 -and
                $seedSequence[0].Condition -match 'SEAMLYDATAROOTRECORDED' -and
                $seedSequence[0].Condition -match 'NOT Installed')

# --- report --------------------------------------------------------------------
[System.Runtime.InteropServices.Marshal]::ReleaseComObject($script:database) | Out-Null

Write-Host ''
if ($script:failures.Count -gt 0) {
    Write-Host "MSI authoring check FAILED - $($script:failures.Count) problem(s):"
    $script:failures | ForEach-Object { Write-Host "  - $_" }
    exit 1
}
Write-Host 'MSI authoring check passed.'
# Explicit, so a caller reading $LASTEXITCODE after `& smsi_check_authoring.ps1`
# sees 0 rather than whatever the previous command left there.
exit 0
