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
    .\smsi_check_authoring.ps1 -Msi scripts\seamly-msi\x64\seamly-x64.msi
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
# full YYYY.M.D.HHMM project version has to reach the user another way: the ARP
# comment, and the install-info registry key.
$displayVersion = @(Get-MsiRows -Sql "SELECT ``Value`` FROM ``Registry`` WHERE ``Name``='DisplayVersion'" -Columns 'Value')
Assert-That -Name 'full project version recorded in HKLM\SOFTWARE\Seamly\Seamly2D' `
    -Succeeded ($displayVersion.Count -eq 1 -and $displayVersion[0].Value -match '^\d{4}\.\d+\.\d+\.\d+$') `
    -Detail "found '$(if ($displayVersion.Count) { $displayVersion[0].Value } else { '<nothing>' })'"
Assert-That -Name 'ARPCOMMENTS carries the full project version' `
    -Succeeded ((Get-MsiProperty -Name 'ARPCOMMENTS') -match '\d{4}\.\d+\.\d+\.\d+')

# --- 3. upgrade behaviour ------------------------------------------------------
$upgrade = Get-MsiRows -Sql "SELECT ``UpgradeCode``, ``ActionProperty`` FROM ``Upgrade``" -Columns 'UpgradeCode', 'ActionProperty'
Assert-That -Name 'MajorUpgrade keyed on the fixed family UpgradeCode' `
    -Succeeded (@($upgrade | Where-Object { $_.UpgradeCode -eq '{CBF4B5F1-C32C-4DBB-B385-3EE4A7B30658}' -and $_.ActionProperty -eq 'WIX_UPGRADE_DETECTED' }).Count -eq 1)

# --- 4. previous-installation detection ---------------------------------------
# The old NSIS installer is 32-bit and never switches the registry view, so both
# of its keys live in the WOW6432Node view. RegLocator Type bit 4 (value 16) is
# msidbLocatorType64bit: it must be CLEAR or an x64 package looks in the 64-bit
# view and never finds them.
$locators = Get-MsiRows -Sql "SELECT ``Signature_``, ``Root``, ``Key``, ``Name``, ``Type`` FROM ``RegLocator``" `
    -Columns 'Signature', 'Root', 'Key', 'Name', 'Type'
$uninstallLocator = @($locators | Where-Object { $_.Name -eq 'UninstallString' })
$installDirLocator = @($locators | Where-Object { $_.Name -eq 'Install_Dir' })
Assert-That -Name 'NSIS UninstallString is searched for under HKLM' `
    -Succeeded ($uninstallLocator.Count -eq 1 -and $uninstallLocator[0].Root -eq '2')
Assert-That -Name 'NSIS UninstallString search reads the 32-bit registry view' `
    -Succeeded ($uninstallLocator.Count -eq 1 -and (([int]$uninstallLocator[0].Type) -band 16) -eq 0) `
    -Detail 'RegLocator Type has the 64-bit flag set'
Assert-That -Name 'NSIS Install_Dir search reads the 32-bit registry view' `
    -Succeeded ($installDirLocator.Count -eq 1 -and (([int]$installDirLocator[0].Type) -band 16) -eq 0)

$appSearch = @(Get-MsiRows -Sql "SELECT ``Property`` FROM ``AppSearch``" -Columns 'Property' | ForEach-Object { $_.Property })
foreach ($searched in @('SEAMLYLEGACYUNINSTALLSTRING', 'SEAMLYLEGACYINSTALLDIR')) {
    Assert-That -Name "$searched is filled in by AppSearch" -Succeeded ($appSearch -contains $searched)
}

# Public properties only survive the hand-off to the elevated server-side
# sequence when they are listed here.
$secure = Get-MsiProperty -Name 'SecureCustomProperties'
foreach ($property in @('SEAMLYDESKTOPSHORTCUTS', 'SEAMLYLEGACYUNINSTALLSTRING', 'SEAMLYLEGACYINSTALLDIR')) {
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
Assert-Transition -From 'LicenseAgreementDlg' -Control 'Next' -To 'SeamlyPreviousInstallDlg' `
    -ConditionMatch 'WIX_UPGRADE_DETECTED.*SEAMLYLEGACYUNINSTALLSTRING.*NOT Installed'
Assert-Transition -From 'LicenseAgreementDlg' -Control 'Next' -To 'InstallDirDlg' -ConditionMatch 'NOT \('
Assert-Transition -From 'SeamlyPreviousInstallDlg' -Control 'Next' -To 'InstallDirDlg'
Assert-Transition -From 'InstallDirDlg' -Control 'Next' -To 'SeamlyDataDirDlg'
Assert-Transition -From 'SeamlyDataDirDlg' -Control 'Next' -To 'SeamlyDataMigrateDlg'
Assert-Transition -From 'SeamlyDataMigrateDlg' -Control 'Next' -To 'SeamlyShortcutsDlg'
Assert-Transition -From 'SeamlyShortcutsDlg' -Control 'Next' -To 'VerifyReadyDlg'

Assert-Transition -From 'LicenseAgreementDlg' -Control 'Back' -To 'WelcomeDlg'
Assert-Transition -From 'SeamlyPreviousInstallDlg' -Control 'Back' -To 'LicenseAgreementDlg'
Assert-Transition -From 'InstallDirDlg' -Control 'Back' -To 'SeamlyPreviousInstallDlg' `
    -ConditionMatch 'WIX_UPGRADE_DETECTED.*SEAMLYLEGACYUNINSTALLSTRING.*NOT Installed'
Assert-Transition -From 'InstallDirDlg' -Control 'Back' -To 'LicenseAgreementDlg' -ConditionMatch 'NOT \('
Assert-Transition -From 'SeamlyDataDirDlg' -Control 'Back' -To 'InstallDirDlg'
Assert-Transition -From 'SeamlyDataMigrateDlg' -Control 'Back' -To 'SeamlyDataDirDlg'
Assert-Transition -From 'SeamlyShortcutsDlg' -Control 'Back' -To 'SeamlyDataMigrateDlg'
Assert-Transition -From 'VerifyReadyDlg' -Control 'Back' -To 'SeamlyShortcutsDlg' -ConditionMatch 'NOT Installed'

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
    $found = [regex]::Escape('(WIX_UPGRADE_DETECTED OR SEAMLYLEGACYUNINSTALLSTRING) AND NOT Installed')
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
foreach ($component in @('Seamly2DDesktopShortcutComponent', 'SeamlyMeDesktopShortcutComponent')) {
    $row = @($components | Where-Object { $_.Component -eq $component })
    Assert-That -Name "$component is conditional on the checkbox" `
        -Succeeded ($row.Count -eq 1 -and $row[0].Condition -eq 'SEAMLYDESKTOPSHORTCUTS')
}

# --- 6b. install location ------------------------------------------------------
# The family installs into ProgramFiles64Folder\SeamlyApps. Both halves are
# asserted because both have been wrong before in ways nothing else catches: the
# 32-bit tree would be wrong for an all-x64/arm64 package (only the OLD NSIS
# installer belongs there, being 32-bit), and the folder is named for the whole
# family rather than for seamly2d alone.
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
foreach ($name in @('Seamly2D', 'SeamlyMe')) {
    $row = @($shortcuts | Where-Object { $_.Directory -eq 'DesktopFolder' -and $_.Name -eq $name })
    Assert-That -Name "desktop shortcut '$name' targets the installed executable" `
        -Succeeded ($row.Count -eq 1 -and $row[0].Target -like '`[INSTALLFOLDER`]*.exe' -and $row[0].Icon -ne '') `
        -Detail "target is '$(if ($row.Count) { $row[0].Target } else { '<nothing>' })'"
}
# Deliberate: SeamlyLayout opens a .pieces.svg handed to it by seamly2d, so a
# desktop launch would only ever show an empty canvas.
Assert-That -Name 'SeamlyLayout has no desktop shortcut' `
    -Succeeded (@($shortcuts | Where-Object { $_.Directory -eq 'DesktopFolder' -and $_.Name -eq 'SeamlyLayout' }).Count -eq 0)

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
foreach ($property in @('SEAMLYDATAROOT', 'SEAMLYDATAPARENT', 'SEAMLYCOPYUSERDATA')) {
    Assert-That -Name "$property is a secure custom property" -Succeeded ($secure -like "*$property*")
}
# The default is computed in the UI sequence only: the execute sequence runs as
# SYSTEM, whose %USERPROFILE% is not the user's.
$uiActions = @(Get-MsiRows -Sql "SELECT ``Action`` FROM ``InstallUISequence``" -Columns 'Action' |
    ForEach-Object { $_.Action })
$executeActions = @(Get-MsiRows -Sql "SELECT ``Action`` FROM ``InstallExecuteSequence``" -Columns 'Action' |
    ForEach-Object { $_.Action })
Assert-That -Name 'the data-root default is computed in the UI sequence' `
    -Succeeded ($uiActions -contains 'SetSEAMLYDATAPARENT')
Assert-That -Name 'the data-root default is NOT computed in the elevated sequence' `
    -Succeeded (-not ($executeActions -contains 'SetSEAMLYDATAPARENT'))
Assert-That -Name 'the chosen data root is recorded for the apps to read' `
    -Succeeded (@($registry | Where-Object { $_.Root -eq '2' -and $_.Key -eq 'SOFTWARE\Seamly\Seamly2D' -and $_.Name -eq 'DataRoot' }).Count -eq 1)

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
