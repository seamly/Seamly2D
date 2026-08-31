<#
 ******************************************************************************
 **  @file   smsi_migrate_user_data.ps1
 **  @author slspencer
 **  @date   August 18, 2026
 **
 **  @brief
 **  Archives and migrates a Windows user's Seamly data tree.
 **
 **  @copyright
 **  Copyright (C) 2026 Seamly2D Project
 **  All Rights Reserved.
 **
 **  @license
 **  GPL-3.0-or-later
 ******************************************************************************
#>

<#
.SYNOPSIS
    Migrates an old or new Seamly data tree into a selected SeamlyData directory.

.DESCRIPTION
    Old migrations archive a seamly2d tree. The script extracts it and renames
    the extracted root to SeamlyData.

    New migrations archive a SeamlyData tree. The archive keeps SeamlyData as
    its top-level directory.

    The source tree remains unchanged. Existing destination files always win.
    The script updates path settings only after all copied files verify.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Old', 'New')]
    [string]$Mode,

    [Parameter(Mandatory = $true)]
    [string]$Destination,

    [string]$PreviousDataRoot,
    [string]$Source,
    [string]$InstallFolder,
    [string]$RoamingSettingsRoot,
    [string]$LocalSettingsRoot,
    [string]$ArchivePath,
    [string]$LogPath
)

$ErrorActionPreference = 'Stop'

if (-not $RoamingSettingsRoot) {
    $RoamingSettingsRoot = $env:APPDATA
}
if (-not $LocalSettingsRoot) {
    $LocalSettingsRoot = $env:LOCALAPPDATA
}
if (-not $LogPath) {
    $LogPath = Join-Path $LocalSettingsRoot 'Seamly\smsi_migrate_user_data.log'
}

<#
.SYNOPSIS
    Writes one migration log entry without stopping the migration.
#>
function Write-Log {
    param([string]$Message)

    $line = '{0}  {1}' -f (Get-Date -Format 's'), $Message
    Write-Output $line
    try {
        $logDirectory = Split-Path -Parent $LogPath
        if ($logDirectory -and -not (Test-Path -LiteralPath $logDirectory)) {
            New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
        }
        Add-Content -LiteralPath $LogPath -Value $line -Encoding utf8
    } catch {
        Write-Output "Could not write the migration log: $_"
    }
}

<#
.SYNOPSIS
    Returns the known Seamly settings files that exist for the current user.
#>
function Get-SettingsFile {
    $roots = @(
        (Join-Path $RoamingSettingsRoot 'Seamly'),
        (Join-Path $RoamingSettingsRoot 'Seamly2DTeam'),
        (Join-Path $LocalSettingsRoot 'Seamly\Seamly2D'),
        (Join-Path $LocalSettingsRoot 'Seamly\SeamlyMe')
    )

    $files = @()
    foreach ($root in $roots) {
        if (Test-Path -LiteralPath $root) {
            $files += Get-ChildItem -LiteralPath $root -Filter '*.ini' -File -Recurse -ErrorAction SilentlyContinue
        }
    }

    $flatLegacy = Join-Path $RoamingSettingsRoot 'Unknown Organization.ini'
    if (Test-Path -LiteralPath $flatLegacy) {
        $files += Get-Item -LiteralPath $flatLegacy
    }

    # Task SettingsFiles.1: the shared common settings file lives in
    # %LOCALAPPDATA%\Seamly\qt6_common.ini. The Roaming roots above stay for installs
    # made before the move. Named explicitly rather than recursing the Seamly root,
    # which would also sweep up SeamlyLayout's own preferences file.
    $localCommon = Join-Path $LocalSettingsRoot 'Seamly\qt6_common.ini'
    if (Test-Path -LiteralPath $localCommon) {
        $files += Get-Item -LiteralPath $localCommon
    }

    return @($files | Sort-Object FullName -Unique)
}

<#
.SYNOPSIS
    Reads all values from an INI file's paths section.
#>
function Get-IniPathValue {
    param([Parameter(Mandatory = $true)][string]$Path)

    $values = @{}
    $section = ''
    foreach ($line in Get-Content -LiteralPath $Path -ErrorAction Stop) {
        if ($line -match '^\s*\[([^]]+)\]\s*$') {
            $section = $Matches[1]
            continue
        }
        if ($section -ieq 'paths' -and $line -match '^\s*([^=]+?)\s*=\s*(.*)$') {
            $values[$Matches[1].Trim()] = $Matches[2].Trim()
        }
    }
    return $values
}

<#
.SYNOPSIS
    Returns a legacy seamly2d root derived from application path settings.
#>
function Find-LegacyDataRoot {
    param([System.IO.FileInfo[]]$SettingsFiles)

    foreach ($file in $SettingsFiles) {
        $values = Get-IniPathValue -Path $file.FullName
        if ($values.ContainsKey('dataRoot') -and $values['dataRoot']) {
            $candidate = $values['dataRoot'].Replace('/', '\')
            if ((Split-Path -Leaf $candidate) -ieq 'seamly2d' -and (Test-Path -LiteralPath $candidate -PathType Container)) {
                return (Resolve-Path -LiteralPath $candidate).Path
            }
        }

        foreach ($value in $values.Values) {
            $candidate = $value.Replace('/', '\')
            if ($candidate -match '^(.*[\\/]seamly2d)(?:[\\/].*)?$') {
                $root = $Matches[1]
                if (Test-Path -LiteralPath $root -PathType Container) {
                    return (Resolve-Path -LiteralPath $root).Path
                }
            }
        }
    }

    $defaultLegacyRoot = Join-Path $env:USERPROFILE 'seamly2d'
    if (Test-Path -LiteralPath $defaultLegacyRoot -PathType Container) {
        return (Resolve-Path -LiteralPath $defaultLegacyRoot).Path
    }
    return $null
}

<#
.SYNOPSIS
    Returns the current SeamlyData root for a new-version relocation.
#>
function Find-NewDataRoot {
    param([System.IO.FileInfo[]]$SettingsFiles)

    if ($PreviousDataRoot -and (Test-Path -LiteralPath $PreviousDataRoot -PathType Container)) {
        return (Resolve-Path -LiteralPath $PreviousDataRoot).Path
    }

    foreach ($file in $SettingsFiles) {
        $values = Get-IniPathValue -Path $file.FullName
        if ($values.ContainsKey('dataRoot') -and $values['dataRoot']) {
            $candidate = $values['dataRoot'].Replace('/', '\')
            if ((Split-Path -Leaf $candidate) -ieq 'SeamlyData' -and
                (Test-Path -LiteralPath $candidate -PathType Container)) {
                return (Resolve-Path -LiteralPath $candidate).Path
            }
        }
    }
    return $null
}

<#
.SYNOPSIS
    Creates an archive with the source directory as its top-level directory.
#>
function New-DataArchive {
    param(
        [Parameter(Mandatory = $true)][string]$SourceRoot,
        [Parameter(Mandatory = $true)][string]$Path
    )

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $sourcePath = (Resolve-Path -LiteralPath $SourceRoot).Path
    $sourceName = Split-Path -Leaf $sourcePath
    $archive = [System.IO.Compression.ZipFile]::Open($Path, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        $archive.CreateEntry("$sourceName/") | Out-Null
        foreach ($directory in Get-ChildItem -LiteralPath $sourcePath -Directory -Recurse -Force) {
            $relative = $directory.FullName.Substring($sourcePath.Length).TrimStart('\').Replace('\', '/')
            $archive.CreateEntry("$sourceName/$relative/") | Out-Null
        }
        foreach ($file in Get-ChildItem -LiteralPath $sourcePath -File -Recurse -Force) {
            $relative = $file.FullName.Substring($sourcePath.Length).TrimStart('\').Replace('\', '/')
            [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $archive, $file.FullName, "$sourceName/$relative",
                [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
        }
    } finally {
        $archive.Dispose()
    }
}

<#
.SYNOPSIS
    Copies a complete tree without overwriting and verifies each copied file.
#>
function Merge-DataTree {
    param(
        [Parameter(Mandatory = $true)][string]$SourceRoot,
        [Parameter(Mandatory = $true)][string]$DestinationRoot
    )

    if (-not (Test-Path -LiteralPath $DestinationRoot)) {
        New-Item -ItemType Directory -Path $DestinationRoot -Force | Out-Null
    }

    $sourcePath = (Resolve-Path -LiteralPath $SourceRoot).Path
    $destinationPath = (Resolve-Path -LiteralPath $DestinationRoot).Path
    $copied = 0
    $skipped = 0

    foreach ($directory in Get-ChildItem -LiteralPath $sourcePath -Directory -Recurse -Force) {
        $relative = $directory.FullName.Substring($sourcePath.Length).TrimStart('\')
        $targetDirectory = Join-Path $destinationPath $relative
        if (-not (Test-Path -LiteralPath $targetDirectory)) {
            New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
        }
    }

    foreach ($file in Get-ChildItem -LiteralPath $sourcePath -File -Recurse -Force) {
        $relative = $file.FullName.Substring($sourcePath.Length).TrimStart('\')
        $target = Join-Path $destinationPath $relative
        if (Test-Path -LiteralPath $target) {
            $skipped++
            continue
        }

        $targetDirectory = Split-Path -Parent $target
        if (-not (Test-Path -LiteralPath $targetDirectory)) {
            New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
        }
        Copy-Item -LiteralPath $file.FullName -Destination $target -Confirm:$false
        if ((Get-Item -LiteralPath $target).Length -ne $file.Length) {
            Remove-Item -LiteralPath $target -Force
            throw "Copy verification failed for '$($file.FullName)'."
        }
        $copied++
    }

    Write-Log "$copied file(s) copied; $skipped existing file(s) kept"
}

<#
.SYNOPSIS
    Adds the standard SeamlyData directories without changing existing objects.
#>
function Add-StandardDirectory {
    param([Parameter(Mandatory = $true)][string]$Root)

    $directories = @(
        'measurements\individual',
        'measurements\multisize',
        'templates',
        'bodyscans',
        'label templates',
        'images',
        'backups',
        'patterns',
        'layouts'
    )
    foreach ($directory in $directories) {
        $path = Join-Path $Root $directory
        if (-not (Test-Path -LiteralPath $path)) {
            New-Item -ItemType Directory -Path $path -Force | Out-Null
        }
    }
}

<#
.SYNOPSIS
    Replaces path settings while preserving all non-path settings.
#>
function Set-IniPathValue {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][hashtable]$Values,
        [Parameter(Mandatory = $true)][hashtable]$PatternValues
    )

    $sourceLines = @(Get-Content -LiteralPath $Path -ErrorAction Stop)
    $result = [System.Collections.Generic.List[string]]::new()
    $section = ''
    $pathsWritten = $false
    $patternKeysWritten = @{}

    foreach ($line in $sourceLines) {
        if ($line -match '^\s*\[([^]]+)\]\s*$') {
            if ($section -ieq 'paths' -and -not $pathsWritten) {
                foreach ($key in ($Values.Keys | Sort-Object)) {
                    $result.Add("$key=$($Values[$key])")
                }
                $pathsWritten = $true
            }
            $section = $Matches[1]
            $result.Add($line)
            continue
        }

        if ($section -ieq 'paths') {
            continue
        }
        if ($section -ieq 'pattern' -and $line -match '^\s*([^=]+?)\s*=') {
            $key = $Matches[1].Trim()
            if ($PatternValues.ContainsKey($key)) {
                $result.Add("$key=$($PatternValues[$key])")
                $patternKeysWritten[$key] = $true
                continue
            }
        }
        $result.Add($line)
    }

    if ($section -ieq 'paths' -and -not $pathsWritten) {
        foreach ($key in ($Values.Keys | Sort-Object)) {
            $result.Add("$key=$($Values[$key])")
        }
        $pathsWritten = $true
    }
    if (-not $pathsWritten) {
        if ($result.Count -gt 0 -and $result[$result.Count - 1] -ne '') {
            $result.Add('')
        }
        $result.Add('[paths]')
        foreach ($key in ($Values.Keys | Sort-Object)) {
            $result.Add("$key=$($Values[$key])")
        }
    }

    $missingPatternKeys = @($PatternValues.Keys | Where-Object { -not $patternKeysWritten.ContainsKey($_) })
    if ($missingPatternKeys.Count -gt 0) {
        $patternSection = -1
        for ($index = 0; $index -lt $result.Count; $index++) {
            if ($result[$index] -match '^\s*\[pattern\]\s*$') {
                $patternSection = $index
                break
            }
        }
        if ($patternSection -lt 0) {
            $result.Add('')
            $result.Add('[pattern]')
            foreach ($key in ($missingPatternKeys | Sort-Object)) {
                $result.Add("$key=$($PatternValues[$key])")
            }
        } else {
            $insertAt = $patternSection + 1
            while ($insertAt -lt $result.Count -and $result[$insertAt] -notmatch '^\s*\[') {
                $insertAt++
            }
            foreach ($key in ($missingPatternKeys | Sort-Object -Descending)) {
                $result.Insert($insertAt, "$key=$($PatternValues[$key])")
            }
        }
    }

    Set-Content -LiteralPath $Path -Value $result -Encoding utf8
}

<#
.SYNOPSIS
    Replaces every discovered path setting with a destination-relative value.
#>
function Update-PathSettings {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][System.IO.FileInfo[]]$SettingsFiles,
        [Parameter(Mandatory = $true)][string]$Root
    )

    $qtRoot = $Root.Replace('\', '/')
    $layoutExecutable = if ($InstallFolder) {
        (Join-Path $InstallFolder 'SeamlyLayout.exe').Replace('\', '/')
    } else {
        ''
    }
    $values = @{
        'dataRoot' = $qtRoot
        'individual_size_measurements' = "$qtRoot/measurements/individual"
        'multi_size_measurements' = "$qtRoot/measurements/multisize"
        'templates' = "$qtRoot/templates"
        'bodyscans' = "$qtRoot/bodyscans"
        'labels' = "$qtRoot/label templates"
        'images' = "$qtRoot/images"
        'backups' = "$qtRoot/backups"
        'pattern' = "$qtRoot/patterns"
        'layout' = "$qtRoot/layouts"
    }
    if ($layoutExecutable) {
        $values['seamlyLayoutApp'] = $layoutExecutable
    }
    $patternValues = @{
        'defaultPatternTemplate' = "$qtRoot/label templates/default_pattern_label.xml"
        'defaultPieceTemplate' = "$qtRoot/label templates/default_piece_label.xml"
    }

    foreach ($file in $SettingsFiles) {
        Set-IniPathValue -Path $file.FullName -Values $values -PatternValues $patternValues
        Write-Log "updated path settings in '$($file.FullName)'"
    }
}

$settingsFiles = @(Get-SettingsFile)
$destinationParent = Split-Path -Parent $Destination
$destinationLeaf = Split-Path -Leaf ($Destination.TrimEnd('\'))
if ($destinationLeaf -ine 'SeamlyData') {
    Write-Log "FAILED: destination '$Destination' does not end in SeamlyData"
    exit 0
}

try {
    $sourceRoot = if ($Source) {
        (Resolve-Path -LiteralPath $Source).Path
    } elseif ($Mode -eq 'Old') {
        Find-LegacyDataRoot -SettingsFiles $settingsFiles
    } else {
        Find-NewDataRoot -SettingsFiles $settingsFiles
    }

    if (-not $sourceRoot) {
        throw "No $Mode Seamly data tree was found."
    }
    $expectedLeaf = if ($Mode -eq 'Old') { 'seamly2d' } else { 'SeamlyData' }
    if ((Split-Path -Leaf $sourceRoot) -ine $expectedLeaf) {
        throw "The $Mode source root '$sourceRoot' is not named $expectedLeaf."
    }

    $sourceRoot = [System.IO.Path]::GetFullPath($sourceRoot).TrimEnd('\')
    $destinationFull = [System.IO.Path]::GetFullPath($Destination).TrimEnd('\')
    if ($destinationFull.StartsWith($sourceRoot + '\', [System.StringComparison]::OrdinalIgnoreCase) -or
        $sourceRoot.StartsWith($destinationFull + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw 'The source and destination directories cannot contain each other.'
    }

    if (Test-Path -LiteralPath $Destination) {
        $destinationResolved = (Resolve-Path -LiteralPath $Destination).Path
        if ($sourceRoot -ieq $destinationResolved) {
            Write-Log 'source and destination are unchanged; no migration is required'
            exit 0
        }
    }

    if (-not (Test-Path -LiteralPath $destinationParent)) {
        New-Item -ItemType Directory -Path $destinationParent -Force | Out-Null
    }
    $workRoot = Join-Path $destinationParent ('.seamly-migration-' + [guid]::NewGuid().ToString('N'))
    $extractRoot = Join-Path $workRoot 'expanded'
    New-Item -ItemType Directory -Path $extractRoot -Force | Out-Null

    $temporaryArchive = -not $ArchivePath
    if (-not $ArchivePath) {
        $ArchivePath = Join-Path $workRoot 'seamly2d.zip'
    }
    Write-Log "archiving '$sourceRoot' as '$ArchivePath'"
    New-DataArchive -SourceRoot $sourceRoot -Path $ArchivePath
    Expand-Archive -LiteralPath $ArchivePath -DestinationPath $extractRoot

    $extractedRoot = Join-Path $extractRoot $expectedLeaf
    if (-not (Test-Path -LiteralPath $extractedRoot -PathType Container)) {
        throw "The archive does not contain $expectedLeaf as its top-level directory."
    }
    if ($Mode -eq 'Old') {
        $renamedRoot = Join-Path $extractRoot 'SeamlyData'
        Rename-Item -LiteralPath $extractedRoot -NewName 'SeamlyData'
        $extractedRoot = $renamedRoot
    }

    Merge-DataTree -SourceRoot $extractedRoot -DestinationRoot $Destination
    Add-StandardDirectory -Root $Destination
    Update-PathSettings -SettingsFiles $settingsFiles -Root (Resolve-Path -LiteralPath $Destination).Path
    Write-Log "migration completed from '$sourceRoot' to '$Destination'"

    if ($temporaryArchive -and (Test-Path -LiteralPath $workRoot)) {
        Remove-Item -LiteralPath $workRoot -Recurse -Force
    }
} catch {
    Write-Log "FAILED: $_"
}

# A data migration failure must not roll back a valid application installation.
exit 0
