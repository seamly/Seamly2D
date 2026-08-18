<#
 ******************************************************************************
 **  @file   smsi_migrate_user_data_test.ps1
 **  @author slspencer
 **  @date   August 18, 2026
 **
 **  @brief
 **  Tests old-version and new-version MSI data migration.
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
$migrationScript = Join-Path $PSScriptRoot 'smsi_migrate_user_data.ps1'
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('seamly-msi-migration-' + [guid]::NewGuid().ToString('N'))

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
    Returns the archive's top-level directory names.
#>
function Get-ArchiveRoot {
    param([Parameter(Mandatory = $true)][string]$Path)

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($Path)
    try {
        return @($archive.Entries | ForEach-Object { ($_.FullName -split '/')[0] } | Sort-Object -Unique)
    } finally {
        $archive.Dispose()
    }
}

try {
    New-Item -ItemType Directory -Path $testRoot -Force | Out-Null

    $oldRoot = Join-Path $testRoot 'old\seamly2d'
    $oldRoaming = Join-Path $testRoot 'old\roaming'
    $oldLocal = Join-Path $testRoot 'old\local'
    $oldSettingsDirectory = Join-Path $oldRoaming 'Seamly2DTeam'
    $oldSettings = Join-Path $oldSettingsDirectory 'qt6_common.ini'
    $oldDestination = Join-Path $testRoot 'old\new-parent\SeamlyData'
    $oldArchive = Join-Path $testRoot 'old\seamly2d.zip'
    New-Item -ItemType Directory -Path (Join-Path $oldRoot 'patterns'), $oldSettingsDirectory,
        (Join-Path $oldDestination 'patterns') -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $oldRoot 'patterns\shirt.sm2d') -Value 'source pattern'
    Set-Content -LiteralPath (Join-Path $oldRoot 'patterns\keep.sm2d') -Value 'source value'
    Set-Content -LiteralPath (Join-Path $oldDestination 'patterns\keep.sm2d') -Value 'destination value'
    Set-Content -LiteralPath $oldSettings -Value @(
        '[configuration]',
        'theme=dark',
        '[paths]',
        "pattern=$($oldRoot.Replace('\', '/'))/patterns",
        '[pattern]',
        "defaultPatternTemplate=$($oldRoot.Replace('\', '/'))/label templates/old.xml",
        'graphicalOutput=true'
    )

    & $migrationScript -Mode Old -Destination $oldDestination `
        -RoamingSettingsRoot $oldRoaming -LocalSettingsRoot $oldLocal -ArchivePath $oldArchive `
        -InstallFolder 'C:\Program Files\SeamlyApps'

    Assert-That -Name 'old migration keeps the source tree' -Succeeded (Test-Path -LiteralPath $oldRoot)
    Assert-That -Name 'old migration copies a missing file' `
        -Succeeded (Test-Path -LiteralPath (Join-Path $oldDestination 'patterns\shirt.sm2d'))
    Assert-That -Name 'old migration does not overwrite an existing file' `
        -Succeeded ((Get-Content -LiteralPath (Join-Path $oldDestination 'patterns\keep.sm2d') -Raw).Trim() -eq 'destination value')
    Assert-That -Name 'old archive has seamly2d as its top-level directory' `
        -Succeeded (@(Get-ArchiveRoot -Path $oldArchive) -join ',' -eq 'seamly2d')
    Assert-That -Name 'old migration adds every standard directory' `
        -Succeeded (@('measurements\individual', 'measurements\multisize', 'templates', 'bodyscans',
            'label templates', 'images', 'backups', 'patterns', 'layouts' | Where-Object {
                -not (Test-Path -LiteralPath (Join-Path $oldDestination $_))
            }).Count -eq 0)
    $oldSettingsText = Get-Content -LiteralPath $oldSettings -Raw
    Assert-That -Name 'old migration retains non-path settings' `
        -Succeeded ($oldSettingsText -match 'theme=dark' -and $oldSettingsText -match 'graphicalOutput=true')
    Assert-That -Name 'old migration replaces data path settings' `
        -Succeeded ($oldSettingsText -match [regex]::Escape("dataRoot=$($oldDestination.Replace('\', '/'))"))
    Assert-That -Name 'old migration replaces label-template paths' `
        -Succeeded ($oldSettingsText -match [regex]::Escape("defaultPatternTemplate=$($oldDestination.Replace('\', '/'))/label templates/default_pattern_label.xml"))

    $unsafeArchive = Join-Path $testRoot 'old\unsafe.zip'
    & $migrationScript -Mode Old -Destination (Join-Path $oldRoot 'SeamlyData') -Source $oldRoot `
        -RoamingSettingsRoot $oldRoaming -LocalSettingsRoot $oldLocal -ArchivePath $unsafeArchive
    Assert-That -Name 'migration rejects a destination inside the source tree' `
        -Succeeded (-not (Test-Path -LiteralPath $unsafeArchive))

    $newRoot = Join-Path $testRoot 'new\current\SeamlyData'
    $newRoaming = Join-Path $testRoot 'new\roaming'
    $newLocal = Join-Path $testRoot 'new\local'
    $newSettingsDirectory = Join-Path $newRoaming 'Seamly'
    $newSettings = Join-Path $newSettingsDirectory 'qt6_common.ini'
    $newDestination = Join-Path $testRoot 'new\selected-parent\SeamlyData'
    $newArchive = Join-Path $testRoot 'new\seamly2d.zip'
    New-Item -ItemType Directory -Path (Join-Path $newRoot 'measurements'), $newSettingsDirectory -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $newRoot 'measurements\person.smis') -Value 'measurements'
    Set-Content -LiteralPath $newSettings -Value @(
        '[configuration]',
        'language=en',
        '[paths]',
        "dataRoot=$($newRoot.Replace('\', '/'))"
    )

    & $migrationScript -Mode New -Destination $newDestination -PreviousDataRoot $newRoot `
        -RoamingSettingsRoot $newRoaming -LocalSettingsRoot $newLocal -ArchivePath $newArchive

    Assert-That -Name 'new migration keeps the source tree' -Succeeded (Test-Path -LiteralPath $newRoot)
    Assert-That -Name 'new migration copies the complete tree' `
        -Succeeded (Test-Path -LiteralPath (Join-Path $newDestination 'measurements\person.smis'))
    Assert-That -Name 'new archive has SeamlyData as its top-level directory' `
        -Succeeded (@(Get-ArchiveRoot -Path $newArchive) -join ',' -ceq 'SeamlyData')
    $newSettingsText = Get-Content -LiteralPath $newSettings -Raw
    Assert-That -Name 'new migration retains non-path settings' -Succeeded ($newSettingsText -match 'language=en')
    Assert-That -Name 'new migration records the selected root' `
        -Succeeded ($newSettingsText -match [regex]::Escape("dataRoot=$($newDestination.Replace('\', '/'))"))

    $sameArchive = Join-Path $testRoot 'new\same-location.zip'
    & $migrationScript -Mode New -Destination $newRoot -PreviousDataRoot $newRoot `
        -RoamingSettingsRoot $newRoaming -LocalSettingsRoot $newLocal -ArchivePath $sameArchive
    Assert-That -Name 'new migration does nothing when the location is unchanged' `
        -Succeeded (-not (Test-Path -LiteralPath $sameArchive))
} finally {
    if (Test-Path -LiteralPath $testRoot) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}

Write-Output "$script:passed passed, $script:failed failed"
if ($script:failed -gt 0) {
    exit 1
}
exit 0
