#******************************************************************************
# **  @file   st.ps1
# **  @author slspencer
# **  @date   July 18, 2026
# **
# **  @brief
# **  "seamly2d tests" — local unit-test runner for the Seamly2DTests suite,
# **  mirroring the sd.ps1 build-script style. Sets up the DLL search path
# **  and Qt plugin path the test executable needs on Windows, runs the
# **  suite with per-suite file logging (working around the lost-stdout
# **  issue), and prints an aggregated pass/fail summary.
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
    Run the Seamly2DTests unit-test suite locally on Windows.

.DESCRIPTION
    Runs the debug-built Seamly2DTests.exe from the sd.ps1 shadow build
    (<repo-root>\scripts\seamly2d-build-debug\), or the release build\ tree
    with -Release. Build first with scripts\sd.ps1 — this script only runs.

    Two Windows-specific traps are handled (Task 23):

      * DLL / plugin resolution — the test exe needs the Qt DLLs, the
        xerces-c DLL, and the Qt platform plugin (platforms\qwindows[d].dll).
        The Seamly2DTest.pro post-link step deploys all of these beside the
        test exe; as a belt-and-braces fallback for older build trees this
        script also puts the seamly2d.exe bin directory on PATH and sets
        QT_PLUGIN_PATH to whichever directory has a platforms\ subdirectory.
        (Without the platform plugin, QGuiApplication startup qFatals into a
        hidden modal dialog and the suite appears to hang.)

      * Lost stdout — QTest console output from the suite is lost when
        redirected on this setup, and a single "-o file,txt" logger argument
        is overwritten by every suite in turn. The runner therefore sets
        SEAMLY_TEST_LOG_DIR, which qttestmainlambda.cpp honors by writing a
        per-suite plain-text log to <build>\test-logs\<Suite>.txt; the
        script aggregates those logs into a summary and prints any FAIL!
        details in full.

    The script's exit code is the test executable's exit code (QTest failure
    status OR-ed across suites; 0 = all passed).

.PARAMETER Release
    Run the release-built suite from build\ instead of the debug suite from
    scripts\seamly2d-build-debug\.

.PARAMETER TestArgs
    Any remaining arguments are forwarded to Seamly2DTests.exe (standard
    QTest options, e.g. -functions, or a test function name filter).

.EXAMPLE
    .\scripts\st.ps1
    Run the debug test suite with per-suite logs and a summary.

.EXAMPLE
    .\scripts\st.ps1 -Release
    Run the release-built suite from the build\ tree.

.NOTES
    "st" = seamly2d tests, following the sd.ps1 ("seamly2d debug") naming.
#>

param(
    # When set, use the release build\ tree instead of scripts\seamly2d-build-debug\.
    [switch]$Release,

    # Extra arguments forwarded verbatim to Seamly2DTests.exe.
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$TestArgs = @()
)

$ErrorActionPreference = 'Stop'

# --- Resolve build tree and executables --------------------------------------
# The script lives in <repo-root>\scripts\, so the repo root is its parent.
$repoRoot = Split-Path -Parent $PSScriptRoot
if ($Release) {
    $buildDir = Join-Path $repoRoot 'build'
} else {
    $buildDir = Join-Path $PSScriptRoot 'seamly2d-build-debug'
}

$testBin = Join-Path $buildDir 'src\test\Seamly2DTest\bin'
$testExe = Join-Path $testBin 'Seamly2DTests.exe'
$appBin  = Join-Path $buildDir 'src\app\seamly2d\bin'

if (-not (Test-Path $testExe)) {
    throw "Test executable not found at '$testExe' - build first with scripts\sd.ps1$(if ($Release) { ' (release: qmake+jom in build\)' })."
}

# --- Locate the Qt platform plugin -------------------------------------------
# Qt only searches for platforms\qwindows[d].dll relative to the executable
# (or QT_PLUGIN_PATH). Prefer the plugins deployed beside the test exe by the
# Seamly2DTest.pro windeployqt step; fall back to the seamly2d.exe deployment
# for build trees that predate that step.
$pluginDir = $null
foreach ($candidate in @($testBin, $appBin)) {
    if (Test-Path (Join-Path $candidate 'platforms')) {
        $pluginDir = $candidate
        break
    }
}
if ($null -eq $pluginDir) {
    throw "No Qt 'platforms' plugin directory found in '$testBin' or '$appBin' - rebuild with scripts\sd.ps1 so windeployqt deploys the Qt runtime."
}

# --- Per-suite log directory --------------------------------------------------
# qttestmainlambda.cpp reads SEAMLY_TEST_LOG_DIR and writes one QTest text log
# per suite there; start each run from a clean slate so stale logs from a
# previous run can't mix into the summary.
$logDir = Join-Path $buildDir 'test-logs'
if (Test-Path $logDir) {
    Remove-Item -Path (Join-Path $logDir '*.txt') -Force -Confirm:$false
} else {
    New-Item -ItemType Directory -Path $logDir | Out-Null
}

Write-Host "exe     : $testExe"
Write-Host "plugins : $pluginDir"
Write-Host "logs    : $logDir"
Write-Host ''

# --- Run the suite with a scoped environment ---------------------------------
# PowerShell scripts share the caller's process, so save and restore the
# environment variables we touch instead of leaking them into the shell.
$savedPath      = $env:PATH
$savedPluginEnv = $env:QT_PLUGIN_PATH
$savedLogEnv    = $env:SEAMLY_TEST_LOG_DIR
try {
    # Test-exe bin first (Qt DLLs + xerces beside the exe), app bin as the
    # fallback DLL source for older build trees.
    $env:PATH                = "$testBin;$appBin;$env:PATH"
    $env:QT_PLUGIN_PATH      = $pluginDir
    $env:SEAMLY_TEST_LOG_DIR = $logDir

    & $testExe @TestArgs
    $suiteExit = $LASTEXITCODE
}
finally {
    $env:PATH                = $savedPath
    $env:QT_PLUGIN_PATH      = $savedPluginEnv
    $env:SEAMLY_TEST_LOG_DIR = $savedLogEnv
}

# --- Aggregate the per-suite logs into a summary ------------------------------
# Each log ends with a QTest totals line: "Totals: N passed, N failed, ...".
$logs = @(Get-ChildItem -Path $logDir -Filter '*.txt' | Sort-Object Name)
if ($logs.Count -eq 0) {
    Write-Host "WARNING: no per-suite logs were written to '$logDir' - the executable may have failed before running any suite."
} else {
    $totalPassed = 0
    $totalFailed = 0
    Write-Host ('-' * 60)
    foreach ($log in $logs) {
        $suite  = [IO.Path]::GetFileNameWithoutExtension($log.Name)
        $totals = Select-String -Path $log.FullName -Pattern '^Totals:' | Select-Object -First 1
        if ($null -eq $totals) {
            Write-Host ("{0,-28} NO TOTALS LINE (suite crashed?)" -f $suite)
            continue
        }
        # Pull the passed/failed counts out of the totals line.
        if ($totals.Line -match '(\d+) passed, (\d+) failed') {
            $passed = [int]$Matches[1]
            $failed = [int]$Matches[2]
            $totalPassed += $passed
            $totalFailed += $failed
            $mark = if ($failed -gt 0) { 'FAIL' } else { 'ok  ' }
            Write-Host ("{0,-28} {1}  {2}" -f $suite, $mark, $totals.Line)
        } else {
            Write-Host ("{0,-28} ????  {1}" -f $suite, $totals.Line)
        }
    }
    Write-Host ('-' * 60)
    Write-Host "TOTAL: $totalPassed passed, $totalFailed failed across $($logs.Count) suites (exit code $suiteExit)"

    # Print every FAIL! block in full so the cause is visible without opening
    # the individual log files.
    $failLines = Select-String -Path (Join-Path $logDir '*.txt') -Pattern '^FAIL!' -Context 0, 3
    if ($failLines) {
        Write-Host ''
        Write-Host 'Failure details:'
        foreach ($f in $failLines) {
            Write-Host ("  [{0}] {1}" -f [IO.Path]::GetFileNameWithoutExtension($f.Filename), $f.Line)
            foreach ($ctx in $f.Context.PostContext) {
                Write-Host ("      {0}" -f $ctx)
            }
        }
    }
}

exit $suiteExit
