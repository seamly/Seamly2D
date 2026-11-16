# @file   test_cli_layout_export.ps1
# @author slspencer
# @brief  Runs the Seamly2D command-line layout export test cases and writes a JSON report.
# @copyright Copyright (C) 2026 Seamly2D Project
# @license GPL-3.0-or-later
#
# Usage:
#   powershell -NoProfile -File src\test\CliTest\test_cli_layout_export.ps1 -FixtureRoot <test-seamly-layout>
#       [-Seamly2D <seamly2d.exe>] [-SeamlyLayout <seamlylayout.exe>] [-QtBin <Qt bin>]
#       [-OutputRoot <folder>] [-Tester <cli_test_app.exe>] [-Filter <name wildcard>] [-SkipSlow]
#
# Workflow:
#   1. Find the programs, the Qt runtime and the fixture files; stop with exit 2 when one is missing.
#   2. Make an isolated data folder (SEAMLY_DATA_DIR) whose Seamly2D settings name the SeamlyLayout program.
#   3. Run each case: build the argument list, run the program, capture streams and exit code.
#   4. Check the exit code, the streams, the output file and its structure (test_cli_output_checks.ps1).
#   5. Write <OutputRoot>\cli_test_report.json and print one line per case. Exit 1 when a case fails.
#
# Generated files stay under -OutputRoot; the fixture folder is only read.

param(
    [string]$FixtureRoot = $env:SEAMLY_TEST_FIXTURES,
    [string]$Seamly2D,
    [string]$SeamlyLayout,
    [string]$QtBin,
    [string]$OutputRoot,
    [string]$Tester,
    [string]$Filter = '*',
    [switch]$SkipSlow
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'test_cli_output_checks.ps1')
. (Join-Path $PSScriptRoot 'test_cli_case_list.ps1')

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
if (-not $Seamly2D) { $Seamly2D = Join-Path $repo 'src\app\seamly2d\bin\seamly2d.exe' }
if (-not $SeamlyLayout) { $SeamlyLayout = Join-Path $repo 'src\app\seamlylayout\qt_frontend\build\Release\seamlylayout.exe' }
if (-not $Tester) {
    # The Rust/Iced tester is optional: its cases are skipped when it is not built.
    $Tester = @('release', 'debug') | ForEach-Object { Join-Path $repo "src\test\CliTestApp\target\$_\cli_test_app.exe" } |
        Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}
if (-not $QtBin) {
    $qmake = if ($env:QMAKE) { $env:QMAKE } else { (Get-Command qmake -ErrorAction SilentlyContinue).Source }
    if ($qmake) { $QtBin = Split-Path -Parent $qmake }
}
if (-not $OutputRoot) { $OutputRoot = Join-Path $repo ('build\cli-tests\' + (Get-Date -Format 'yyMMddHHmm')) }

# --- 1. Inputs ----------------------------------------------------------------------------------------------------
$missing = @()
foreach ($item in @(@('Seamly2D', $Seamly2D), @('SeamlyLayout', $SeamlyLayout), @('FixtureRoot', $FixtureRoot), @('QtBin', $QtBin))) {
    if (-not $item[1] -or -not (Test-Path -LiteralPath $item[1])) { $missing += "$($item[0]): '$($item[1])'" }
}
if ($missing) {
    Write-Host "Missing inputs:`n  $($missing -join "`n  ")"
    Write-Host 'Give -FixtureRoot (the test-seamly-layout folder) and -QtBin (the Qt bin folder, or set QMAKE).'
    exit 2
}
$FixtureRoot = (Resolve-Path -LiteralPath $FixtureRoot).Path
foreach ($fixture in @('patterns\male_shirt.sm2d', 'patterns\trousers.sm2d', 'patterns\jacket2_40-146.sm2d',
                       'measurements\individual\male_shirt.smis', 'measurements\individual\trousers.smis')) {
    if (-not (Test-Path -LiteralPath (Join-Path $FixtureRoot $fixture))) { Write-Host "Missing fixture: $fixture"; exit 2 }
}
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
$OutputRoot = (Resolve-Path -LiteralPath $OutputRoot).Path
$pluginPath = (& (Join-Path $QtBin 'qmake.exe') -query QT_INSTALL_PLUGINS).Trim()

# --- 2. Isolated data folders -------------------------------------------------------------------------------------
# @brief Make a data folder whose Seamly2D settings point paths/seamlyLayoutApp at $LayoutApp.
function New-DataFolder([string]$Folder, [string]$LayoutApp)
{
    $settingsFolder = Join-Path $Folder 'settings\Seamly\Seamly2D'
    New-Item -ItemType Directory -Force -Path $settingsFolder | Out-Null
    Set-Content -LiteralPath (Join-Path $settingsFolder 'qt6_seamly2d.ini') -Encoding UTF8 `
        -Value "[paths]`r`nseamlyLayoutApp=$($LayoutApp -replace '\\', '/')`r`n"
    return $Folder
}
$dataFolder = New-DataFolder (Join-Path $OutputRoot 'data') $SeamlyLayout
$notAProgram = Join-Path $OutputRoot 'not_a_program.txt'
Set-Content -LiteralPath $notAProgram -Value 'not a program'
$badDataFolder = New-DataFolder (Join-Path $OutputRoot 'data-bad-layout-app') $notAProgram

$baseEnvironment = @{
    PATH            = "$QtBin;$env:PATH"
    QT_PLUGIN_PATH  = $pluginPath
    SEAMLY_DATA_DIR = $dataFolder
}

# @brief Replace the path tokens of one argument.
function Expand-Token([string]$Text, [string]$CaseFolder, [string]$OutputFile)
{
    return $Text.Replace('{fixtures}', $FixtureRoot).Replace('{settings}', (Join-Path $PSScriptRoot 'settings')).
        Replace('{outdir}', $CaseFolder).Replace('{out}', $OutputFile).Replace('{repo}', $repo)
}

# @brief Short text for the report: at most 4000 characters.
function Limit-Text([string]$Text) { if ($Text.Length -gt 4000) { $Text.Substring(0, 4000) + '...' } else { $Text } }

# --- 3. and 4. Run and check each case ----------------------------------------------------------------------------
$extensions = @{ 'pdf-tiled' = 'pdf' }
$oldOutputText = 'old output from an earlier run'
$results = New-Object System.Collections.Generic.List[object]
$outputs = @{}
$cases = @(Get-CliTestCases | Where-Object { $_.Name -like $Filter -and -not ($SkipSlow -and $_.PSObject.Properties['Slow'] -and $_.Slow) })
foreach ($case in $cases) {
    $property = { param($name, $default) if ($case.PSObject.Properties[$name]) { $case.$name } else { $default } }
    $caseFolder = Join-Path $OutputRoot $case.Name
    New-Item -ItemType Directory -Force -Path $caseFolder | Out-Null
    $format = & $property 'Format' $null
    $extension = if ($format -and $extensions.ContainsKey($format)) { $extensions[$format] } elseif ($format) { $format } else { 'svg' }
    $outputFile = Join-Path $caseFolder "$($case.Name).$extension"
    $arguments = @($case.Arguments | ForEach-Object { Expand-Token $_ $caseFolder $outputFile })
    $workingDirectory = Expand-Token (& $property 'WorkingDirectory' $caseFolder) $caseFolder $outputFile
    $environment = $baseEnvironment.Clone()
    if ((& $property 'LayoutApp' '') -eq 'missing') { $environment.SEAMLY_DATA_DIR = $badDataFolder }
    $extraEnvironment = & $property 'Environment' @{}
    foreach ($name in $extraEnvironment.Keys) { $environment[$name] = Expand-Token $extraEnvironment[$name] $caseFolder $outputFile }
    if (& $property 'OldOutput' $false) { Set-Content -LiteralPath $outputFile -Value $oldOutputText }

    $failures = New-Object System.Collections.Generic.List[string]
    $program = & $property 'Program' 'seamly2d'
    if ($program -eq 'tester' -and -not $Tester) {
        $results.Add([pscustomobject]@{ name = $case.Name; group = $case.Group; status = 'skip'; program = $program
            command = ''; arguments = $arguments; workingDirectory = $workingDirectory; exitCode = $null
            expectedExit = $case.Exit; seconds = 0; stdout = ''; stderr = 'cli_test_app is not built'
            output = $null; failures = @() })
        Write-Host ('{0,-4} {1,-44} (cli_test_app is not built)' -f 'SKIP', $case.Name)
        continue
    }
    switch ($program) {
        'seamlylayout' {
            $run = Invoke-CliProcess -Executable $SeamlyLayout -Arguments $arguments -WorkingDirectory $workingDirectory -Environment $environment
        }
        'tester' {
            $environment.SEAMLY2D_EXE = $Seamly2D
            $run = Invoke-CliProcess -Executable $Tester -Arguments $arguments -WorkingDirectory $workingDirectory -Environment $environment
            # One ZIP in the case folder holding one file per material, and no ZIP after a failure.
            $zips = @(Get-ChildItem -LiteralPath $caseFolder -Filter '*.zip')
            $zipEntries = & $property 'ZipEntries' 0
            if ($zipEntries -gt 0) {
                if ($zips.Count -ne 1) { $failures.Add("expected one ZIP; found $($zips.Count)") }
                else {
                    Add-Type -AssemblyName System.IO.Compression.FileSystem
                    $archive = [System.IO.Compression.ZipFile]::OpenRead($zips[0].FullName)
                    $names = @($archive.Entries | ForEach-Object { $_.FullName })
                    $archive.Dispose()
                    if ($names.Count -ne $zipEntries) { $failures.Add("ZIP holds $($names.Count) entries; expected $zipEntries") }
                    foreach ($name in $names) {
                        if ($name -notmatch '^[^_]+(_[^_]+)*_\d{10}\.[a-z]+$') { $failures.Add("ZIP entry '$name' does not follow <basename>_<material>_<YYMMDDHHMM>.<ext>") }
                        if (-not (Test-Path -LiteralPath (Join-Path $caseFolder $name))) { $failures.Add("ZIP entry '$name' has no file beside it") }
                    }
                }
            } elseif ($zips.Count -gt 0) { $failures.Add('a ZIP was made although a run failed') }
        }
        'console' {
            # A hidden console of its own: Seamly2D must attach to it, no redirection involved.
            $argumentFile = Join-Path $caseFolder 'arguments.json'
            $resultFile = Join-Path $caseFolder 'console.json'
            ConvertTo-Json -InputObject @($arguments) | Set-Content -LiteralPath $argumentFile -Encoding UTF8
            foreach ($name in $environment.Keys) { Set-Item -Path "env:$name" -Value $environment[$name] }
            $clock = [System.Diagnostics.Stopwatch]::StartNew()
            Start-Process -FilePath 'powershell.exe' -WindowStyle Hidden -Wait -WorkingDirectory $workingDirectory -ArgumentList @(
                '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Format-WindowsArgument (Join-Path $PSScriptRoot 'test_cli_console_attach.ps1')),
                '-Executable', (Format-WindowsArgument $Seamly2D), '-ArgumentFile', (Format-WindowsArgument $argumentFile),
                '-ResultFile', (Format-WindowsArgument $resultFile))
            $console = if (Test-Path -LiteralPath $resultFile) { Get-Content -LiteralPath $resultFile -Raw | ConvertFrom-Json } else { $null }
            $run = [pscustomobject]@{
                ExitCode = if ($console) { [int]$console.ExitCode } else { -1 }
                Stdout = if ($console) { [string]$console.Console } else { '' }
                Stderr = ''; Seconds = [math]::Round($clock.Elapsed.TotalSeconds, 2); TimedOut = $false
                CommandLine = (Format-WindowsArgument $Seamly2D) + ' ' + (($arguments | ForEach-Object { Format-WindowsArgument $_ }) -join ' ')
            }
        }
        'gui' {
            # Normal start: no CLI option, so Seamly2D must keep running its event loop and print nothing.
            $environment.QT_QPA_PLATFORM = 'offscreen'
            $guiInfo = New-Object System.Diagnostics.ProcessStartInfo
            $guiInfo.FileName = $Seamly2D
            $guiInfo.Arguments = ($arguments | ForEach-Object { Format-WindowsArgument $_ }) -join ' '
            $guiInfo.UseShellExecute = $false
            $guiInfo.RedirectStandardOutput = $true
            foreach ($name in $environment.Keys) { $guiInfo.EnvironmentVariables[$name] = [string]$environment[$name] }
            $guiProcess = [System.Diagnostics.Process]::Start($guiInfo)
            $stdoutTask = $guiProcess.StandardOutput.ReadToEndAsync()
            $exited = $guiProcess.WaitForExit(10000)
            if (-not $exited) { $guiProcess.Kill(); $guiProcess.WaitForExit() }
            $run = [pscustomobject]@{
                ExitCode = if ($exited) { $guiProcess.ExitCode } else { 0 }
                Stdout = $stdoutTask.Result; Stderr = ''; Seconds = 10; TimedOut = $false
                CommandLine = (Format-WindowsArgument $Seamly2D) + ' ' + $guiInfo.Arguments
            }
            if ($exited) { $failures.Add("Seamly2D exited after start (code $($guiProcess.ExitCode)); the GUI event loop must keep running") }
        }
        default {
            $run = Invoke-CliProcess -Executable $Seamly2D -Arguments $arguments -WorkingDirectory $workingDirectory -Environment $environment
        }
    }

    if ($run.TimedOut) { $failures.Add('timed out') }
    if ($run.ExitCode -ne $case.Exit) { $failures.Add("exit code $($run.ExitCode); expected $($case.Exit)") }
    $stdoutPattern = & $property 'Stdout' $null
    if ($stdoutPattern -and $run.Stdout -notmatch $stdoutPattern) { $failures.Add("stdout does not match /$stdoutPattern/") }
    $stderrPattern = & $property 'Stderr' $null
    if ($stderrPattern -and $run.Stderr -notmatch $stderrPattern) { $failures.Add("stderr does not match /$stderrPattern/") }
    if ($program -eq 'gui' -and $run.Stdout -match 'Usage:|SeamlyLayout:') { $failures.Add('GUI start printed command-line output') }
    if ($case.Exit -ne 0 -and $program -eq 'seamly2d' -and -not ($run.Stderr.Trim())) { $failures.Add('failure without a message on stderr') }

    $outputExists = Test-Path -LiteralPath $outputFile
    $outputBytes = if ($outputExists) { (Get-Item -LiteralPath $outputFile).Length } else { 0 }
    if ((& $property 'NoOutput' $false) -and $outputExists) { $failures.Add("a file stays at the output path after a failed run ($outputBytes bytes)") }
    if ((& $property 'OldOutput' $false) -and $case.Exit -ne 0) {
        $kept = $outputExists -and ((Get-Content -LiteralPath $outputFile -Raw).Trim() -eq $oldOutputText)
        if (-not $kept) { $failures.Add('the failed run changed or removed the existing output file') }
    }
    if ($format -and $case.Exit -eq 0) {
        if (-not $outputExists -or $outputBytes -eq 0) {
            $failures.Add('no output file')
        } else {
            foreach ($problem in @(Test-ExportFile $outputFile $format (& $property 'Expect' @{}))) { if ($problem) { $failures.Add($problem) } }
            $outputs[$case.Name] = $outputFile
            $other = & $property 'DiffersFrom' $null
            if ($other -and $outputs.ContainsKey($other) -and
                (Get-FileHash -LiteralPath $outputFile).Hash -eq (Get-FileHash -LiteralPath $outputs[$other]).Hash) {
                $failures.Add("output is identical to $other; the option had no effect")
            }
        }
    }
    $info = & $property 'Info' $null
    if ($info -and $run.ExitCode -eq 0) { foreach ($problem in @(Test-InfoOutput $run.Stdout $info)) { if ($problem) { $failures.Add($problem) } } }
    $handoff = & $property 'Handoff' $null
    if ($handoff) {
        $handoffFile = Join-Path $caseFolder 'handoff.svg'
        if (-not (Test-Path -LiteralPath $handoffFile)) { $failures.Add('no handoff copy was written') }
        else {
            $handoffText = Get-Content -LiteralPath $handoffFile -Raw -Encoding UTF8
            $handoffBytes = (Get-Item -LiteralPath $handoffFile).Length
            if ($handoffBytes -lt $handoff.MinBytes) { $failures.Add("handoff is $handoffBytes bytes; expected at least $($handoff.MinBytes)") }
            if ($handoffText.TrimEnd() -notmatch '</svg>$') { $failures.Add('handoff does not end with </svg>: truncated') }
            if ($handoffText -notmatch [regex]::Escape($handoff.Contains)) { $failures.Add("handoff lacks '$($handoff.Contains)'") }
            try { [xml]$handoffText | Out-Null } catch { $failures.Add('handoff is not well-formed XML') }
        }
    }

    $status = if ($failures.Count -eq 0) { 'pass' } else { 'fail' }
    $results.Add([pscustomobject]@{
        name = $case.Name; group = $case.Group; status = $status; program = $program
        command = $run.CommandLine; arguments = $arguments; workingDirectory = $workingDirectory
        exitCode = $run.ExitCode; expectedExit = $case.Exit; seconds = $run.Seconds
        stdout = (Limit-Text $run.Stdout); stderr = (Limit-Text $run.Stderr)
        output = [pscustomobject]@{ path = $outputFile; exists = $outputExists; bytes = $outputBytes }
        failures = @($failures)
    })
    $line = '{0,-4} {1,-44} exit {2,3} {3,6}s' -f $status.ToUpper(), $case.Name, $run.ExitCode, $run.Seconds
    Write-Host $line
    foreach ($failure in $failures) { Write-Host "       - $failure" }
}

# --- 5. Report ----------------------------------------------------------------------------------------------------
$failed = @($results | Where-Object { $_.status -eq 'fail' }).Count
$skipped = @($results | Where-Object { $_.status -eq 'skip' }).Count
$passed = $results.Count - $failed - $skipped
$report = [pscustomobject]@{
    created = (Get-Date).ToString('s'); seamly2d = $Seamly2D; seamlyLayout = $SeamlyLayout; tester = $Tester
    fixtureRoot = $FixtureRoot; qtBin = $QtBin; total = $results.Count; passed = $passed; failed = $failed
    skipped = $skipped; cases = $results
}
$reportFile = Join-Path $OutputRoot 'cli_test_report.json'
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $reportFile -Encoding UTF8
Write-Host "Cases: $($results.Count)  passed: $passed  failed: $failed  skipped: $skipped"
Write-Host "Report: $reportFile"
if ($failed -gt 0) { exit 1 }
exit 0
