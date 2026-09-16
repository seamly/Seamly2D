<#
 ******************************************************************************
 **  @file   smsi_ensure_user_data.ps1
 **  @author slspencer
 **  @date   September 15, 2026
 **
 **  @brief
 **  Ensures the fixed Seamly data root and per-user settings exist and are
 **  complete, without ever touching a user's existing files.
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
    Creates the standard data subfolders under DataRoot and seeds the
    per-user settings ini files.

.DESCRIPTION
    Runs on every install that has a resolved data root - fresh, update, and
    repair alike. DataRoot is never chosen here: smsi.wxs resolves it to
    either the fixed fresh-install default ([%USERPROFILE]\seamly2d) or
    whatever an earlier install already recorded, so this script only ever
    fills in what is missing.

    Two things, both purely additive:

    1. The nine standard subfolders under DataRoot (measurements/individual,
       measurements/multisize, templates, bodyscans, label templates,
       images, backups, patterns, layouts) - created if absent, left alone
       if present.
    2. %LOCALAPPDATA%\Seamly's ini files: qt6_common.ini,
       Seamly2D\qt6_seamly2d.ini, SeamlyMe\qt6_seamlyme.ini, and
       SeamlyLayout\qt6_seamlylayout.ini. A file is created only when
       absent; an existing file only gets missing keys added, so an update
       or repair never overwrites a value already there - including one the
       user changed in Preferences.

    SeamlyLayout's ini must be COMPLETE - all 11 keys. PreferencesModel::
    load() treats an existing ini as fully authoritative: a partial ini
    would leave the missing keys empty. The values below mirror what
    seedFromBundledDefaults() derives from default_preferences.json with
    ${DATAROOT} resolved to DataRoot.

    Values use Qt's '/' separator form. The folder names match the English
    defaults this script creates under the data root.

.PARAMETER DataRoot
    The resolved user-data root, e.g. C:\Users\name\seamly2d.

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

# The MSI custom-action command line pads each path argument with a space
# before the closing quote, so a property's trailing backslash cannot escape
# that quote. Trim the padding off here.
$DataRoot = $DataRoot.Trim()
$InstallFolder = $InstallFolder.Trim()

if (-not $LocalSettingsRoot) {
    $LocalSettingsRoot = $env:LOCALAPPDATA
}
if (-not $LogPath) {
    $LogPath = Join-Path $LocalSettingsRoot 'Seamly\smsi_ensure_user_data.log'
}

<#
.SYNOPSIS
    Writes one log entry without stopping the rest of the script.
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
        Write-Output "Could not write the log: $_"
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
    Adds the standard data-root directories without changing existing objects.
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
            Write-Log "created directory '$path'"
        }
    }
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
    Write-Log "ensuring user data: DataRoot='$DataRoot' InstallFolder='$InstallFolder'"

    if (-not $DataRoot) {
        Write-Log 'no data root given; nothing to do'
        exit 0
    }

    Add-StandardDirectory -Root $DataRoot

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
    Add-IniKey -Path (Join-Path $seamlyRoot 'qt6_common.ini') -Section 'paths' -Pairs $commonKeys

    # Per-app keys. labels/images/backups are per-app, not shared: their
    # setters call QSettings::setValue on the app's own settings object
    # (vcommonsettings.cpp / vsettings.cpp).
    $seamly2dKeys = [ordered]@{
        'pattern'         = "$root/patterns"
        'layout'          = "$root/layouts"
        'labels'          = "$root/label templates"
        'images'          = "$root/images"
        'backups'         = "$root/backups"
        'seamlyLayoutApp' = "$install/seamlylayout.exe"
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

    Write-Log 'ensure completed'
} catch {
    Write-Log "ensure failed: $_"
}

# A problem here must never fail or roll back the install.
exit 0
