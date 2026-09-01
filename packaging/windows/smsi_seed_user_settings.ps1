<#
 ******************************************************************************
 **  @file   smsi_seed_user_settings.ps1
 **  @author slspencer
 **  @date   August 31, 2026
 **
 **  @brief
 **  Seeds the per-user Seamly settings directories and ini files at install time.
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
    Creates the per-user settings directories and seeds the path settings.

.DESCRIPTION
    Tasks SettingsFiles.2 and SettingsFiles.3. After this script runs,
    %LOCALAPPDATA%\Seamly holds qt6_common.ini, Seamly2D\qt6_seamly2d.ini,
    SeamlyMe\qt6_seamlyme.ini, and SeamlyLayout\qt6_seamlylayout.ini with
    every path key present, so no app requires a Preferences > Paths visit
    and no app has to seed its own ini on first run.

    The script writes a file only when it is absent. In an existing file it
    adds only missing keys. It never changes an existing value, so an
    upgrade keeps the configuration smsi_migrate_user_data.ps1 carried over.

    SeamlyLayout's ini must be COMPLETE — all 11 keys. PreferencesModel::
    load() treats an existing ini as fully authoritative: a partial ini
    would suppress the app's own (now deprecated) first-run seeding and
    leave the missing keys empty. The values below mirror what
    seedFromBundledDefaults() derives from default_preferences.json with
    ${DATAROOT} resolved to the recorded data root.

    Values use Qt's '/' separator form. The folder names match the English
    defaults the MSI itself creates under the data root.

    Task SettingsFiles.5: when qt6_common.ini is newly created (a fresh
    machine), the script also seeds [notices] firstRunDataNotice=pending.
    The first Seamly app to run shows a one-shot notice about the data
    locations and backups, then rewrites the value as 'shown'.

.PARAMETER DataRoot
    The recorded user-data root, e.g. C:\Users\name\Documents\SeamlyData.

.PARAMETER InstallFolder
    The resolved INSTALLFOLDER, used for the seamlyLayoutApp key.

.PARAMETER LocalSettingsRoot
    Test override for %LOCALAPPDATA%.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$DataRoot,

    [Parameter(Mandatory = $true)]
    [string]$InstallFolder,

    [string]$LocalSettingsRoot,
    [string]$LogPath
)

$ErrorActionPreference = 'Stop'

if (-not $LocalSettingsRoot) {
    $LocalSettingsRoot = $env:LOCALAPPDATA
}
if (-not $LogPath) {
    $LogPath = Join-Path $LocalSettingsRoot 'Seamly\smsi_seed_user_settings.log'
}

<#
.SYNOPSIS
    Writes one log entry without stopping the seeding.
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
        Write-Output "Could not write the seeding log: $_"
    }
}

<#
.SYNOPSIS
    Converts a path to Qt's cleaned '/' separator form.
#>
function ConvertTo-QtPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return ($Path.Trim() -replace '\\', '/').TrimEnd('/')
}

<#
.SYNOPSIS
    Adds missing keys to one ini section; never changes an existing key.

.DESCRIPTION
    Creates the file with only the given section when it is absent. In an
    existing file, appends each missing key at the end of the section, and
    appends the whole section when the section is absent. Existing keys keep
    their values even when they differ from the seed values.

    Writes UTF-8 without BOM. PowerShell 5.1's -Encoding utf8 writes a BOM,
    which Qt's ini parser treats as part of the first section name.
#>
function Add-IniKey {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Section,
        [Parameter(Mandatory = $true)][System.Collections.Specialized.OrderedDictionary]$Pairs
    )

    $newline = [System.Environment]::NewLine

    if (-not (Test-Path -LiteralPath $Path)) {
        $content = "[$Section]$newline"
        foreach ($key in $Pairs.Keys) {
            $content += '{0}={1}{2}' -f $key, $Pairs[$key], $newline
        }
        [System.IO.File]::WriteAllText($Path, $content, [System.Text.UTF8Encoding]::new($false))
        Write-Log "created '$Path' with $($Pairs.Count) key(s) in [$Section]"
        return
    }

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]](Get-Content -LiteralPath $Path))

    # Find the section and the keys it already holds.
    $sectionStart = -1
    $sectionEnd = $lines.Count
    $existingKeys = @()
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i].Trim()
        if ($line -match '^\[(.+)\]$') {
            if ($sectionStart -ge 0) {
                $sectionEnd = $i
                break
            }
            if ($Matches[1] -eq $Section) {
                $sectionStart = $i
            }
        } elseif ($sectionStart -ge 0 -and $line -match '^([^=;#]+)=') {
            $existingKeys += $Matches[1].Trim()
        }
    }

    $missing = [System.Collections.Specialized.OrderedDictionary]::new()
    foreach ($key in $Pairs.Keys) {
        if ($existingKeys -notcontains $key) {
            $missing[$key] = $Pairs[$key]
        }
    }
    if ($missing.Count -eq 0) {
        Write-Log "'$Path' [$Section] already holds every key; unchanged"
        return
    }

    if ($sectionStart -lt 0) {
        if ($lines.Count -gt 0 -and $lines[$lines.Count - 1].Trim() -ne '') {
            $lines.Add('')
        }
        $lines.Add("[$Section]")
        foreach ($key in $missing.Keys) {
            $lines.Add(('{0}={1}' -f $key, $missing[$key]))
        }
    } else {
        # Insert before trailing blank lines so the keys sit inside the section.
        $insertAt = $sectionEnd
        while ($insertAt -gt ($sectionStart + 1) -and $lines[$insertAt - 1].Trim() -eq '') {
            $insertAt--
        }
        foreach ($key in $missing.Keys) {
            $lines.Insert($insertAt, ('{0}={1}' -f $key, $missing[$key]))
            $insertAt++
        }
    }

    [System.IO.File]::WriteAllText($Path,
        (($lines -join $newline) + $newline),
        [System.Text.UTF8Encoding]::new($false))
    Write-Log "added $($missing.Count) missing key(s) to '$Path' [$Section]"
}

try {
    Write-Log "seeding user settings: DataRoot='$DataRoot' InstallFolder='$InstallFolder'"

    if (-not $DataRoot.Trim()) {
        Write-Log 'no data root given; nothing to seed'
        exit 0
    }

    $root = ConvertTo-QtPath $DataRoot
    $install = ConvertTo-QtPath $InstallFolder

    $seamlyRoot = Join-Path $LocalSettingsRoot 'Seamly'
    foreach ($directory in @($seamlyRoot,
                             (Join-Path $seamlyRoot 'Seamly2D'),
                             (Join-Path $seamlyRoot 'SeamlyMe'),
                             (Join-Path $seamlyRoot 'SeamlyLayout'))) {
        if (-not (Test-Path -LiteralPath $directory)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
            Write-Log "created directory '$directory'"
        }
    }

    # Shared keys. The apps read these from qt6_common.ini
    # (VCommonSettings::commonSettingsFilePath()).
    $commonKeys = [ordered]@{
        'dataRoot'                     = $root
        'individual_size_measurements' = "$root/measurements/individual"
        'multi_size_measurements'      = "$root/measurements/multisize"
        'templates'                    = "$root/templates"
        'bodyscans'                    = "$root/bodyscans"
    }
    $commonIni = Join-Path $seamlyRoot 'qt6_common.ini'
    # Task SettingsFiles.5: an absent qt6_common.ini marks a fresh machine.
    # Only then is the one-shot first-run data notice due — an existing file
    # means a previous install already ran here.
    $freshMachine = -not (Test-Path -LiteralPath $commonIni)
    Add-IniKey -Path $commonIni -Section 'paths' -Pairs $commonKeys
    if ($freshMachine) {
        # The first Seamly app to run shows the data-location notice, then
        # rewrites this value as 'shown'.
        Add-IniKey -Path $commonIni -Section 'notices' -Pairs ([ordered]@{
            'firstRunDataNotice' = 'pending'
        })
    }

    # Per-app keys. labels/images/backups are per-app, not shared: their
    # setters call QSettings::setValue on the app's own settings object
    # (vcommonsettings.cpp / vsettings.cpp).
    $seamly2dKeys = [ordered]@{
        'pattern'         = "$root/patterns"
        'layout'          = "$root/layouts"
        'labels'          = "$root/label templates"
        'images'          = "$root/images"
        'backups'         = "$root/backups"
        'seamlyLayoutApp' = "$install/SeamlyLayout.exe"
    }
    Add-IniKey -Path (Join-Path $seamlyRoot 'Seamly2D\qt6_seamly2d.ini') -Section 'paths' -Pairs $seamly2dKeys

    # SeamlyMe's path keys live in qt6_common.ini. Its own file only has to
    # exist; the app supplies every other default at runtime.
    $seamlyMeIni = Join-Path $seamlyRoot 'SeamlyMe\qt6_seamlyme.ini'
    if (-not (Test-Path -LiteralPath $seamlyMeIni)) {
        [System.IO.File]::WriteAllText($seamlyMeIni, '', [System.Text.UTF8Encoding]::new($false))
        Write-Log "created empty '$seamlyMeIni'"
    }

    # SeamlyLayout: the complete key set PreferencesModel::save() writes,
    # in the [General] section (QSettings default group). Values mirror
    # seedFromBundledDefaults() + default_preferences.json (windows block)
    # with ${DATAROOT} resolved to the data root. The set must stay complete:
    # load() takes an existing ini as authoritative and its missing-key
    # fallbacks are empty strings.
    $layoutConfig = ConvertTo-QtPath (Join-Path $seamlyRoot 'SeamlyLayout')
    foreach ($directory in @((Join-Path $seamlyRoot 'SeamlyLayout\settings'),
                             (Join-Path $seamlyRoot 'SeamlyLayout\preferences'))) {
        if (-not (Test-Path -LiteralPath $directory)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
            Write-Log "created directory '$directory'"
        }
    }
    $layoutKeys = [ordered]@{
        'input_directory'        = "$root/layouts"
        'layout_directory'       = "$root/layouts"
        'preferences_directory'  = "$layoutConfig/preferences"
        'settings_directory'     = "$layoutConfig/settings"
        'settings_file'          = "$layoutConfig/settings/default_settings.json"
        'preferences_file'       = "$layoutConfig/preferences/default_preferences.json"
        'dxf_viewer_path'        = 'https://sharecad.org'
        'pdf_viewer_path'        = ''
        'png_viewer_path'        = ''
        'projector_path'         = 'https://patternprojector.com'
        'data_root'              = $root
    }
    Add-IniKey -Path (Join-Path $seamlyRoot 'SeamlyLayout\qt6_seamlylayout.ini') -Section 'General' -Pairs $layoutKeys

    Write-Log 'seeding completed'
} catch {
    Write-Log "seeding failed: $_"
}

# A seeding problem must never fail or roll back the install.
exit 0
