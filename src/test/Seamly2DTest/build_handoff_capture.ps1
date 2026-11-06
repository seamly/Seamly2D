# @file build_handoff_capture.ps1
# @author slspencer
# @brief Builds a real-pattern writer probe without changing shipped application sources.
# Copyright (C) 2026 Seamly2D Project
# SPDX-License-Identifier: GPL-3.0-or-later
# License: https://www.gnu.org/licenses/gpl-3.0.html

param(
    [Parameter(Mandatory = $true)][string]$QtDirectory,
    [Parameter(Mandatory = $true)][string]$VisualStudioEnvironment,
    [string]$BuildDirectory = 'build\handoff-writer-comparison',
    [switch]$ReuseLibraries
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$build = [IO.Path]::GetFullPath((Join-Path $root $BuildDirectory))
if (-not $build.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The comparison build must remain inside this checkout.'
}
$qmake = Join-Path $QtDirectory 'bin\qmake.exe'
$libraries = @('vtools', 'tools', 'vwidgets', 'vformat', 'vlayout', 'vpatterndb',
               'vgeometry', 'fervor', 'ifc', 'vmisc', 'qmuparser', 'vpropertyexplorer', 'layout_handoff')

# Each command keeps the MSVC environment and build in the same cmd.exe process.
function Build-Project([string]$directory, [string]$project) {
    New-Item -ItemType Directory -Force $directory | Out-Null
    $command = 'call "{0}" >nul && cd /d "{1}" && "{2}" "{3}" CONFIG+=release CONFIG-=debug ' +
               'CONFIG+=deferDeploy CONFIG-=precompile_header QMAKE_CXXFLAGS+=/MP4 && nmake /nologo'
    $command = $command -f $VisualStudioEnvironment, $directory, $qmake, $project
    $command = '(' + $command + ') > "' + (Join-Path $directory 'build.log') + '" 2>&1'
    & $env:ComSpec /c $command
    if ($LASTEXITCODE -ne 0) {
        Get-Content (Join-Path $directory 'build.log') -Tail 12
        throw "Comparison build failed: $directory"
    }
}

foreach ($library in $libraries) {
    $directory = Join-Path $build "libraries\$library"
    if (-not $ReuseLibraries -or -not (Test-Path (Join-Path $directory "bin\$library.lib"))) {
        Build-Project $directory (Join-Path $root "src\libs\$library\$library.pro")
    }
}

$application = Join-Path $build 'application'
New-Item -ItemType Directory -Force $application | Out-Null
$source = Get-Content (Join-Path $root 'src\app\seamly2d\mainwindow.cpp') -Raw
$marker = '        if (loaded && (cmd->IsTestModeEnabled() || layout_export))'
if (-not $source.Contains($marker)) {
    throw 'The loaded-pattern capture insertion point changed. Update the test probe.'
}
$probe = Get-Content (Join-Path $PSScriptRoot 'handoff_capture_probe.inc') -Raw
$encoding = New-Object System.Text.UTF8Encoding($false)
[IO.File]::WriteAllText((Join-Path $application 'comparison_mainwindow.cpp'),
                       $source.Replace($marker, $probe + "`r`n" + $marker), $encoding)

# qmake requires slash-separated paths inside project files on every platform.
$source_directory = (Join-Path $root 'src\app\seamly2d').Replace('\', '/')
$project = @"
include("$source_directory/seamly2d.pro")
TARGET = handoff_capture
CONFIG += console deferDeploy
CONFIG -= windows
SOURCES -= "$source_directory/mainwindow.cpp"
SOURCES += comparison_mainwindow.cpp
INCLUDEPATH += "$source_directory"
RESOURCES -= share/resources/cursor.qrc share/resources/toolicon.qrc share/resources/sounds.qrc
RESOURCES += "$source_directory/share/resources/cursor.qrc" "$source_directory/share/resources/toolicon.qrc"
RC_FILE = "$source_directory/share/resources/seamly2d.rc"
INCLUDEPATH += ../libraries/tools/uic ../libraries/vtools/uic
LIBS =
PRE_TARGETDEPS =
"@
foreach ($library in $libraries) {
    $project += "`r`nLIBS += ../libraries/$library/bin/$library.lib"
}
$xerces = (Join-Path $root 'src\libs\xerces-c\msvc\lib\xerces-c_3.lib').Replace('\', '/')
$project += "`r`nLIBS += `"$xerces`" user32.lib`r`n"
[IO.File]::WriteAllText((Join-Path $application 'capture.pro'), $project, $encoding)
Build-Project $application (Join-Path $application 'capture.pro')
Write-Output (Join-Path $application 'bin\handoff_capture.exe')
