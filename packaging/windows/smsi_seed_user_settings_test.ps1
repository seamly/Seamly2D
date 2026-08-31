<#
 ******************************************************************************
 **  @file   smsi_seed_user_settings_test.ps1
 **  @author slspencer
 **  @date   August 31, 2026
 **
 **  @brief
 **  Tests the install-time seeding of the per-user settings files.
 **
 **  @copyright
 **  Copyright (C) 2026 Seamly2D Project
 **  All Rights Reserved.
 **
 **  @license
 **  GPL-3.0-or-later
 ******************************************************************************
#>

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$script:passed = 0
$script:failed = 0
$seedScript = Join-Path $PSScriptRoot 'smsi_seed_user_settings.ps1'
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('seamly-msi-seed-' + [guid]::NewGuid().ToString('N'))

<#
.SYNOPSIS
    Records one test result.
#>
function Assert-That {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][bool]$Succeeded
    )

    if ($Succeeded) {
        $script:passed++
        Write-Output "PASS: $Name"
    } else {
        $script:failed++
        Write-Output "FAIL: $Name"
    }
}

<#
.SYNOPSIS
    Reads one key value from one ini section, or $null when absent.
#>
function Get-IniValue {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Section,
        [Parameter(Mandatory = $true)][string]$Key
    )

    $inSection = $false
    foreach ($line in (Get-Content -LiteralPath $Path)) {
        $trimmed = $line.Trim()
        if ($trimmed -match '^\[(.+)\]$') {
            $inSection = ($Matches[1] -eq $Section)
        } elseif ($inSection -and $trimmed -match '^([^=]+)=(.*)$' -and $Matches[1].Trim() -eq $Key) {
            return $Matches[2]
        }
    }
    return $null
}

try {
    New-Item -ItemType Directory -Path $testRoot -Force | Out-Null

    # --- Fresh seeding -----------------------------------------------------
    $freshLocal = Join-Path $testRoot 'fresh'
    & $seedScript -DataRoot 'C:\Users\test\Documents\SeamlyData\' `
        -InstallFolder 'C:\Program Files\SeamlyApps\' -LocalSettingsRoot $freshLocal | Out-Null

    $common = Join-Path $freshLocal 'Seamly\qt6_common.ini'
    $s2d = Join-Path $freshLocal 'Seamly\Seamly2D\qt6_seamly2d.ini'
    $sme = Join-Path $freshLocal 'Seamly\SeamlyMe\qt6_seamlyme.ini'

    Assert-That -Name 'fresh seeding creates qt6_common.ini' -Succeeded (Test-Path -LiteralPath $common)
    Assert-That -Name 'fresh seeding creates qt6_seamly2d.ini' -Succeeded (Test-Path -LiteralPath $s2d)
    Assert-That -Name 'fresh seeding creates an empty qt6_seamlyme.ini' `
        -Succeeded ((Test-Path -LiteralPath $sme) -and ((Get-Item -LiteralPath $sme).Length -eq 0))
    $slay = Join-Path $freshLocal 'Seamly\SeamlyLayout\qt6_seamlylayout.ini'
    Assert-That -Name 'fresh seeding creates qt6_seamlylayout.ini' -Succeeded (Test-Path -LiteralPath $slay)
    Assert-That -Name 'fresh seeding creates the SeamlyLayout settings and preferences directories' `
        -Succeeded ((Test-Path -LiteralPath (Join-Path $freshLocal 'Seamly\SeamlyLayout\settings')) -and
                    (Test-Path -LiteralPath (Join-Path $freshLocal 'Seamly\SeamlyLayout\preferences')))

    Assert-That -Name 'the data root key uses the / separator form without a trailing slash' `
        -Succeeded ((Get-IniValue -Path $common -Section 'paths' -Key 'dataRoot') -eq 'C:/Users/test/Documents/SeamlyData')
    Assert-That -Name 'the shared measurement and template keys sit under the data root' `
        -Succeeded (((Get-IniValue -Path $common -Section 'paths' -Key 'individual_size_measurements') -eq 'C:/Users/test/Documents/SeamlyData/measurements/individual') -and
                    ((Get-IniValue -Path $common -Section 'paths' -Key 'multi_size_measurements') -eq 'C:/Users/test/Documents/SeamlyData/measurements/multisize') -and
                    ((Get-IniValue -Path $common -Section 'paths' -Key 'templates') -eq 'C:/Users/test/Documents/SeamlyData/templates') -and
                    ((Get-IniValue -Path $common -Section 'paths' -Key 'bodyscans') -eq 'C:/Users/test/Documents/SeamlyData/bodyscans'))
    Assert-That -Name 'the Seamly2D per-app path keys sit under the data root' `
        -Succeeded (((Get-IniValue -Path $s2d -Section 'paths' -Key 'pattern') -eq 'C:/Users/test/Documents/SeamlyData/patterns') -and
                    ((Get-IniValue -Path $s2d -Section 'paths' -Key 'layout') -eq 'C:/Users/test/Documents/SeamlyData/layouts') -and
                    ((Get-IniValue -Path $s2d -Section 'paths' -Key 'labels') -eq 'C:/Users/test/Documents/SeamlyData/label templates') -and
                    ((Get-IniValue -Path $s2d -Section 'paths' -Key 'images') -eq 'C:/Users/test/Documents/SeamlyData/images') -and
                    ((Get-IniValue -Path $s2d -Section 'paths' -Key 'backups') -eq 'C:/Users/test/Documents/SeamlyData/backups'))
    Assert-That -Name 'the seamlyLayoutApp key points into the install folder' `
        -Succeeded ((Get-IniValue -Path $s2d -Section 'paths' -Key 'seamlyLayoutApp') -eq 'C:/Program Files/SeamlyApps/SeamlyLayout.exe')

    $layoutRoot = 'C:/Users/test/Documents/SeamlyData'
    $layoutConfig = ((Join-Path $freshLocal 'Seamly\SeamlyLayout') -replace '\\', '/')
    Assert-That -Name 'the SeamlyLayout data keys sit under the data root' `
        -Succeeded (((Get-IniValue -Path $slay -Section 'General' -Key 'input_directory') -eq "$layoutRoot/layouts") -and
                    ((Get-IniValue -Path $slay -Section 'General' -Key 'layout_directory') -eq "$layoutRoot/layouts") -and
                    ((Get-IniValue -Path $slay -Section 'General' -Key 'data_root') -eq $layoutRoot))
    Assert-That -Name 'the SeamlyLayout app-config keys sit under its settings directory' `
        -Succeeded (((Get-IniValue -Path $slay -Section 'General' -Key 'settings_directory') -eq "$layoutConfig/settings") -and
                    ((Get-IniValue -Path $slay -Section 'General' -Key 'preferences_directory') -eq "$layoutConfig/preferences") -and
                    ((Get-IniValue -Path $slay -Section 'General' -Key 'settings_file') -eq "$layoutConfig/settings/default_settings.json") -and
                    ((Get-IniValue -Path $slay -Section 'General' -Key 'preferences_file') -eq "$layoutConfig/preferences/default_preferences.json"))
    Assert-That -Name 'the SeamlyLayout viewer keys match the bundled defaults' `
        -Succeeded (((Get-IniValue -Path $slay -Section 'General' -Key 'dxf_viewer_path') -eq 'https://sharecad.org') -and
                    ((Get-IniValue -Path $slay -Section 'General' -Key 'pdf_viewer_path') -eq '') -and
                    ((Get-IniValue -Path $slay -Section 'General' -Key 'png_viewer_path') -eq '') -and
                    ((Get-IniValue -Path $slay -Section 'General' -Key 'projector_path') -eq 'https://patternprojector.com'))

    Assert-That -Name 'fresh seeding marks the first-run data notice pending' `
        -Succeeded ((Get-IniValue -Path $common -Section 'notices' -Key 'firstRunDataNotice') -eq 'pending')

    $commonBytes = [System.IO.File]::ReadAllBytes($common)
    Assert-That -Name 'the seeded files carry no UTF-8 BOM' `
        -Succeeded (-not ($commonBytes.Length -ge 3 -and $commonBytes[0] -eq 0xEF -and $commonBytes[1] -eq 0xBB -and $commonBytes[2] -eq 0xBF))

    # --- Merging into existing files ---------------------------------------
    $mergeLocal = Join-Path $testRoot 'merge'
    $mergeCommonDirectory = Join-Path $mergeLocal 'Seamly'
    New-Item -ItemType Directory -Path $mergeCommonDirectory -Force | Out-Null
    $mergeCommon = Join-Path $mergeCommonDirectory 'qt6_common.ini'
    [System.IO.File]::WriteAllText($mergeCommon, @'
[configuration]
theme=dark

[paths]
dataRoot=D:/CustomRoot
templates=D:/CustomRoot/my templates
'@, [System.Text.UTF8Encoding]::new($false))

    & $seedScript -DataRoot 'C:\Users\test\Documents\SeamlyData' `
        -InstallFolder 'C:\Program Files\SeamlyApps' -LocalSettingsRoot $mergeLocal | Out-Null

    Assert-That -Name 'merging keeps an existing dataRoot value' `
        -Succeeded ((Get-IniValue -Path $mergeCommon -Section 'paths' -Key 'dataRoot') -eq 'D:/CustomRoot')
    Assert-That -Name 'merging keeps an existing templates value' `
        -Succeeded ((Get-IniValue -Path $mergeCommon -Section 'paths' -Key 'templates') -eq 'D:/CustomRoot/my templates')
    Assert-That -Name 'merging adds the missing bodyscans key' `
        -Succeeded ((Get-IniValue -Path $mergeCommon -Section 'paths' -Key 'bodyscans') -eq 'C:/Users/test/Documents/SeamlyData/bodyscans')
    Assert-That -Name 'merging leaves other sections alone' `
        -Succeeded ((Get-IniValue -Path $mergeCommon -Section 'configuration' -Key 'theme') -eq 'dark')
    Assert-That -Name 'an existing qt6_common.ini gets no first-run data notice' `
        -Succeeded ($null -eq (Get-IniValue -Path $mergeCommon -Section 'notices' -Key 'firstRunDataNotice'))

    $mergeLayoutDirectory = Join-Path $mergeLocal 'Seamly\SeamlyLayout'
    $mergeLayout = Join-Path $mergeLayoutDirectory 'qt6_seamlylayout.ini'
    Assert-That -Name 'merging keeps an existing SeamlyLayout value and adds the missing keys' `
        -Succeeded ((Get-IniValue -Path $mergeLayout -Section 'General' -Key 'data_root') -eq 'C:/Users/test/Documents/SeamlyData')
    [System.IO.File]::WriteAllText($mergeLayout, @'
[General]
layout_directory=E:/MyLayouts
'@, [System.Text.UTF8Encoding]::new($false))
    & $seedScript -DataRoot 'C:\Users\test\Documents\SeamlyData' `
        -InstallFolder 'C:\Program Files\SeamlyApps' -LocalSettingsRoot $mergeLocal | Out-Null
    Assert-That -Name 'merging keeps an existing layout_directory value' `
        -Succeeded ((Get-IniValue -Path $mergeLayout -Section 'General' -Key 'layout_directory') -eq 'E:/MyLayouts')
    Assert-That -Name 'merging completes a partial SeamlyLayout ini' `
        -Succeeded ((Get-IniValue -Path $mergeLayout -Section 'General' -Key 'input_directory') -eq 'C:/Users/test/Documents/SeamlyData/layouts')

    # --- A file without the section gets the section appended ---------------
    $sectionLocal = Join-Path $testRoot 'section'
    $sectionDirectory = Join-Path $sectionLocal 'Seamly\Seamly2D'
    New-Item -ItemType Directory -Path $sectionDirectory -Force | Out-Null
    $sectionIni = Join-Path $sectionDirectory 'qt6_seamly2d.ini'
    [System.IO.File]::WriteAllText($sectionIni, @'
[configuration]
unit=inch
'@, [System.Text.UTF8Encoding]::new($false))

    & $seedScript -DataRoot 'C:\Users\test\Documents\SeamlyData' `
        -InstallFolder 'C:\Program Files\SeamlyApps' -LocalSettingsRoot $sectionLocal | Out-Null

    Assert-That -Name 'a file without a paths section gains one' `
        -Succeeded ((Get-IniValue -Path $sectionIni -Section 'paths' -Key 'pattern') -eq 'C:/Users/test/Documents/SeamlyData/patterns')
    Assert-That -Name 'the existing configuration section survives' `
        -Succeeded ((Get-IniValue -Path $sectionIni -Section 'configuration' -Key 'unit') -eq 'inch')

    # --- A complete file stays byte-identical --------------------------------
    $repeatBefore = [System.IO.File]::ReadAllBytes($common)
    & $seedScript -DataRoot 'C:\Users\test\Documents\SeamlyData' `
        -InstallFolder 'C:\Program Files\SeamlyApps' -LocalSettingsRoot $freshLocal | Out-Null
    $repeatAfter = [System.IO.File]::ReadAllBytes($common)
    Assert-That -Name 'a second run leaves a complete file byte-identical' `
        -Succeeded ([System.Linq.Enumerable]::SequenceEqual($repeatBefore, $repeatAfter))

    # --- An existing SeamlyMe ini is never touched ---------------------------
    $meLocal = Join-Path $testRoot 'me'
    $meDirectory = Join-Path $meLocal 'Seamly\SeamlyMe'
    New-Item -ItemType Directory -Path $meDirectory -Force | Out-Null
    $meIni = Join-Path $meDirectory 'qt6_seamlyme.ini'
    [System.IO.File]::WriteAllText($meIni, "[configuration]`r`nunit=cm`r`n", [System.Text.UTF8Encoding]::new($false))
    & $seedScript -DataRoot 'C:\Users\test\Documents\SeamlyData' `
        -InstallFolder 'C:\Program Files\SeamlyApps' -LocalSettingsRoot $meLocal | Out-Null
    Assert-That -Name 'an existing qt6_seamlyme.ini keeps its content' `
        -Succeeded ((Get-IniValue -Path $meIni -Section 'configuration' -Key 'unit') -eq 'cm')

    # --- An empty data root seeds nothing, still exit 0 ----------------------
    $emptyLocal = Join-Path $testRoot 'empty'
    & $seedScript -DataRoot ' ' -InstallFolder 'C:\Program Files\SeamlyApps' -LocalSettingsRoot $emptyLocal | Out-Null
    Assert-That -Name 'an empty data root exits 0' -Succeeded ($LASTEXITCODE -eq 0)
    Assert-That -Name 'an empty data root seeds no files' `
        -Succeeded (-not (Test-Path -LiteralPath (Join-Path $emptyLocal 'Seamly\qt6_common.ini')))
} finally {
    if (Test-Path -LiteralPath $testRoot) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force -Confirm:$false
    }
}

Write-Output "$script:passed passed, $script:failed failed"
if ($script:failed -gt 0) {
    exit 1
}
exit 0
