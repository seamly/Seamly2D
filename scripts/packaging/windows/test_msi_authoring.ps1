#******************************************************************************
# **  @file   test_msi_authoring.ps1
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
# **  CI for both architectures via .github\workflows\windows-msi.yml.
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

.PARAMETER ExpectSeamlyLayout
    Assert the SeamlyLayout shortcut and icon are present. Omit for the arm64
    package, which ships the two parent apps only.

.EXAMPLE
    .\test_msi_authoring.ps1 -Msi scripts\seamly-msi\x64\Seamly2D-x64.msi -ExpectSeamlyLayout
#>

param(
    [Parameter(Mandatory = $true)]
    [string]$Msi,

    [ValidateSet('x64', 'arm64')]
    [string]$Arch = 'x64',

    [switch]$ExpectSeamlyLayout
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
foreach ($searched in @('SEAMLYNSISUNINSTALLSTRING', 'SEAMLYNSISINSTALLDIR')) {
    Assert-That -Name "$searched is filled in by AppSearch" -Succeeded ($appSearch -contains $searched)
}

# Public properties only survive the hand-off to the elevated server-side
# sequence when they are listed here.
$secure = Get-MsiProperty -Name 'SecureCustomProperties'
foreach ($property in @('SEAMLYDESKTOPSHORTCUTS', 'SEAMLYNSISUNINSTALLSTRING', 'SEAMLYNSISINSTALLDIR')) {
    Assert-That -Name "$property is a secure custom property" -Succeeded ($secure -like "*$property*")
}

# --- 5. the "existing installation" warning dialog ----------------------------
$uiSequence = Get-MsiRows -Sql "SELECT ``Action``, ``Sequence``, ``Condition`` FROM ``InstallUISequence``" `
    -Columns 'Action', 'Sequence', 'Condition'
$warningRow = @($uiSequence | Where-Object { $_.Action -eq 'SeamlyPreviousInstallDlg' })
Assert-That -Name 'the previous-installation dialog is in the UI sequence' -Succeeded ($warningRow.Count -eq 1)
if ($warningRow.Count -eq 1) {
    $firstWixUiDialog = ($uiSequence |
        Where-Object { $_.Action -in @('WelcomeDlg', 'ResumeDlg', 'MaintenanceWelcomeDlg') } |
        ForEach-Object { [int]$_.Sequence } | Measure-Object -Minimum).Minimum
    Assert-That -Name 'the warning is shown before the first WixUI dialog' `
        -Succeeded ([int]$warningRow[0].Sequence -lt $firstWixUiDialog) `
        -Detail "warning at $($warningRow[0].Sequence), first WixUI dialog at $firstWixUiDialog"
    Assert-That -Name 'the warning triggers on either kind of existing install, and only on install' `
        -Succeeded ($warningRow[0].Condition -match 'WIX_UPGRADE_DETECTED' -and
                    $warningRow[0].Condition -match 'SEAMLYNSISUNINSTALLSTRING' -and
                    $warningRow[0].Condition -match 'NOT Installed') `
        -Detail "condition is '$($warningRow[0].Condition)'"
}

# The wording is load-bearing: Task 51 requires the dialog to name the current
# user-data folder, which Tasks 34 and 53 renamed to seamlyData.
$warningText = Get-MsiRows -Sql "SELECT ``Control``, ``Text`` FROM ``Control`` WHERE ``Dialog_``='SeamlyPreviousInstallDlg'" `
    -Columns 'Control', 'Text'
$userDataText = @($warningText | Where-Object { $_.Control -eq 'UserDataText' })
Assert-That -Name 'the warning names the seamlyData user-data folder' `
    -Succeeded ($userDataText.Count -eq 1 -and $userDataText[0].Text -match 'seamlyData')
Assert-That -Name 'the warning states that user data is not removed' `
    -Succeeded ($userDataText.Count -eq 1 -and $userDataText[0].Text -match 'not touched')
$nsisText = @($warningText | Where-Object { $_.Control -eq 'NsisText' })
Assert-That -Name 'the warning says Setup removes the old NSIS installation' `
    -Succeeded ($nsisText.Count -eq 1 -and $nsisText[0].Text -match 'Setup will remove')
# The removal takes the whole directory, so the page has to warn about anything
# of the user's that happens to be sitting in it.
Assert-That -Name 'the warning tells the user to move their own files out of it first' `
    -Succeeded ($nsisText.Count -eq 1 -and $nsisText[0].Text -match 'move anything of your own out')
Assert-That -Name 'the warning names the directory it found' `
    -Succeeded ($nsisText.Count -eq 1 -and $nsisText[0].Text -match '\[SEAMLYNSISINSTALLDIR\]')

# --- 5b. removal of the old NSIS installation ----------------------------------
# Its own uninstall.exe is deliberately never invoked - see seamly-family.wxs.
# What must be present is the removal of the four things it created.
$removeComponents = Get-MsiRows -Sql "SELECT ``Component``, ``Condition``, ``Attributes`` FROM ``Component``" `
    -Columns 'Component', 'Condition', 'Attributes'
foreach ($component in @('RemoveNsisProgramFiles', 'RemoveNsisRegistryKeys')) {
    $row = @($removeComponents | Where-Object { $_.Component -eq $component })
    Assert-That -Name "$component exists and is conditional on finding the NSIS install" `
        -Succeeded ($row.Count -eq 1 -and $row[0].Condition -eq 'SEAMLYNSISINSTALLDIR')
}
# msidbComponentAttributes64bit = 256. The NSIS keys live under WOW6432Node
# because that installer was 32-bit and never switched view, so the component
# carrying the RemoveRegistryKey rows must NOT have the 64-bit bit set.
$registryRemoval = @($removeComponents | Where-Object { $_.Component -eq 'RemoveNsisRegistryKeys' })
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
foreach ($property in @('SEAMLYNSISINSTALLDIR', 'SEAMLYNSISSTARTMENU')) {
    # InstallMode 1 = remove on install, which is the point: the old product has
    # to be gone before this one takes over its shortcuts and associations.
    Assert-That -Name "'$property' is scheduled for recursive removal on install" `
        -Succeeded (@($removeFolderEx | Where-Object {
            $_.Property -eq $property -and $_.InstallMode -eq '1' -and
            $_.Component -eq 'RemoveNsisProgramFiles' }).Count -eq 1)
}

$conditions = Get-MsiRows -Sql "SELECT ``Control_``, ``Action``, ``Condition`` FROM ``ControlCondition`` WHERE ``Dialog_``='SeamlyPreviousInstallDlg'" `
    -Columns 'Control', 'Action', 'Condition'
foreach ($control in @('UpgradeText', 'NsisText')) {
    Assert-That -Name "$control is shown and hidden by condition" `
        -Succeeded ((@($conditions | Where-Object { $_.Control -eq $control -and $_.Action -eq 'Show' }).Count -eq 1) -and
                    (@($conditions | Where-Object { $_.Control -eq $control -and $_.Action -eq 'Hide' }).Count -eq 1))
}

# --- 6. optional desktop shortcuts --------------------------------------------
Assert-That -Name 'desktop shortcuts default to on' -Succeeded ((Get-MsiProperty -Name 'SEAMLYDESKTOPSHORTCUTS') -eq '1')
$checkBoxes = Get-MsiRows -Sql "SELECT ``Property``, ``Value`` FROM ``CheckBox``" -Columns 'Property', 'Value'
Assert-That -Name 'the shortcuts checkbox sets SEAMLYDESKTOPSHORTCUTS' `
    -Succeeded (@($checkBoxes | Where-Object { $_.Property -eq 'SEAMLYDESKTOPSHORTCUTS' -and $_.Value -eq '1' }).Count -eq 1)

# The checkbox dialog must be spawned before the built-in transition to
# VerifyReadyDlg - that ordering is the one thing this authoring depends on.
$nextEvents = Get-MsiRows -Sql "SELECT ``Event``, ``Argument``, ``Ordering`` FROM ``ControlEvent`` WHERE ``Dialog_``='InstallDirDlg' AND ``Control_``='Next'" `
    -Columns 'Event', 'Argument', 'Ordering'
$spawn = @($nextEvents | Where-Object { $_.Event -eq 'SpawnDialog' -and $_.Argument -eq 'SeamlyShortcutsDlg' })
$leave = @($nextEvents | Where-Object { $_.Event -eq 'NewDialog' })
Assert-That -Name 'the shortcuts dialog is spawned from the install-directory page' -Succeeded ($spawn.Count -eq 1)
Assert-That -Name 'it is spawned before the page hands off to the ready page' `
    -Succeeded ($spawn.Count -eq 1 -and $leave.Count -ge 1 -and
                [int]$spawn[0].Ordering -lt ([int](($leave | ForEach-Object { [int]$_.Ordering } | Measure-Object -Minimum).Minimum))) `
    -Detail 'SpawnDialog must have a lower Ordering than every NewDialog event'

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
$expectedStartMenu = @('Seamly2D', 'SeamlyMe')
if ($ExpectSeamlyLayout) { $expectedStartMenu += 'SeamlyLayout' }
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
$expectedIcons = @('seamly2d.ico', 'seamlyme.ico')
if ($ExpectSeamlyLayout) { $expectedIcons += 'seamlylayout.ico' }
foreach ($icon in $expectedIcons) {
    Assert-That -Name "icon '$icon' is packaged" -Succeeded ($icons -contains $icon)
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

# --- report --------------------------------------------------------------------
[System.Runtime.InteropServices.Marshal]::ReleaseComObject($script:database) | Out-Null

Write-Host ''
if ($script:failures.Count -gt 0) {
    Write-Host "MSI authoring check FAILED - $($script:failures.Count) problem(s):"
    $script:failures | ForEach-Object { Write-Host "  - $_" }
    exit 1
}
Write-Host 'MSI authoring check passed.'
# Explicit, so a caller reading $LASTEXITCODE after `& test_msi_authoring.ps1`
# sees 0 rather than whatever the previous command left there.
exit 0
