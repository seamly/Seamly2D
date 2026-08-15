<#
 ******************************************************************************
 **  @file   smsi_migrate_user_data.ps1
 **  @author slspencer
 **  @date   August 11, 2026
 **
 **  @brief
 **  Copies a user's existing Seamly work into a newly chosen data root.
 **
 **  Run by the MSI as a deferred, impersonated custom action when the user
 **  ticks "copy my existing patterns and measurements" on SeamlyDataMigrateDlg
 **  (Task InstWinX64.1.2.4). Also runnable by hand.
 **
 **  THE THREE RULES THIS SCRIPT EXISTS TO ENFORCE
 **    1. Never delete anything. The source is only ever read.
 **    2. Never overwrite anything. A file already present at the destination
 **       wins, always, and is left byte-for-byte as it was.
 **    3. Never fail the install. Every error is reported and swallowed; the
 **       exit code is always 0.
 **
 **  Rule 2 is what makes the operation safe to repeat. Running it twice copies
 **  only what is still missing, so an interrupted run can simply be run again.
 **
 **  @copyright
 **  This source code is part of the Seamly project, a suite of apparel CAD
 **  software.
 **  Copyright (C) 2026 Seamly Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
 **
 **  @license
 **  Seamly2D/SeamlyMe is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Seamly2D/SeamlyMe is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Seamly2D/SeamlyMe.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
#>

<#
.SYNOPSIS
    Copies existing Seamly user data into a new data root, without deleting or
    overwriting anything.

.DESCRIPTION
    Searches the folders earlier Seamly versions used for patterns and
    measurements, and copies any file that the destination does not already
    have. Existing destination files are never replaced. Source files are never
    changed or removed.

    The script always exits 0. It is run during an installation, where failing
    would roll back a working program install over a file-copy problem.

.PARAMETER Destination
    The new data root, e.g. C:\Users\susan\SeamlyData. Created if absent.

.PARAMETER Source
    Extra folder to copy from, on top of the known previous locations. Optional.

.PARAMETER LogPath
    Where to write the transcript. Defaults to
    %LOCALAPPDATA%\Seamly\smsi_migrate_user_data.log.

.EXAMPLE
    .\smsi_migrate_user_data.ps1 -Destination "E:\SeamlyData"
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Destination,

    [string]$Source,

    [string]$LogPath
)

# Never abort the installation. Every failure below is caught and logged.
$ErrorActionPreference = 'Continue'

if (-not $LogPath) {
    $LogPath = Join-Path $env:LOCALAPPDATA 'Seamly\smsi_migrate_user_data.log'
}

<#
.SYNOPSIS
    Appends one timestamped line to the log, and never throws.
#>
function Write-Log {
    param([string]$Message)

    $line = "{0}  {1}" -f (Get-Date -Format 's'), $Message
    Write-Output $line
    try {
        $dir = Split-Path -Parent $LogPath
        if ($dir -and -not (Test-Path $dir)) {
            New-Item -ItemType Directory -Path $dir -Force | Out-Null
        }
        Add-Content -Path $LogPath -Value $line -Encoding utf8
    } catch {
        # A log that cannot be written must not stop the copy.
    }
}

<#
.SYNOPSIS
    Returns the folders earlier Seamly versions kept user data in.

.DESCRIPTION
    Order matters only for logging; a file found in two sources is copied from
    whichever is read first, and the second is then skipped by the
    already-exists rule.
#>
function Get-SourceCandidate {
    $profileDir = $env:USERPROFILE
    $documents  = [Environment]::GetFolderPath('MyDocuments')

    $candidates = @(
        # Current default: Documents\Seamly
        (Join-Path $documents 'Seamly'),
        # Pre-MSI layout, written by the old installer's apps
        (Join-Path $profileDir 'seamly2d'),
        (Join-Path $documents 'seamly2d'),
        # Lowercase variant named in the installer's own dialog text
        (Join-Path $profileDir 'seamlyData')
    )

    if ($Source) {
        $candidates = @($Source) + $candidates
    }

    return $candidates
}

Write-Log "=== copy to '$Destination' ==="

# A destination equal to a source would make the copy meaningless and risks
# walking a tree while writing into it.
try {
    if (-not (Test-Path $Destination)) {
        New-Item -ItemType Directory -Path $Destination -Force | Out-Null
        Write-Log "created destination"
    }
    $destFull = (Resolve-Path $Destination).Path
} catch {
    Write-Log "FAILED to create destination: $_"
    exit 0
}

$copied  = 0
$skipped = 0
$failed  = 0

foreach ($candidate in Get-SourceCandidate) {
    if (-not $candidate) { continue }
    if (-not (Test-Path $candidate)) { continue }

    try {
        $sourceFull = (Resolve-Path $candidate).Path
    } catch {
        continue
    }

    if ($sourceFull -eq $destFull) {
        Write-Log "skip '$sourceFull' - same folder as the destination"
        continue
    }

    Write-Log "reading '$sourceFull'"

    try {
        $files = Get-ChildItem -Path $sourceFull -Recurse -File -Force -ErrorAction Stop
    } catch {
        Write-Log "FAILED to list '$sourceFull': $_"
        $failed++
        continue
    }

    foreach ($file in $files) {
        # Rebuild the tree under the destination, so a pattern in a subfolder
        # stays in that subfolder.
        $relative = $file.FullName.Substring($sourceFull.Length).TrimStart('\')
        $target   = Join-Path $destFull $relative

        # RULE 2. The destination always wins. Checked immediately before the
        # copy rather than once up front, so a file that appears mid-run is
        # still respected.
        if (Test-Path -LiteralPath $target) {
            $skipped++
            continue
        }

        try {
            $targetDir = Split-Path -Parent $target
            if ($targetDir -and -not (Test-Path $targetDir)) {
                New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
            }
            # -Confirm:$false and no -Force: -Force would overwrite a read-only
            # destination file, which rule 2 forbids.
            Copy-Item -LiteralPath $file.FullName -Destination $target -Confirm:$false -ErrorAction Stop
            $copied++
        } catch {
            Write-Log "FAILED '$($file.FullName)': $_"
            $failed++
        }
    }
}

Write-Log "done - $copied copied, $skipped already present, $failed failed"

# RULE 3. The installation continues whatever happened above.
exit 0
