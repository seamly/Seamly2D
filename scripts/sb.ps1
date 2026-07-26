#******************************************************************************
# **  @file   sb.ps1
# **  @author slspencer
# **  @date   July 25, 2026
# **
# **  @brief
# **  "seamly build" — build the whole Seamly app family locally in release
# **  configuration: seamly2d + seamlyme via qmake (shadow-built into build\)
# **  and SeamlyLayout via CMake/Cargo (src\app\seamlylayout\build.ps1).
# **  Mirrors what .github\workflows\windows-msi.yml does in CI, and produces
# **  exactly the trees scripts\packaging\windows\smsi.ps1 expects by default,
# **  so `sb.ps1` followed by `smsi.ps1` yields the family MSI.
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
    Build all three Seamly apps locally in release (Qt 6.11.1+ + VS 18 Community).

.DESCRIPTION
    One command for the whole family, because the three apps do NOT share a
    build system (see the comment block in src\app\app.pro):

      * seamly2d + seamlyme - qmake + jom, shadow-built into <repo>\build\,
        the same layout ci.yml's windows job produces and the default
        smsi.ps1 reads (build\src\app\<app>\bin).
      * SeamlyLayout        - CMake/Ninja + Cargo/Corrosion, delegated to
        src\app\seamlylayout\build.ps1 -Preset release -NoRun, which lands
        the exe in qt_frontend\build\Release (smsi.ps1's default too).

    Sharing one Qt release (6.11.1, Task 30) is what lets the resulting trees
    be packaged into a single MSI with one shared Qt runtime; it does not make
    them one build. This script is the local equivalent of the two build steps
    in .github\workflows\windows-msi.yml.

    Toolchain is auto-detected and the script fails early naming whatever is
    missing:
      * qmake  - newest C:\Qt\<version>\msvc2022_64 kit (6.11.1 or newer)
      * MSVC   - VS 18 Community vcvars64.bat
      * make   - C:\Qt\Tools\QtCreator\bin\jom\jom.exe, else nmake

    STALE BUILD TREES (Task 46): a qmake shadow-build regenerates only the
    top-level Makefile; every existing sub-Makefile is reused as-is because of
    qmake's `if not exist Makefile` guard. After a Qt upgrade that silently
    builds against the OLD kit and fails with "dependent
    'C:\Qt\<old>\...\Qt6Cored.lib' does not exist". This script therefore
    records the qmake it used in build\.seamly-qmake-kit and wipes the tree
    automatically when the detected kit differs.

    DEPLOYED RUNTIME CHECK (Task 48): after the parent build the script compares
    the FileVersion of the deployed build\src\app\<app>\bin\Qt6Core.dll against
    the kit that compiled the exes, and fails if they differ. That mismatch used
    to happen silently whenever a stray windeployqt (Qt Design Studio's reduced
    kit) was first on PATH, and smsi.ps1 would package the broken tree.

.PARAMETER Clean
    Wipe both build trees before building (the qmake build\ tree and
    SeamlyLayout's CMake preset directory).

.PARAMETER SkipParents
    Do not build seamly2d/seamlyme; build only SeamlyLayout.

.PARAMETER SkipLayout
    Do not build SeamlyLayout; build only seamly2d/seamlyme. Use this on a
    machine whose Qt kit lacks the WebEngine dependency modules (Task 44).

.EXAMPLE
    .\scripts\sb.ps1
    Build all three apps in release.

.EXAMPLE
    .\scripts\sb.ps1 -Clean
    Wipe both build trees first, then build all three.

.EXAMPLE
    .\scripts\sb.ps1 ; .\scripts\packaging\windows\smsi.ps1
    Build the family, then package it into Seamly2D-x64.msi.

.NOTES
    "sb" = seamly build, following sd.ps1 ("seamly2d debug"), st.ps1
    ("seamly2d tests") and smsi.ps1 ("seamly msi").
#>

param(
    # Wipe both build trees before building.
    [switch]$Clean,

    # Build only SeamlyLayout.
    [switch]$SkipParents,

    # Build only the parent apps (seamly2d + seamlyme).
    [switch]$SkipLayout
)

# Stop on any PowerShell-level error; native tool failures are checked via
# exit codes below.
$ErrorActionPreference = 'Stop'

#------------------------------------------------------------------------------
# @brief  Locate qmake.exe from the newest installed Qt msvc2022_64 kit.
#
# Mirrors sd.ps1's Find-QtQmake: scans C:\Qt for directories whose name parses
# as a version, keeps those meeting the family minimum, and returns the newest
# kit that actually ships msvc2022_64\bin\qmake.exe. No Qt version is
# hard-coded, so a Qt upgrade needs no edit here (Task 30/31).
#
# @return Full path to qmake.exe.
#------------------------------------------------------------------------------
function Find-QtQmake {
    $qtRoot = 'C:\Qt'
    $minimumQtVersion = [version]'6.11.1'
    if (-not (Test-Path $qtRoot)) {
        throw "Qt root '$qtRoot' not found - install Qt $minimumQtVersion or newer (msvc2022_64) first."
    }

    $kits = Get-ChildItem $qtRoot -Directory -ErrorAction SilentlyContinue |
        ForEach-Object {
            $parsed = $null
            if ([version]::TryParse($_.Name, [ref]$parsed)) {
                [pscustomobject]@{ Version = $parsed; Dir = $_.FullName }
            }
        } |
        Where-Object { $_.Version -ge $minimumQtVersion } |
        Sort-Object Version -Descending

    foreach ($kit in $kits) {
        $qmake = Join-Path $kit.Dir 'msvc2022_64\bin\qmake.exe'
        if (Test-Path $qmake) { return $qmake }
    }

    throw "No Qt $minimumQtVersion+ kit with msvc2022_64\bin\qmake.exe found under '$qtRoot'."
}

#------------------------------------------------------------------------------
# @brief  Reject a Qt whose install prefix ships no mkspecs directory.
#
# Task 47: Qt Design Studio bundles a stripped Qt
# (C:\Qt\Tools\QtDesignStudio\qt6_design_studio_reduced_version) that has no
# mkspecs\ at all and is often first on PATH. Building with it fails naming
# that prefix instead of the real kit, which is thoroughly confusing. Fail here
# with the actual cause instead.
#
# @param  QmakePath  qmake.exe to validate
#------------------------------------------------------------------------------
function Assert-FullQtKit {
    param([string]$QmakePath)

    $prefix = (& $QmakePath -query QT_INSTALL_PREFIX) -replace '/', '\'
    if (-not (Test-Path (Join-Path $prefix 'mkspecs'))) {
        throw "Qt kit at '$prefix' has no mkspecs directory - it is not a full Qt installation (a Qt Design Studio reduced Qt looks like this). Install or select a full msvc2022_64 kit."
    }
}

#------------------------------------------------------------------------------
# @brief  Locate the VS 18 Community 64-bit MSVC environment script.
#
# @return Full path to vcvars64.bat.
#------------------------------------------------------------------------------
function Find-VcVars64 {
    $vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
    if (-not (Test-Path $vcvars)) {
        throw "VS 18 Community vcvars64.bat not found at '$vcvars' - install Visual Studio 18 Community with the C++ workload."
    }
    return $vcvars
}

#------------------------------------------------------------------------------
# @brief  Verify the Qt runtime deployed beside an exe matches the build kit.
#
# Task 48: the .pro post-link step runs windeployqt, and until that step was
# changed to qtPrepareTool() it resolved the tool from PATH - which on a
# developer PC with Qt Design Studio installed is a *different*, older Qt. The
# result was a tree whose exes were linked against 6.11.1 but sat next to 6.8.7
# DLLs: silently broken, and packaged verbatim into the MSI by smsi.ps1. The
# bug was invisible until someone read the DLL's FileVersion by hand, so check
# it here on every build.
#
# Compares only major.minor.patch: Qt DLLs carry a fourth "0" field that
# qmake -query QT_VERSION does not report.
#
# @param  QmakePath  qmake.exe of the kit that compiled the exes
# @param  BinDirs    directories holding a deployed Qt6Core.dll
# @param  CoreDll    name of the core DLL to inspect (Qt6Core.dll / Qt6Cored.dll)
#------------------------------------------------------------------------------
function Assert-DeployedQtVersion {
    param(
        [string]$QmakePath,
        [string[]]$BinDirs,
        [string]$CoreDll = 'Qt6Core.dll'
    )

    $kitVersion = [version]((& $QmakePath -query QT_VERSION) | Select-Object -First 1).Trim()

    foreach ($dir in $BinDirs) {
        $corePath = Join-Path $dir $CoreDll
        if (-not (Test-Path $corePath)) {
            throw "windeployqt did not deploy '$CoreDll' into '$dir' - the post-link deploy step failed."
        }

        $info     = (Get-Item -LiteralPath $corePath).VersionInfo
        $deployed = [version]"$($info.FileMajorPart).$($info.FileMinorPart).$($info.FileBuildPart)"

        if ($deployed -ne $kitVersion) {
            throw @"
Deployed Qt runtime does not match the build kit (TODO_MIGRATE.md Task 48).
  exes linked against : Qt $kitVersion  ($QmakePath)
  DLLs deployed       : Qt $deployed  ('$corePath')
Qt's binary compatibility is forward-only, so these exes will not run and must
not be packaged. A stray windeployqt on PATH - typically Qt Design Studio's
reduced kit under C:\Qt\Tools\QtDesignStudio\ - is the usual cause. Wipe the
build tree (-Clean) and rebuild.
"@
        }
    }
}

#------------------------------------------------------------------------------
# @brief  Pick the make tool: jom (parallel) if present, else nmake.
#
# nmake needs no path - it is on PATH once vcvars64.bat has run inside the
# generated batch file.
#
# @return Full path to jom.exe, or the bare string 'nmake'.
#------------------------------------------------------------------------------
function Find-MakeTool {
    $jom = 'C:\Qt\Tools\QtCreator\bin\jom\jom.exe'
    if (Test-Path $jom) { return $jom }
    Write-Host "jom not found at '$jom' - falling back to nmake (single-threaded)."
    return 'nmake'
}

# --- Resolve toolchain and paths ---------------------------------------------
$qmake = Find-QtQmake
Assert-FullQtKit -QmakePath $qmake
$vcvars   = Find-VcVars64
$makeTool = Find-MakeTool

# The script lives in <repo-root>\scripts\, so the repo root is its parent.
$repoRoot = Split-Path -Parent $PSScriptRoot
$proPath  = Join-Path $repoRoot 'Seamly.pro'
if (-not (Test-Path $proPath)) {
    throw "Seamly.pro not found at '$proPath' - is the script still in <repo-root>\scripts\?"
}

# Release shadow-build tree. Kept at <repo>\build\ (gitignored) because that is
# where smsi.ps1 looks by default: build\src\app\seamly2d\bin and
# build\src\app\seamlyme\bin.
$buildDir  = Join-Path $repoRoot 'build'
$layoutDir = Join-Path $repoRoot 'src\app\seamlylayout'

Write-Host "qmake  : $qmake"
Write-Host "msvc   : $vcvars"
Write-Host "make   : $makeTool"
Write-Host "parents: $buildDir"
Write-Host "layout : $layoutDir"
Write-Host ''

# --- 1. Parent apps: seamly2d + seamlyme (qmake) ------------------------------
if ($SkipParents) {
    Write-Host '1 seamly2d + seamlyme: SKIPPED (-SkipParents)'
} else {
    Write-Host '1 Building seamly2d + seamlyme (qmake release)...'

    # Task 46: a shadow-build tree generated by a different Qt keeps using that
    # Qt in its sub-Makefiles. Record the kit and wipe the tree when it changes,
    # so a Qt upgrade can never produce the misleading "Qt6Cored.lib does not
    # exist" failure against an uninstalled kit.
    $kitMarker = Join-Path $buildDir '.seamly-qmake-kit'
    $wipe = $Clean.IsPresent
    if ((-not $wipe) -and (Test-Path $kitMarker)) {
        $previousKit = (Get-Content -LiteralPath $kitMarker -Raw).Trim()
        if ($previousKit -and ($previousKit -ne $qmake)) {
            Write-Host "  Qt kit changed since the last build - recreating the tree:"
            Write-Host "    was : $previousKit"
            Write-Host "    now : $qmake"
            $wipe = $true
        }
    } elseif ((-not $wipe) -and (Test-Path (Join-Path $buildDir 'Makefile'))) {
        # A tree built before this script existed carries no marker; its
        # sub-Makefiles may reference any Qt. Recreate it once to be safe.
        Write-Host '  Existing build tree has no recorded Qt kit - recreating it once.'
        $wipe = $true
    }

    if ($wipe -and (Test-Path $buildDir)) {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    # vcvars64.bat must be 'call'ed from a batch context to import the MSVC
    # environment, so the whole qmake+make sequence runs inside one cmd.exe via
    # a generated .cmd file (this also sidesteps cmd/PowerShell quoting
    # pitfalls). vcvars output is discarded: it only prints a banner plus a
    # harmless vswhere warning, and failures are still caught via exit code.
    #
    # CONFIG+=noTests matches ci.yml's and windows-msi.yml's release build - the
    # unit tests are built and run by scripts\st.ps1, not here.
    $batch = Join-Path $buildDir 'sb-build.cmd'
    @"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 echo vcvars64.bat failed & exit /b 1
"$qmake" "$proPath" -config release CONFIG+=noTests
if errorlevel 1 exit /b 1
"$makeTool"
"@ | Set-Content -Path $batch -Encoding Ascii

    # Run with the shadow-build dir as working directory so qmake writes all
    # Makefiles and objects there instead of into the source tree.
    Push-Location $buildDir
    try {
        & cmd.exe /d /c $batch
        $buildExit = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }
    if ($buildExit -ne 0) {
        throw "seamly2d/seamlyme build failed (exit code $buildExit) - see output above."
    }

    # Record the kit only after a successful build, so a failed run does not
    # suppress the next wipe.
    Set-Content -LiteralPath $kitMarker -Value $qmake -Encoding Ascii

    # Both .pro files set DESTDIR = bin and post-link windeployqt.
    foreach ($check in @(
            @{ Path = (Join-Path $buildDir 'src\app\seamly2d\bin\seamly2d.exe'); What = 'seamly2d.exe' },
            @{ Path = (Join-Path $buildDir 'src\app\seamlyme\bin\seamlyme.exe'); What = 'seamlyme.exe' })) {
        if (-not (Test-Path $check.Path)) {
            throw "Build reported success but $($check.What) is missing: '$($check.Path)'."
        }
    }

    # ...and the runtime windeployqt put there must be the kit's own (Task 48).
    Assert-DeployedQtVersion -QmakePath $qmake -BinDirs @(
        (Join-Path $buildDir 'src\app\seamly2d\bin'),
        (Join-Path $buildDir 'src\app\seamlyme\bin'))

    Write-Host '  seamly2d + seamlyme OK'
}

Write-Host ''

# --- 2. SeamlyLayout (CMake + Cargo) ------------------------------------------
if ($SkipLayout) {
    Write-Host '2 SeamlyLayout: SKIPPED (-SkipLayout)'
} else {
    Write-Host '2 Building SeamlyLayout (CMake/Cargo release)...'

    # Delegated rather than reimplemented: build.ps1 already selects the Qt kit,
    # pins QMAKE/PATH for Corrosion and cxx-qt-build (Task 47), and drives the
    # CMake release preset. -NoRun keeps it from launching the GUI, which would
    # block this script.
    $layoutBuild = Join-Path $layoutDir 'build.ps1'
    if (-not (Test-Path $layoutBuild)) {
        throw "SeamlyLayout build script not found at '$layoutBuild'."
    }

    # Splat a HASHTABLE, not an array. Splatting an array passes its elements
    # positionally, so @('-Preset','release',...) hands build.ps1 the literal
    # string "-Preset" as the value of its first positional parameter and dies
    # on its ValidateSet. A hashtable is what binds names to values.
    $layoutArgs = @{ Preset = 'release'; NoRun = $true }
    if ($Clean) { $layoutArgs['Clean'] = $true }

    $global:LASTEXITCODE = 0
    & $layoutBuild @layoutArgs
    $layoutExit = $LASTEXITCODE
    if ($layoutExit -ne 0) {
        throw "SeamlyLayout build failed (exit code $layoutExit) - see output above. If it failed at find_package(Qt6 ... WebEngineQuick), the Qt kit is missing qtwebchannel/qtpositioning (TODO_MIGRATE.md Task 44); -SkipLayout builds the parent apps only."
    }

    $layoutExe = Join-Path $layoutDir 'qt_frontend\build\Release\SeamlyLayout.exe'
    if (-not (Test-Path $layoutExe)) {
        throw "SeamlyLayout build reported success but '$layoutExe' is missing."
    }
    Write-Host '  SeamlyLayout OK'
}

# --- Summary ------------------------------------------------------------------
Write-Host ''
Write-Host 'Release build OK:'
if (-not $SkipParents) {
    Write-Host "  seamly2d     : $(Join-Path $buildDir 'src\app\seamly2d\bin\seamly2d.exe')"
    Write-Host "  seamlyme     : $(Join-Path $buildDir 'src\app\seamlyme\bin\seamlyme.exe')"
}
if (-not $SkipLayout) {
    Write-Host "  SeamlyLayout : $(Join-Path $layoutDir 'qt_frontend\build\Release\SeamlyLayout.exe')"
}
Write-Host ''
Write-Host 'Next: .\scripts\packaging\windows\smsi.ps1   (packages these trees into Seamly2D-x64.msi)'
