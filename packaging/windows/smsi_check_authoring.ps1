#******************************************************************************
# **  @file   smsi_check_authoring.ps1
# **  @author slspencer
# **
# **  @brief
# **  Check that a built Seamly2D MSI includes the expected install-time
# **  authoring: elevation, ARP metadata, shortcuts, file
# **  associations, registry rows, and previous-install detection.
# **
# **  This script inspects the MSI database directly via the Windows Installer
# **  COM API, so it validates what the package actually contains. It does not
# **  verify runtime behavior on a real machine; that remains a manual checklist.
# **
# **  Run automatically by smsi.ps1 after `wix msi validate`, and in CI for
# **  both architectures.
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
    Check the install-time authoring of a built Seamly2D MSI.

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
# ProductVersion is the full YY.M.DDHH project version, so Apps and features
# shows the same string as Help > About. The ARP comment and the install-info
# registry key carry it too.
Assert-That -Name 'ProductVersion is the full project version' `
    -Succeeded ((Get-MsiProperty -Name 'ProductVersion') -match '^\d{2}\.\d+\.\d+$') `
    -Detail "found '$(Get-MsiProperty -Name 'ProductVersion')'"
$displayVersion = @(Get-MsiRows -Sql "SELECT ``Value`` FROM ``Registry`` WHERE ``Name``='DisplayVersion'" -Columns 'Value')
Assert-That -Name 'full project version recorded in HKLM\SOFTWARE\Seamly\Seamly2D' `
    -Succeeded ($displayVersion.Count -eq 1 -and $displayVersion[0].Value -match '^\d{2}\.\d+\.\d+$') `
    -Detail "found '$(if ($displayVersion.Count) { $displayVersion[0].Value } else { '<nothing>' })'"
Assert-That -Name 'ARPCOMMENTS carries the full project version' `
    -Succeeded ((Get-MsiProperty -Name 'ARPCOMMENTS') -match '\d{2}\.\d+\.\d+')

# --- 3. upgrade behaviour ------------------------------------------------------
$upgrade = Get-MsiRows -Sql "SELECT ``UpgradeCode``, ``ActionProperty`` FROM ``Upgrade``" -Columns 'UpgradeCode', 'ActionProperty'
Assert-That -Name 'MajorUpgrade keyed on the fixed suite UpgradeCode' `
    -Succeeded (@($upgrade | Where-Object { $_.UpgradeCode -eq '{CBF4B5F1-C32C-4DBB-B385-3EE4A7B30658}' -and $_.ActionProperty -eq 'WIX_UPGRADE_DETECTED' }).Count -eq 1)
# A newer installed version is detected by its own row, then the user chooses:
# SeamlyNewerVersionDlg offers uninstall-and-continue or cancel. Without the
# user's choice, SeamlyBlockDowngrade stops the install, silent ones included.
$newerRow = @($upgrade | Where-Object { $_.ActionProperty -eq 'SEAMLYNEWERINSTALLED' })
Assert-That -Name 'a newer installed version is detected' -Succeeded ($newerRow.Count -eq 1)
$uiRows = Get-MsiRows -Sql "SELECT ``Action``, ``Sequence``, ``Condition`` FROM ``InstallUISequence``" -Columns 'Action', 'Sequence', 'Condition'
$execRows = Get-MsiRows -Sql "SELECT ``Action``, ``Sequence``, ``Condition`` FROM ``InstallExecuteSequence``" -Columns 'Action', 'Sequence', 'Condition'
$newerDialog = @($uiRows | Where-Object { $_.Action -eq 'SeamlyNewerVersionDlg' })
$uiAppSearch = @($uiRows | Where-Object { $_.Action -eq 'AppSearch' })
$uiFindRelated = @($uiRows | Where-Object { $_.Action -eq 'FindRelatedProducts' })
Assert-That -Name 'the newer-version page is shown after AppSearch and FindRelatedProducts' `
    -Succeeded ($newerDialog.Count -eq 1 -and $uiAppSearch.Count -eq 1 -and $uiFindRelated.Count -eq 1 -and
                $newerDialog[0].Condition -match 'SEAMLYNEWERINSTALLED' -and
                [int]$newerDialog[0].Sequence -gt [int]$uiAppSearch[0].Sequence -and
                [int]$newerDialog[0].Sequence -gt [int]$uiFindRelated[0].Sequence) `
    -Detail "found $(if ($newerDialog.Count) { "$($newerDialog[0].Sequence) '$($newerDialog[0].Condition)'" } else { '<nothing>' })"
$newerOptions = Get-MsiRows -Sql "SELECT ``Value``, ``Text`` FROM ``RadioButton`` WHERE ``Property``='SeamlyNewerOption'" -Columns 'Value', 'Text'
Assert-That -Name 'the newer-version page offers uninstall and cancel' `
    -Succeeded ($newerOptions.Count -eq 2 -and
                @($newerOptions | Where-Object { $_.Value -eq 'Uninstall' }).Count -eq 1 -and
                @($newerOptions | Where-Object { $_.Value -eq 'Cancel' }).Count -eq 1)
$confirm = Get-MsiRows -Sql "SELECT ``Event``, ``Argument``, ``Condition`` FROM ``ControlEvent`` WHERE ``Dialog_``='SeamlyNewerVersionDlg' AND ``Control_``='OK'" -Columns 'Event', 'Argument', 'Condition'
Assert-That -Name 'OK with uninstall confirms the downgrade' `
    -Succeeded (@($confirm | Where-Object { $_.Event -eq '[SEAMLYDOWNGRADECONFIRMED]' -and $_.Argument -eq '1' -and $_.Condition -match 'Uninstall' }).Count -eq 1)
Assert-That -Name 'the downgrade confirmation reaches the execute sequence' `
    -Succeeded ((Get-MsiProperty -Name 'SecureCustomProperties') -match '(^|;)SEAMLYDOWNGRADECONFIRMED(;|$)')
$block = @($execRows | Where-Object { $_.Action -eq 'SeamlyBlockDowngrade' })
$execAppSearch = @($execRows | Where-Object { $_.Action -eq 'AppSearch' })
Assert-That -Name 'an unconfirmed downgrade is stopped after AppSearch' `
    -Succeeded ($block.Count -eq 1 -and $execAppSearch.Count -eq 1 -and
                $block[0].Condition -match 'SEAMLYNEWERINSTALLED' -and
                $block[0].Condition -match 'NOT SEAMLYDOWNGRADECONFIRMED' -and
                [int]$block[0].Sequence -gt [int]$execAppSearch[0].Sequence) `
    -Detail "found $(if ($block.Count) { "$($block[0].Sequence) '$($block[0].Condition)'" } else { '<nothing>' })"
$blockText = Get-MsiRows -Sql "SELECT ``Target``, ``Type`` FROM ``CustomAction`` WHERE ``Action``='SeamlyBlockDowngrade'" -Columns 'Target', 'Type'
Assert-That -Name 'the stop message names the installed and the new version' `
    -Succeeded ($blockText.Count -eq 1 -and
                $blockText[0].Target -match '\[SEAMLYINSTALLEDVERSION\]' -and
                $blockText[0].Target -match '\[ProductVersion\]')

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
                        'SEAMLYNEWLAYOUTEXE')) {
    Assert-That -Name "$property is a secure custom property" -Succeeded ($secure -like "*$property*")
}

# --- 5. the wizard dialog chain ---------------------------
# The package defines its own dialog set, so it owns every page transition. Each
# arrow is a NewDialog row it authors itself.
#
#   WelcomeDlg -> LicenseAgreementDlg -> [SeamlyPreviousInstallDlg] ->
#   SeamlyDataLocationDlg -> SeamlyShortcutsDlg -> VerifyReadyDlg
#
# SeamlyPreviousInstallDlg appears only when an earlier program install is
# found. SeamlyDataLocationDlg always appears: read-only when an earlier
# install already recorded a data root (SEAMLYDATAROOTRECORDED), editable
# with a Change button (BrowseDlg) otherwise, defaulting to
# [%USERPROFILE]\seamly2d. There is no program-directory page: INSTALLFOLDER
# is fixed and never asked about (see smsi.wxs).
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
Assert-Transition -From 'LicenseAgreementDlg' -Control 'Next' -To 'SeamlyDataLocationDlg' `
    -ConditionMatch 'NOT \('
Assert-Transition -From 'SeamlyPreviousInstallDlg' -Control 'Next' -To 'SeamlyDataLocationDlg'
Assert-Transition -From 'SeamlyDataLocationDlg' -Control 'Next' -To 'SeamlyShortcutsDlg'
Assert-Transition -From 'SeamlyShortcutsDlg' -Control 'Next' -To 'VerifyReadyDlg'

Assert-Transition -From 'LicenseAgreementDlg' -Control 'Back' -To 'WelcomeDlg'
Assert-Transition -From 'SeamlyPreviousInstallDlg' -Control 'Back' -To 'LicenseAgreementDlg'
Assert-Transition -From 'SeamlyDataLocationDlg' -Control 'Back' -To 'SeamlyPreviousInstallDlg' `
    -ConditionMatch $previousInstallCondition
Assert-Transition -From 'SeamlyDataLocationDlg' -Control 'Back' -To 'LicenseAgreementDlg' -ConditionMatch 'NOT \('
Assert-Transition -From 'SeamlyShortcutsDlg' -Control 'Back' -To 'SeamlyDataLocationDlg'
Assert-Transition -From 'VerifyReadyDlg' -Control 'Back' -To 'SeamlyShortcutsDlg' -ConditionMatch 'NOT Installed'
# The maintenance page is customized, not the stock MaintenanceTypeDlg,
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
            $_.Control -eq $line -and $_.Text -match '\d+\.\d+\.\d+' }).Count -eq 1)
}
$sameVersion = @($maintenanceConditions | Where-Object { $_.Control -eq 'SameVersionText' })
Assert-That -Name 'the same-version line compares against the built version' `
    -Succeeded ($sameVersion.Count -eq 1 -and $sameVersion[0].Condition -match 'SEAMLYINSTALLEDVERSION = "\d+\.\d+\.\d+"') `
    -Detail "condition '$(if ($sameVersion.Count) { $sameVersion[0].Condition } else { '<nothing>' })'"
$unknownVersion = @($maintenanceConditions | Where-Object { $_.Control -eq 'UnknownVersionText' })
Assert-That -Name 'a machine with no recorded version still gets a line' `
    -Succeeded ($unknownVersion.Count -eq 1 -and $unknownVersion[0].Condition -match 'NOT SEAMLYINSTALLEDVERSION')
# Read from the same HKLM value InstallInfoRegistry writes, 64-bit view, raw
# (type 2 + 16).
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

# The two License Next rows must be mutually exclusive and between them cover
# every case, or the button either picks an undefined winner or does nothing
# at all. SeamlyDataLocationDlg is the sole destination when the
# previous-install page is skipped - it shows its own read-only or editable
# variant depending on SEAMLYDATAROOTRECORDED (section 5's dialog-chain
# block), so the License page itself no longer needs to split on that.
$licenseNext = @($script:controlEvents | Where-Object {
    $_.Dialog -eq 'LicenseAgreementDlg' -and $_.Control -eq 'Next' -and $_.Event -eq 'NewDialog' })
$toPrevious = @($licenseNext | Where-Object { $_.Argument -eq 'SeamlyPreviousInstallDlg' })
$toDataLocation = @($licenseNext | Where-Object { $_.Argument -eq 'SeamlyDataLocationDlg' })
Assert-That -Name 'the license page has exactly two exits' -Succeeded ($licenseNext.Count -eq 2)
if ($toPrevious.Count -eq 1 -and $toDataLocation.Count -eq 1) {
    $found = [regex]::Escape('((SEAMLYOLDS2DEXE AND SEAMLYOLDMEEXE AND NOT SEAMLYOLDLAYOUTEXE) OR SEAMLYNEWLAYOUTEXE) AND NOT Installed')
    Assert-That -Name 'the previous-install page is skipped on a clean machine' `
        -Succeeded ($toPrevious[0].Condition -match $found -and
                    $toDataLocation[0].Condition -match "NOT \($found\)") `
        -Detail "conditions '$($toPrevious[0].Condition)', '$($toDataLocation[0].Condition)'"
}

# The program directory is fixed, not chosen, so nothing commits INSTALLFOLDER
# from a dialog. A SetProperty compiles to a type-51 custom action: Source is
# the property it sets, Target is the value (see the SEAMLYLEGACYSTARTMENU
# comment above). Two actions are expected - one per sequence, like the
# SEAMLYDATAROOT ui/execute pairs below - both pinning the same fixed value, so
# no AppSearch result or command-line value can survive.
$installFolderPin = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Target`` FROM ``CustomAction`` WHERE ``Source``='INSTALLFOLDER'" `
    -Columns 'Action', 'Target'
Assert-That -Name 'the program directory is pinned, not chosen' `
    -Succeeded ($installFolderPin.Count -eq 2 -and
                (@($installFolderPin | Where-Object { $_.Target -eq '[ProgramFiles64Folder]SeamlyApps' }).Count -eq 2)) `
    -Detail "$($installFolderPin.Count) action(s), target(s) '$(($installFolderPin | ForEach-Object { $_.Target }) -join ', ')'"
foreach ($pin in @(@('InstallUISequence', 'SetINSTALLFOLDER'), @('InstallExecuteSequence', 'SetINSTALLFOLDERExecute'))) {
    $sequenceRows = Get-MsiRows -Sql "SELECT ``Action``, ``Sequence``, ``Condition`` FROM ``$($pin[0])``" `
        -Columns 'Action', 'Sequence', 'Condition'
    $pinAt = @($sequenceRows | Where-Object { $_.Action -eq $pin[1] })
    $costFinalizeAt = @($sequenceRows | Where-Object { $_.Action -eq 'CostFinalize' })
    Assert-That -Name "$($pin[1]) pins INSTALLFOLDER unconditionally before CostFinalize in $($pin[0])" `
        -Succeeded ($pinAt.Count -eq 1 -and $costFinalizeAt.Count -eq 1 -and
                    -not $pinAt[0].Condition -and
                    [int]$pinAt[0].Sequence -lt [int]$costFinalizeAt[0].Sequence) `
        -Detail "$($pin[1]) at $(if ($pinAt.Count) { $pinAt[0].Sequence } else { '<nothing>' }) (condition '$(if ($pinAt.Count) { $pinAt[0].Condition } else { '' })'), CostFinalize at $(if ($costFinalizeAt.Count) { $costFinalizeAt[0].Sequence } else { '<nothing>' })"
}

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

# --- 5aa. the preparing page (SeamlyPrepareDlg replaces stock PrepareDlg) -----
# Stock PrepareDlg is Modeless="yes" with Next permanently disabled: it never
# blocks InstallUISequence, so AppSearch/CostFinalize run underneath it and
# WelcomeDlg's own baked-in Show row replaces it within a second - a flash, by
# design. SeamlyPrepareDlg fixes that by staying Modal (Attributes bit 2, the
# WiX default when Modeless is omitted): the sequence engine keeps control
# with it until Continue or Cancel actually fires. Losing that bit, or letting
# stock PrepareDlg's DialogRef sneak back in, silently reintroduces the flash -
# assert both so a rebuild is the only way to find out.
$dialogRows = Get-MsiRows -Sql "SELECT ``Dialog``, ``Attributes`` FROM ``Dialog``" -Columns 'Dialog', 'Attributes'
Assert-That -Name 'stock PrepareDlg is not present (SeamlyPrepareDlg replaces it)' `
    -Succeeded ((@($dialogRows | Where-Object { $_.Dialog -eq 'PrepareDlg' })).Count -eq 0)
$prepareDlg = @($dialogRows | Where-Object { $_.Dialog -eq 'SeamlyPrepareDlg' })
Assert-That -Name 'SeamlyPrepareDlg is present' -Succeeded ($prepareDlg.Count -eq 1)
if ($prepareDlg.Count -eq 1) {
    Assert-That -Name 'SeamlyPrepareDlg is Modal, not Modeless (blocks for a real click)' `
        -Succeeded (([int]$prepareDlg[0].Attributes -band 2) -ne 0) `
        -Detail "Attributes = $($prepareDlg[0].Attributes)"
}
$prepareSequence = @($uiSequence | Where-Object { $_.Action -eq 'SeamlyPrepareDlg' })
$appSearchSequence = @($uiSequence | Where-Object { $_.Action -eq 'AppSearch' })
Assert-That -Name 'SeamlyPrepareDlg is sequenced before AppSearch' `
    -Succeeded ($prepareSequence.Count -eq 1 -and $appSearchSequence.Count -eq 1 -and
                [int]$prepareSequence[0].Sequence -lt [int]$appSearchSequence[0].Sequence) `
    -Detail "SeamlyPrepareDlg at $(if ($prepareSequence.Count) { $prepareSequence[0].Sequence } else { '<nothing>' }), AppSearch at $(if ($appSearchSequence.Count) { $appSearchSequence[0].Sequence } else { '<nothing>' })"
# Both pages genuinely work (Continue ends this one, WelcomeDlg's own Next
# carries on), but identical wording between them is what read as one broken
# flash rather than two working pages - assert the titles stay distinct.
$titleControls = Get-MsiRows `
    -Sql "SELECT ``Dialog_``, ``Text`` FROM ``Control`` WHERE ``Control``='Title' AND (``Dialog_``='SeamlyPrepareDlg' OR ``Dialog_``='WelcomeDlg')" `
    -Columns 'Dialog', 'Text'
$prepareTitle = @($titleControls | Where-Object { $_.Dialog -eq 'SeamlyPrepareDlg' })
$welcomeTitle = @($titleControls | Where-Object { $_.Dialog -eq 'WelcomeDlg' })
Assert-That -Name "SeamlyPrepareDlg's title does not repeat WelcomeDlg's title" `
    -Succeeded ($prepareTitle.Count -eq 1 -and $welcomeTitle.Count -eq 1 -and $prepareTitle[0].Text -ne $welcomeTitle[0].Text) `
    -Detail "'$(if ($prepareTitle.Count) { $prepareTitle[0].Text } else { '<nothing>' })' vs '$(if ($welcomeTitle.Count) { $welcomeTitle[0].Text } else { '<nothing>' })'"
$prepareEvents = @($script:controlEvents | Where-Object { $_.Dialog -eq 'SeamlyPrepareDlg' })
Assert-That -Name 'SeamlyPrepareDlg Continue ends the dialog and lets the sequence carry on' `
    -Succeeded (@($prepareEvents | Where-Object { $_.Control -eq 'Continue' -and $_.Event -eq 'EndDialog' -and $_.Argument -eq 'Return' }).Count -eq 1)
Assert-That -Name 'SeamlyPrepareDlg Cancel spawns CancelDlg' `
    -Succeeded (@($prepareEvents | Where-Object { $_.Control -eq 'Cancel' -and $_.Event -eq 'SpawnDialog' -and $_.Argument -eq 'CancelDlg' }).Count -eq 1)

# --- 5a. the "existing installation" warning text -----------------------------

# The wording is load-bearing: dialog should tell the user what
# happens to their own work. It no longer names a fixed folder.
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
    -Succeeded (@($removeFolderEx | Where-Object { $_.Property -eq 'SEAMLYDATAROOT' }).Count -eq 0)

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
            $_.Name -in @('InstallPath', 'DisplayVersion', 'DataRoot') }).Count -eq 3)
}

# Every desktop-shortcut breadcrumb used to land in the
# Seamly2D key, so a reader could not tell which app a shortcut belonged to.
# Each one now goes under its own app key.
foreach ($app in @('Seamly2D', 'SeamlyMe', 'SeamlyLayout')) {
    Assert-That -Name "DesktopShortcut$app is recorded under HKLM\SOFTWARE\Seamly\$app" `
        -Succeeded (@($registry | Where-Object {
            $_.Root -eq '2' -and $_.Key -eq "SOFTWARE\Seamly\$app" -and
            $_.Name -eq "DesktopShortcut$app" }).Count -eq 1)
}

# --- 9. program folder and user-data root ----------
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


# The data root is a directory id so it can double as the property Setup
# resolves it through (smsi_files.wxs) - the same technique INSTALLFOLDER
# does not need but the old SEAMLYDATAPARENT used to. It sits directly under
# TARGETDIR: there is no longer a chosen parent to nest it under, since the
# root itself is now either the recorded value from an earlier install or the
# fixed [%USERPROFILE]\seamly2d default (smsi.wxs).
$dataRoot = @($directories | Where-Object { $_.Directory -eq 'SEAMLYDATAROOT' })
Assert-That -Name 'the user-data root is a settable directory' -Succeeded ($dataRoot.Count -eq 1)
Assert-That -Name 'the data root sits directly under TARGETDIR, with no chosen parent' `
    -Succeeded ($dataRoot.Count -eq 1 -and $dataRoot[0].Parent -eq 'TARGETDIR') `
    -Detail "parent '$(if ($dataRoot.Count) { $dataRoot[0].Parent } else { '<nothing>' })'"
$dataRootComponent = Get-MsiRows `
    -Sql "SELECT ``Component``, ``Directory_``, ``Condition``, ``Attributes`` FROM ``Component`` WHERE ``Component``='CreateUserDataRoot'" `
    -Columns 'Component', 'Directory', 'Condition', 'Attributes'
Assert-That -Name 'Setup creates the resolved user-data root' `
    -Succeeded ($dataRootComponent.Count -eq 1 -and
                $dataRootComponent[0].Directory -eq 'SEAMLYDATAROOT' -and
                $dataRootComponent[0].Condition -eq 'SEAMLYDATAROOT')
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
Assert-That -Name 'SEAMLYDATAROOT is a secure custom property' -Succeeded ($secure -like '*SEAMLYDATAROOT*')
# SEAMLYDATAROOT is resolved one of two ways, in both UI and execute
# sequences so a bare /qn install also gets a value: copied from
# SEAMLYDATAROOTRECORDED when an earlier install recorded one, otherwise
# defaulted to [%USERPROFILE]\seamly2d on a fresh install only. Unlike the old
# SEAMLYDATAPARENT default, this needs no known-folder API and no fallback -
# %USERPROFILE% is an environment property, always available.
$uiActions = @(Get-MsiRows -Sql "SELECT ``Action`` FROM ``InstallUISequence``" -Columns 'Action' |
    ForEach-Object { $_.Action })
$executeActions = @(Get-MsiRows -Sql "SELECT ``Action`` FROM ``InstallExecuteSequence``" -Columns 'Action' |
    ForEach-Object { $_.Action })
Assert-That -Name 'the recorded root is copied over in the UI sequence' `
    -Succeeded ($uiActions -contains 'SetSEAMLYDATAROOTRecorded')
Assert-That -Name 'the recorded root is ALSO copied over in the elevated sequence, for a bare /qn install' `
    -Succeeded ($executeActions -contains 'SetSEAMLYDATAROOTRecordedExecute')
Assert-That -Name 'the fresh-install default is computed in the UI sequence' `
    -Succeeded ($uiActions -contains 'SetSEAMLYDATAROOTDefault')
Assert-That -Name 'the fresh-install default is ALSO computed in the elevated sequence, for a bare /qn install' `
    -Succeeded ($executeActions -contains 'SetSEAMLYDATAROOTDefaultExecute')
Assert-That -Name 'the data root is recorded for the apps to read' `
    -Succeeded (@($registry | Where-Object { $_.Root -eq '2' -and $_.Key -eq 'SOFTWARE\Seamly\Seamly2D' -and $_.Name -eq 'DataRoot' }).Count -eq 1)
# The two SetProperty pairs that resolve SEAMLYDATAROOT share Source
# (the property id being set); Target tells them apart. Count 2, not 1: the
# UI and execute-sequence pair share the same Source/Target.
$dataRootActions = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Target`` FROM ``CustomAction`` WHERE ``Source``='SEAMLYDATAROOT'" `
    -Columns 'Action', 'Target'
Assert-That -Name 'the recorded root is copied verbatim from SEAMLYDATAROOTRECORDED' `
    -Succeeded (@($dataRootActions | Where-Object { $_.Target -eq '[SEAMLYDATAROOTRECORDED]' }).Count -eq 2)
Assert-That -Name 'the fresh-install default is %USERPROFILE%\seamly2d' `
    -Succeeded (@($dataRootActions | Where-Object { $_.Target -eq '[%USERPROFILE]\seamly2d' }).Count -eq 2)
# The fresh-install default must stand down on a maintenance run - without
# NOT Installed a repair would recompute it and a user who moved their data
# root would lose it silently. The recorded-root copy carries no such guard:
# a repair has to be able to fill in anything missing too (B1-B3/C1-C2 in
# TODO_INSTALLER.md).
$uiDataRootDefault = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Condition`` FROM ``InstallUISequence`` WHERE ``Action``='SetSEAMLYDATAROOTDefault'" `
    -Columns 'Action', 'Condition'
Assert-That -Name 'the UI fresh-install default is skipped on a maintenance run' `
    -Succeeded ($uiDataRootDefault.Count -eq 1 -and $uiDataRootDefault[0].Condition -match 'NOT Installed')
$executeDataRootDefault = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Condition`` FROM ``InstallExecuteSequence`` WHERE ``Action``='SetSEAMLYDATAROOTDefaultExecute'" `
    -Columns 'Action', 'Condition'
Assert-That -Name 'the execute-sequence fresh-install default is skipped on a maintenance run' `
    -Succeeded ($executeDataRootDefault.Count -eq 1 -and $executeDataRootDefault[0].Condition -match 'NOT Installed')
$uiDataRootRecorded = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Condition`` FROM ``InstallUISequence`` WHERE ``Action``='SetSEAMLYDATAROOTRecorded'" `
    -Columns 'Action', 'Condition'
Assert-That -Name 'the recorded-root copy is NOT skipped on a repair' `
    -Succeeded ($uiDataRootRecorded.Count -eq 1 -and $uiDataRootRecorded[0].Condition -notmatch 'NOT Installed') `
    -Detail "condition '$(if ($uiDataRootRecorded.Count) { $uiDataRootRecorded[0].Condition } else { '<nothing>' })'"
# SEAMLYDATAROOT is now the property everything else reads directly - it is
# always resolved by one of the two SetProperty pairs above before
# CostFinalize, so writing it straight to the registry is safe. SEAMLYDATAROOT
# no longer needs a separate guarded copy the way the old raw SEAMLYDATAPARENT
# directory-property did.
$dataRootValue = @($registry | Where-Object {
    $_.Root -eq '2' -and $_.Key -eq 'SOFTWARE\Seamly\Seamly2D' -and $_.Name -eq 'DataRoot' })
Assert-That -Name 'the recorded data root reads the resolved SEAMLYDATAROOT property' `
    -Succeeded ($dataRootValue.Count -eq 1 -and $dataRootValue[0].Value -eq '[SEAMLYDATAROOT]') `
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
# Program Files, matching the fixed value the SetProperty pins pass above.
Assert-That -Name 'the program directory is still authored under the 64-bit Program Files' `
    -Succeeded (@($directories | Where-Object {
        $_.Directory -eq 'INSTALLFOLDER' -and $_.Parent -eq 'ProgramFiles64Folder' }).Count -eq 1)
# Sequence matters: both SEAMLYDATAROOT SetProperty pairs run before
# CostFinalize, when the directory that shares its id resolves - and
# SeamlyEnsureUserData (below) has to run after WriteRegistryValues, so the
# recorded root it reads is already correct.
$executeSequence = Get-MsiRows `
    -Sql "SELECT ``Action``, ``Sequence`` FROM ``InstallExecuteSequence``" -Columns 'Action', 'Sequence'
function Get-SequenceNumber {
    param([string]$Action)
    $row = @($executeSequence | Where-Object { $_.Action -eq $Action })
    if ($row.Count -ne 1) { return -1 }
    return [int]$row[0].Sequence
}
$recordedCopyAt = Get-SequenceNumber -Action 'SetSEAMLYDATAROOTRecordedExecute'
$defaultAt = Get-SequenceNumber -Action 'SetSEAMLYDATAROOTDefaultExecute'
$costFinalizeAt = Get-SequenceNumber -Action 'CostFinalize'
$writeRegistryAt = Get-SequenceNumber -Action 'WriteRegistryValues'
Assert-That -Name 'the recorded-root copy runs before the directories resolve' `
    -Succeeded ($recordedCopyAt -gt 0 -and $costFinalizeAt -gt 0 -and $recordedCopyAt -lt $costFinalizeAt) `
    -Detail "SetSEAMLYDATAROOTRecordedExecute at $recordedCopyAt, CostFinalize at $costFinalizeAt"
Assert-That -Name 'the fresh-install default runs before the directories resolve' `
    -Succeeded ($defaultAt -gt 0 -and $costFinalizeAt -gt 0 -and $defaultAt -lt $costFinalizeAt) `
    -Detail "SetSEAMLYDATAROOTDefaultExecute at $defaultAt, CostFinalize at $costFinalizeAt"

$dialogs = @(Get-MsiRows -Sql "SELECT ``Dialog`` FROM ``Dialog``" -Columns 'Dialog' | ForEach-Object { $_.Dialog })
foreach ($dialog in @('SeamlyDataLocationDlg', 'SeamlyShortcutsDlg')) {
    Assert-That -Name "dialog '$dialog' is present" -Succeeded ($dialogs -contains $dialog)
}
Assert-That -Name 'the old data-parent and migration pages are gone' `
    -Succeeded (($dialogs -notcontains 'SeamlyDataDirDlg') -and ($dialogs -notcontains 'SeamlyDataMigrateDlg'))
Assert-That -Name 'BrowseDlg is part of the package, for the Change button' `
    -Succeeded ((Get-MsiRows -Sql "SELECT ``Dialog`` FROM ``Dialog`` WHERE ``Dialog``='BrowseDlg'" -Columns 'Dialog').Count -eq 1)
# Where the page sits in the wizard is asserted in section 5.

# The page has two mutually-exclusive variants, split on SEAMLYDATAROOTRECORDED:
# read-only (an earlier install already recorded a root - Setup never
# relocates existing data, B2 in TODO_INSTALLER.md) or editable with a Change
# button that spawns BrowseDlg (a true fresh install).
$folderControl = @(Get-MsiRows `
    -Sql "SELECT ``Control``, ``Type``, ``Text`` FROM ``Control`` WHERE ``Dialog_``='SeamlyDataLocationDlg' AND ``Control``='Folder'" `
    -Columns 'Control', 'Type', 'Text')
Assert-That -Name 'the data-location page shows SEAMLYDATAROOT as plain read-only text' `
    -Succeeded ($folderControl.Count -eq 1 -and $folderControl[0].Type -eq 'Text' -and
                $folderControl[0].Text -eq '[SEAMLYDATAROOT]') `
    -Detail "type '$(if ($folderControl.Count) { $folderControl[0].Type } else { '<nothing>' })', text '$(if ($folderControl.Count) { $folderControl[0].Text } else { '<nothing>' })'"
$folderEditControl = @(Get-MsiRows `
    -Sql "SELECT ``Control``, ``Type``, ``Property`` FROM ``Control`` WHERE ``Dialog_``='SeamlyDataLocationDlg' AND ``Control``='FolderEdit'" `
    -Columns 'Control', 'Type', 'Property')
Assert-That -Name 'the data-location page has an editable path box bound to SEAMLYDATAROOT' `
    -Succeeded ($folderEditControl.Count -eq 1 -and $folderEditControl[0].Type -eq 'PathEdit' -and
                $folderEditControl[0].Property -eq 'SEAMLYDATAROOT') `
    -Detail "type '$(if ($folderEditControl.Count) { $folderEditControl[0].Type } else { '<nothing>' })', property '$(if ($folderEditControl.Count) { $folderEditControl[0].Property } else { '<nothing>' })'"
Assert-That -Name 'the data-location page has a Change button' `
    -Succeeded ((Get-MsiRows -Sql "SELECT ``Control`` FROM ``Control`` WHERE ``Dialog_``='SeamlyDataLocationDlg' AND ``Control``='ChangeFolder'" `
        -Columns 'Control').Count -eq 1)
$browseSpawn = @($script:controlEvents | Where-Object {
    $_.Dialog -eq 'SeamlyDataLocationDlg' -and $_.Control -eq 'ChangeFolder' -and
    $_.Event -eq 'SpawnDialog' -and $_.Argument -eq 'BrowseDlg' })
Assert-That -Name 'the Change button spawns BrowseDlg' -Succeeded ($browseSpawn.Count -eq 1)
$browseProperty = @($script:controlEvents | Where-Object {
    $_.Dialog -eq 'SeamlyDataLocationDlg' -and $_.Control -eq 'ChangeFolder' -and
    $_.Event -eq '[_BrowseProperty]' -and $_.Argument -eq 'SEAMLYDATAROOT' })
Assert-That -Name 'the Change button points BrowseDlg at SEAMLYDATAROOT' -Succeeded ($browseProperty.Count -eq 1)
# BrowseDlg ships with Cancel wired but OK wired to nothing (WixToolset.UI.wixext
# 6.0.2) - without this row, OK just sits there and the folder is never confirmed.
$browseOk = @($script:controlEvents | Where-Object {
    $_.Dialog -eq 'BrowseDlg' -and $_.Control -eq 'OK' -and
    $_.Event -eq 'EndDialog' -and $_.Argument -eq 'Return' })
Assert-That -Name 'BrowseDlg''s OK button closes the dialog' -Succeeded ($browseOk.Count -eq 1)
# Its Next button reads "Continue" - there is nothing left to confirm after
# it, unlike the stock wizard's "Next".
$dataLocationNext = @(Get-MsiRows `
    -Sql "SELECT ``Control``, ``Text`` FROM ``Control`` WHERE ``Dialog_``='SeamlyDataLocationDlg' AND ``Control``='Next'" `
    -Columns 'Control', 'Text')
Assert-That -Name 'the data-location page''s Next button reads Continue' `
    -Succeeded ($dataLocationNext.Count -eq 1 -and $dataLocationNext[0].Text -match 'Continue') `
    -Detail "text '$(if ($dataLocationNext.Count) { $dataLocationNext[0].Text } else { '<nothing>' })'"
# NoPrefix turns accelerator parsing off, so any '&' in a label prints as a
# literal character. msidbControlAttributesNoPrefix is 0x20000.
$labelControls = Get-MsiRows `
    -Sql "SELECT ``Control``, ``Attributes``, ``Text`` FROM ``Control`` WHERE ``Dialog_``='SeamlyDataLocationDlg' AND ``Type``='Text'" `
    -Columns 'Control', 'Attributes', 'Text'
$literalAmpersands = @($labelControls | Where-Object {
    $controlAttributes = [int]$_.Attributes
    (($controlAttributes -band 131072) -ne 0) -and ($_.Text -match '&') })
Assert-That -Name 'no NoPrefix label prints a literal ampersand' `
    -Succeeded ($labelControls.Count -gt 0 -and $literalAmpersands.Count -eq 0) `
    -Detail "$($labelControls.Count) text control(s), offending: $(if ($literalAmpersands.Count) { ($literalAmpersands | ForEach-Object { $_.Control }) -join ', ' } else { 'none' })"

# One action now, not a copy/seed pair: SeamlyEnsureUserData creates the
# standard data subfolders and seeds the per-user settings ini files, so no
# app needs a manual Preferences > Paths visit. It must be deferred (it needs
# the script on disk), impersonated (SYSTEM cannot read the user's own
# folders and %LOCALAPPDATA%) and non-fatal (the apps fill missing defaults
# at runtime anyway).
$ensureAction = @(Get-MsiRows -Sql "SELECT ``Action``, ``Type`` FROM ``CustomAction`` WHERE ``Action``='SeamlyEnsureUserData'" `
    -Columns 'Action', 'Type')
Assert-That -Name 'the user-data ensure action exists' -Succeeded ($ensureAction.Count -eq 1)
if ($ensureAction.Count -eq 1) {
    $type = [int]$ensureAction[0].Type
    # msidbCustomActionTypeInScript 1024, NoImpersonate 2048, ContinueOnError 64.
    Assert-That -Name 'ensuring user data runs deferred' -Succeeded (($type -band 1024) -ne 0) -Detail "type $type"
    Assert-That -Name 'ensuring user data runs as the user, not SYSTEM' -Succeeded (($type -band 2048) -eq 0) -Detail "type $type"
    Assert-That -Name 'a failed ensure does not fail the install' -Succeeded (($type -band 64) -ne 0) -Detail "type $type"
}
# There is deliberately no rollback action: it could only "undo" by deleting
# files out of a folder that may already hold the user's own work.
$customActions = @(Get-MsiRows -Sql "SELECT ``Action`` FROM ``CustomAction``" -Columns 'Action' |
    ForEach-Object { $_.Action })
Assert-That -Name 'the old copy and seed actions are gone' `
    -Succeeded (-not ($customActions -contains 'SeamlyCopyUserData') -and
                -not ($customActions -contains 'SeamlySeedUserSettings'))
Assert-That -Name 'no rollback action deletes user data' `
    -Succeeded (-not ($customActions -contains 'SeamlyEnsureUserDataRollback'))
Assert-That -Name 'the ensure-user-data helper script is packaged' `
    -Succeeded (@(Get-MsiRows -Sql "SELECT ``FileName`` FROM ``File`` WHERE ``Component_``='UserDataEnsureScript'" -Columns 'FileName' |
                  Where-Object { $_.FileName -match 'smsi_ensure_user_data\.ps1' }).Count -eq 1)
$ensureCommand = @(Get-MsiRows -Sql "SELECT ``Action``, ``Target`` FROM ``CustomAction`` WHERE ``Action``='SetSeamlyEnsureUserData'" `
    -Columns 'Action', 'Target')
Assert-That -Name 'the ensure command passes the resolved data root and install folder' `
    -Succeeded ($ensureCommand.Count -eq 1 -and
                $ensureCommand[0].Target -match '-DataRoot' -and
                $ensureCommand[0].Target -match '-InstallFolder')
# Both path properties can resolve with a trailing backslash. Backslash-quote
# is an escaped quote to PowerShell's command-line parser, so each closing
# quote needs a space before it; the script trims the values.
Assert-That -Name 'the ensure command quotes its path arguments quote-safely' `
    -Succeeded ($ensureCommand.Count -eq 1 -and
                $ensureCommand[0].Target -match '-DataRoot "\[SEAMLYDATAROOT\] "' -and
                $ensureCommand[0].Target -match '-InstallFolder "\[INSTALLFOLDER\] "')
$ensureSequence = @(Get-MsiRows -Sql "SELECT ``Action``, ``Condition``, ``Sequence`` FROM ``InstallExecuteSequence`` WHERE ``Action``='SeamlyEnsureUserData'" `
    -Columns 'Action', 'Condition', 'Sequence')
Assert-That -Name 'ensuring user data runs whenever a data root is resolved' `
    -Succeeded ($ensureSequence.Count -eq 1 -and $ensureSequence[0].Condition -eq 'SEAMLYDATAROOT')
# Unlike the old copy/seed actions, this one must NOT be guarded by
# NOT Installed: a repair has to be able to fill in anything missing too.
Assert-That -Name 'ensuring user data is NOT skipped on a repair' `
    -Succeeded ($ensureSequence.Count -eq 1 -and $ensureSequence[0].Condition -notmatch 'NOT Installed') `
    -Detail "condition '$(if ($ensureSequence.Count) { $ensureSequence[0].Condition } else { '<nothing>' })'"
Assert-That -Name 'ensuring user data runs after the recorded root is written to the registry' `
    -Succeeded ($ensureSequence.Count -eq 1 -and $writeRegistryAt -gt 0 -and
                [int]$ensureSequence[0].Sequence -gt $writeRegistryAt) `
    -Detail "SeamlyEnsureUserData at $(if ($ensureSequence.Count) { $ensureSequence[0].Sequence } else { '<nothing>' }), WriteRegistryValues at $writeRegistryAt"

# --- 10. dialog control geometry -------------------------------
# Check that no control extends beyond its dialog, which causes Windows Installer
# Error 2826. WixUI is fixed by smsi_fix_dialog_lines.ps1; this catches any new
# overflowing controls.
$dialogSize = @{}
foreach ($row in (Get-MsiRows -Sql "SELECT ``Dialog``, ``Width``, ``Height`` FROM ``Dialog``" `
                              -Columns 'Dialog', 'Width', 'Height')) {
    $dialogSize[$row.Dialog] = @{ Width = [int]$row.Width; Height = [int]$row.Height }
}
$geometry = Get-MsiRows -Sql "SELECT ``Dialog_``, ``Control``, ``X``, ``Y``, ``Width``, ``Height`` FROM ``Control``" `
    -Columns 'Dialog', 'Control', 'X', 'Y', 'Width', 'Height'
# A Control row can name a dialog this package does not define. Skip it rather
# than read a missing size as zero and report every control on it.
$overflowing = @($geometry |
    Where-Object { $dialogSize.ContainsKey($_.Dialog) } |
    Where-Object {
        ([int]$_.X + [int]$_.Width)  -gt $dialogSize[$_.Dialog].Width -or
        ([int]$_.Y + [int]$_.Height) -gt $dialogSize[$_.Dialog].Height
    } |
    ForEach-Object { "$($_.Dialog).$($_.Control)" })
Assert-That -Name 'every control fits inside its dialog (no Error 2826)' `
    -Succeeded ($overflowing.Count -eq 0) `
    -Detail "$($overflowing.Count) overflow: $($overflowing -join ', ')"

# --- 11. launch Seamly2D from the Finish page ----------------
# The stock ExitDialog carries an OptionalCheckBox control already; only these
# two properties are ours. Checked by default and offered only on a fresh
# install, matching the checkbox's own ShowCondition.
Assert-That -Name 'the launch checkbox text names Seamly2D' `
    -Succeeded ((Get-MsiProperty -Name 'WIXUI_EXITDIALOGOPTIONALCHECKBOXTEXT') -match 'Seamly2D')
Assert-That -Name 'the launch checkbox defaults to checked' `
    -Succeeded ((Get-MsiProperty -Name 'WIXUI_EXITDIALOGOPTIONALCHECKBOX') -eq '1')

# BinaryData custom action (msidbCustomActionTypeBinaryData = 1): Source names
# the Binary row, Target names the DLL entry point it calls.
$launchAction = @(Get-MsiRows -Sql "SELECT ``Action``, ``Type``, ``Source``, ``Target`` FROM ``CustomAction`` WHERE ``Action``='LaunchApplication'" `
    -Columns 'Action', 'Type', 'Source', 'Target')
Assert-That -Name 'the LaunchApplication action exists and calls WixShellExec' `
    -Succeeded ($launchAction.Count -eq 1 -and
                (([int]$launchAction[0].Type) -band 1) -eq 1 -and
                $launchAction[0].Source -eq 'Wix4UtilCA_X64' -and
                $launchAction[0].Target -eq 'WixShellExec') `
    -Detail "$(if ($launchAction.Count) { "type $($launchAction[0].Type), source '$($launchAction[0].Source)', target '$($launchAction[0].Target)'" } else { '<nothing>' })"
if ($launchAction.Count -eq 1) {
    # NoImpersonate = 2048. WixShellExec must run as the signed-in user, or the
    # de-elevation it exists to provide never happens and Seamly2D starts as
    # admin - see the long comment in smsi.wxs.
    Assert-That -Name 'LaunchApplication runs impersonated, not as SYSTEM' `
        -Succeeded ((([int]$launchAction[0].Type) -band 2048) -eq 0) -Detail "type $($launchAction[0].Type)"
}
Assert-That -Name 'WixShellExecTarget points at the installed Seamly2D executable' `
    -Succeeded ((Get-MsiProperty -Name 'WixShellExecTarget') -eq '[#Seamly2DExe]')

# The Finish button must fire the launch BEFORE EndDialog closes the wizard,
# and only when the box is ticked on a fresh install - repeating the
# checkbox's own ShowCondition guards against a stale checked value surviving
# into a repair or removal, where the box is never even shown.
$finishEvents = @($script:controlEvents | Where-Object { $_.Dialog -eq 'ExitDialog' -and $_.Control -eq 'Finish' })
$launchPublish = @($finishEvents | Where-Object { $_.Event -eq 'DoAction' -and $_.Argument -eq 'LaunchApplication' })
$endDialogPublish = @($finishEvents | Where-Object { $_.Event -eq 'EndDialog' })
Assert-That -Name 'Finish launches Seamly2D only when the box is checked on a fresh install' `
    -Succeeded ($launchPublish.Count -eq 1 -and $launchPublish[0].Condition -match 'WIXUI_EXITDIALOGOPTIONALCHECKBOX' -and
                $launchPublish[0].Condition -match 'NOT Installed') `
    -Detail "condition '$(if ($launchPublish.Count) { $launchPublish[0].Condition } else { '<nothing>' })'"
Assert-That -Name 'Finish launches Seamly2D before the wizard closes' `
    -Succeeded ($launchPublish.Count -eq 1 -and $endDialogPublish.Count -eq 1 -and
                [int]$launchPublish[0].Ordering -lt [int]$endDialogPublish[0].Ordering) `
    -Detail "DoAction at $(if ($launchPublish.Count) { $launchPublish[0].Ordering } else { '<nothing>' }), EndDialog at $(if ($endDialogPublish.Count) { $endDialogPublish[0].Ordering } else { '<nothing>' })"

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
