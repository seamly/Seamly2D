# @file   test_cli_output_checks.ps1
# @author slspencer
# @brief  Process runner and export file checks for the Seamly2D command-line tests.
# @copyright Copyright (C) 2026 Seamly2D Project
# @license GPL-3.0-or-later
#
# Dot-source this file. Each Test-*Output function returns a list of failure
# sentences; an empty list means the file passed every check it was given.
# The checks read file structure and size only; they do not render a file.

Set-StrictMode -Version 3.0

$script:Backslash = [char]92

# @brief Quote one argument for the Windows command line (CommandLineToArgvW rules).
# @param Argument Argument text; may hold spaces, quotes and backslashes.
# @return The argument as the target process will read it back.
function Format-WindowsArgument([string]$Argument)
{
    if ($Argument -ne '' -and $Argument -notmatch '[\s"]') { return $Argument }
    $text = New-Object System.Text.StringBuilder
    [void]$text.Append('"')
    $backslashes = 0
    foreach ($character in $Argument.ToCharArray()) {
        if ($character -eq $script:Backslash) { $backslashes++; continue }
        if ($character -eq [char]'"') {
            [void]$text.Append([string]$script:Backslash * ($backslashes * 2 + 1)).Append('"')
            $backslashes = 0
            continue
        }
        [void]$text.Append([string]$script:Backslash * $backslashes).Append($character)
        $backslashes = 0
    }
    [void]$text.Append([string]$script:Backslash * ($backslashes * 2)).Append('"')
    return $text.ToString()
}

# @brief Run a program with an argument list, capture both streams and the exit code.
# @param Executable Absolute path of the program.
# @param Arguments  Argument list; each item is quoted separately, never joined unquoted.
# @param WorkingDirectory Directory the process starts in.
# @param TimeoutSeconds Kill the process after this time.
# @param Environment Extra environment variables for this run only.
# @return Object with ExitCode, Stdout, Stderr, Seconds, TimedOut, CommandLine.
function Invoke-CliProcess
{
    param(
        [Parameter(Mandatory = $true)][string]$Executable,
        [string[]]$Arguments = @(),
        [string]$WorkingDirectory = (Get-Location).Path,
        [int]$TimeoutSeconds = 600,
        [hashtable]$Environment = @{}
    )
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $Executable
    $info.Arguments = ($Arguments | ForEach-Object { Format-WindowsArgument $_ }) -join ' '
    $info.WorkingDirectory = $WorkingDirectory
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = [System.Text.Encoding]::UTF8
    $info.StandardErrorEncoding = [System.Text.Encoding]::UTF8
    foreach ($name in $Environment.Keys) { $info.EnvironmentVariables[$name] = [string]$Environment[$name] }

    $clock = [System.Diagnostics.Stopwatch]::StartNew()
    $process = [System.Diagnostics.Process]::Start($info)
    # Read both pipes at once: a full pipe would block the child.
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $timedOut = -not $process.WaitForExit($TimeoutSeconds * 1000)
    if ($timedOut) { try { $process.Kill() } catch { } }
    $process.WaitForExit()
    [pscustomobject]@{
        ExitCode    = if ($timedOut) { -1 } else { $process.ExitCode }
        Stdout      = $stdoutTask.Result
        Stderr      = $stderrTask.Result
        Seconds     = [math]::Round($clock.Elapsed.TotalSeconds, 2)
        TimedOut    = $timedOut
        CommandLine = (Format-WindowsArgument $Executable) + ' ' + $info.Arguments
    }
}

# @brief Compare a measured length with an expected one.
# @return A failure sentence, or $null when the values agree within the tolerance.
function Compare-Length([string]$What, [double]$Actual, [double]$Expected, [double]$Tolerance)
{
    if ([math]::Abs($Actual - $Expected) -le $Tolerance) { return $null }
    return ('{0} is {1:N3}; expected {2:N3} (tolerance {3})' -f $What, $Actual, $Expected, $Tolerance)
}

# @brief Check an SVG layout: XML, root size, piece groups and label text.
# @param Expect Keys: MinPieces, WidthIn, HasText (true: <text> present; false: none), ElementId, NoElementId.
function Test-SvgOutput([string]$Path, [hashtable]$Expect)
{
    $failures = New-Object System.Collections.Generic.List[string]
    try { [xml]$document = Get-Content -LiteralPath $Path -Raw -Encoding UTF8 }
    catch { $failures.Add("SVG is not well-formed XML: $($_.Exception.Message)"); return $failures }
    $root = $document.DocumentElement
    if ($root.LocalName -ne 'svg') { $failures.Add("root element is <$($root.LocalName)>, not <svg>") ; return $failures }
    $pieces = @($document.SelectNodes("//*[@data-type='piece']")).Count
    if ($Expect.ContainsKey('MinPieces') -and $pieces -lt $Expect.MinPieces) {
        $failures.Add("SVG has $pieces piece groups; expected at least $($Expect.MinPieces)")
    }
    if ($Expect.ContainsKey('WidthIn')) {
        # Layout SVGs give their size in CSS pixels, 96 per inch.
        $width = [double]($root.GetAttribute('width') -replace '[a-z]+$', '')
        $problem = Compare-Length 'SVG width (in)' ($width / 96.0) $Expect.WidthIn 0.05
        if ($problem) { $failures.Add($problem) }
    }
    if ($Expect.ContainsKey('HasText')) {
        $texts = @($document.GetElementsByTagName('text')).Count
        if ($Expect.HasText -and $texts -eq 0) { $failures.Add('SVG has no <text> labels; expected text') }
        if (-not $Expect.HasText -and $texts -gt 0) { $failures.Add("SVG has $texts <text> elements; expected paths only") }
    }
    if ($Expect.ContainsKey('ElementId')) {
        $found = @($document.SelectNodes("//*[@id='$($Expect.ElementId)']")).Count
        if ($found -eq 0) { $failures.Add("SVG has no element with id '$($Expect.ElementId)'") }
    }
    if ($Expect.ContainsKey('NoElementId')) {
        $found = @($document.SelectNodes("//*[@id='$($Expect.NoElementId)']")).Count
        if ($found -gt 0) { $failures.Add("SVG has an element with id '$($Expect.NoElementId)'") }
    }
    return $failures
}

# @brief Check a PDF: header, trailer, page count and page size (with /UserUnit).
# @param Expect Keys: Pages, MinPages, PageWidthIn, PageHeightIn, Orientation ('portrait' or 'landscape').
function Test-PdfOutput([string]$Path, [hashtable]$Expect)
{
    $failures = New-Object System.Collections.Generic.List[string]
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $text = [System.Text.Encoding]::GetEncoding(28591).GetString($bytes)
    if (-not $text.StartsWith('%PDF-')) { $failures.Add('missing %PDF- header'); return $failures }
    if ($text.LastIndexOf('%%EOF') -lt $text.Length - 1024) { $failures.Add('missing %%EOF trailer at the end') }
    $pages = [regex]::Matches($text, '/Type\s*/Page[^s]').Count
    if ($Expect.ContainsKey('Pages') -and $pages -ne $Expect.Pages) { $failures.Add("PDF has $pages pages; expected $($Expect.Pages)") }
    if ($Expect.ContainsKey('MinPages') -and $pages -lt $Expect.MinPages) { $failures.Add("PDF has $pages pages; expected at least $($Expect.MinPages)") }
    $box = [regex]::Match($text, '/MediaBox\s*\[\s*([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)\s*\]')
    if (-not $box.Success) { $failures.Add('no /MediaBox'); return $failures }
    $unit = [regex]::Match($text, '/UserUnit\s+([\d.]+)')
    $scale = if ($unit.Success) { [double]$unit.Groups[1].Value } else { 1.0 }
    $widthIn = ([double]$box.Groups[3].Value - [double]$box.Groups[1].Value) * $scale / 72.0
    $heightIn = ([double]$box.Groups[4].Value - [double]$box.Groups[2].Value) * $scale / 72.0
    if ($Expect.ContainsKey('PageWidthIn')) { $p = Compare-Length 'PDF page width (in)' $widthIn $Expect.PageWidthIn 0.02; if ($p) { $failures.Add($p) } }
    if ($Expect.ContainsKey('PageHeightIn')) { $p = Compare-Length 'PDF page height (in)' $heightIn $Expect.PageHeightIn 0.02; if ($p) { $failures.Add($p) } }
    if ($Expect.ContainsKey('Orientation')) {
        $actual = if ($widthIn -gt $heightIn) { 'landscape' } else { 'portrait' }
        if ($actual -ne $Expect.Orientation) { $failures.Add("PDF pages are $actual; expected $($Expect.Orientation)") }
    }
    $sizes = [regex]::Matches($text, '/MediaBox\s*\[([^\]]*)\]') | ForEach-Object { ($_.Groups[1].Value -split '\s+' | Where-Object { $_ }) -join ' ' } | Sort-Object -Unique
    if ($Expect.ContainsKey('SamePageSize') -and @($sizes).Count -gt 1) { $failures.Add("PDF pages differ in size: $($sizes -join '; ')") }
    return $failures
}

# @brief Check a DXF: sections, end marker, AutoCAD version and piece blocks.
# @param Expect Keys: Version ('AC1012' R13, 'AC1009' R11), MinBlocks, Companion (extension of a file beside it).
function Test-DxfOutput([string]$Path, [hashtable]$Expect)
{
    $failures = New-Object System.Collections.Generic.List[string]
    $lines = [System.IO.File]::ReadAllLines($Path) | ForEach-Object { $_.Trim() }
    $code0 = for ($i = 0; $i -lt $lines.Count - 1; $i += 2) { if ($lines[$i] -eq '0') { $lines[$i + 1] } }
    foreach ($section in @('HEADER', 'ENTITIES')) {
        if (-not ($lines -contains $section)) { $failures.Add("DXF has no $section section") }
    }
    if ($code0[-1] -ne 'EOF') { $failures.Add('DXF does not end with 0/EOF') }
    $versionIndex = [array]::IndexOf($lines, '$ACADVER')
    $version = if ($versionIndex -ge 0) { $lines[$versionIndex + 2] } else { '' }
    if ($Expect.ContainsKey('Version') -and $version -ne $Expect.Version) { $failures.Add("DXF version is '$version'; expected $($Expect.Version)") }
    $blocks = @($code0 | Where-Object { $_ -eq 'BLOCK' }).Count
    if ($Expect.ContainsKey('MinBlocks') -and $blocks -lt $Expect.MinBlocks) { $failures.Add("DXF has $blocks blocks; expected at least $($Expect.MinBlocks)") }
    if ($Expect.ContainsKey('Companion')) {
        $companion = [System.IO.Path]::ChangeExtension($Path, $Expect.Companion)
        if (-not (Test-Path -LiteralPath $companion)) { $failures.Add("no companion file $companion") }
    }
    return $failures
}

# @brief Read the width and height of a PNG from its IHDR chunk.
function Get-PngSize([string]$Path)
{
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $signature = [byte[]](0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A)
    for ($i = 0; $i -lt 8; $i++) { if ($bytes[$i] -ne $signature[$i]) { return $null } }
    # Big-endian 32-bit values; [int] first, because -shl on a [byte] stays in 8 bits.
    $width = ([int]$bytes[16] -shl 24) -bor ([int]$bytes[17] -shl 16) -bor ([int]$bytes[18] -shl 8) -bor [int]$bytes[19]
    $height = ([int]$bytes[20] -shl 24) -bor ([int]$bytes[21] -shl 16) -bor ([int]$bytes[22] -shl 8) -bor [int]$bytes[23]
    return [pscustomobject]@{ Width = $width; Height = $height }
}

# @brief Check a PNG: signature and pixel size.
# @param Expect Keys: WidthPx; ScaleOf = @{ Path = <PNG at scale 1>; Factor = n }.
function Test-PngOutput([string]$Path, [hashtable]$Expect)
{
    $failures = New-Object System.Collections.Generic.List[string]
    $size = Get-PngSize $Path
    if (-not $size) { $failures.Add('missing PNG signature'); return $failures }
    if ($Expect.ContainsKey('WidthPx')) { $p = Compare-Length 'PNG width (px)' $size.Width $Expect.WidthPx 2; if ($p) { $failures.Add($p) } }
    if ($Expect.ContainsKey('ScaleOf') -and (Test-Path -LiteralPath $Expect.ScaleOf.Path)) {
        $base = Get-PngSize $Expect.ScaleOf.Path
        $p = Compare-Length 'PNG width ratio' ($size.Width / [double]$base.Width) $Expect.ScaleOf.Factor 0.01
        if ($p) { $failures.Add($p) }
    }
    return $failures
}

# @brief Check a JPEG: start and end markers.
function Test-JpegOutput([string]$Path, [hashtable]$Expect)
{
    $failures = New-Object System.Collections.Generic.List[string]
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes[0] -ne 0xFF -or $bytes[1] -ne 0xD8) { $failures.Add('missing JPEG start marker FFD8') }
    if ($bytes[-2] -ne 0xFF -or $bytes[-1] -ne 0xD9) { $failures.Add('missing JPEG end marker FFD9') }
    return $failures
}

# @brief Check PostScript or EPS: header, bounding box and end.
# @param Expect Keys: Eps (true for EPSF header).
function Test-PostScriptOutput([string]$Path, [hashtable]$Expect)
{
    $failures = New-Object System.Collections.Generic.List[string]
    $text = [System.IO.File]::ReadAllText($Path, [System.Text.Encoding]::GetEncoding(28591))
    if (-not $text.StartsWith('%!PS-Adobe-3.0')) { $failures.Add('missing %!PS-Adobe-3.0 header') }
    $firstLine = ($text -split "`n", 2)[0]
    if ($Expect.Eps -and $firstLine -notmatch 'EPSF-3\.0') { $failures.Add('EPS header lacks EPSF-3.0') }
    if (-not $Expect.Eps -and $firstLine -match 'EPSF') { $failures.Add('PS header claims EPSF') }
    if ($text -notmatch '%%BoundingBox:\s*-?\d+\s+-?\d+\s+\d+\s+\d+') { $failures.Add('missing %%BoundingBox') }
    if ($text -notmatch '%%EOF') { $failures.Add('missing %%EOF') }
    return $failures
}

# @brief Check HP-GL: initialise command and pen moves.
function Test-HpglOutput([string]$Path, [hashtable]$Expect)
{
    $failures = New-Object System.Collections.Generic.List[string]
    $text = [System.IO.File]::ReadAllText($Path)
    if (-not $text.StartsWith('IN')) { $failures.Add('HP-GL does not start with IN') }
    if ($text -notmatch 'PD') { $failures.Add('HP-GL has no pen-down (PD) moves') }
    if ($text -notmatch 'SP\d') { $failures.Add('HP-GL selects no pen (SP)') }
    return $failures
}

# @brief Check the --layout-info JSON report on standard output.
# @param Expect Keys: Multisize, Materials (names in order), TiledPaper, ViewsInclude, OversizedMin, SplitViewsExcludeMarker.
function Test-InfoOutput([string]$Stdout, [hashtable]$Expect)
{
    $failures = New-Object System.Collections.Generic.List[string]
    try { $report = $Stdout.Trim() | ConvertFrom-Json }
    catch { $failures.Add("stdout is not one JSON document: $($_.Exception.Message)"); return $failures }
    if ($Expect.ContainsKey('Multisize') -and [bool]$report.multisize -ne $Expect.Multisize) { $failures.Add("multisize is $($report.multisize)") }
    if ($Expect.ContainsKey('TiledPaper') -and [bool]$report.tiledPaper -ne $Expect.TiledPaper) { $failures.Add("tiledPaper is $($report.tiledPaper)") }
    $names = @($report.materials | ForEach-Object { $_.name })
    if ($Expect.ContainsKey('Materials') -and (($names -join ',') -ne ($Expect.Materials -join ','))) { $failures.Add("materials are $($names -join ', ')") }
    $views = @($report.materials[0].whole | ForEach-Object { $_.view })
    foreach ($view in @(if ($Expect.ContainsKey('ViewsInclude')) { $Expect.ViewsInclude })) { if ($null -ne $view -and $views -notcontains $view) { $failures.Add("whole views lack '$view': $($views -join ', ')") } }
    if ($Expect.ContainsKey('OversizedMin')) {
        $count = @($report.materials | ForEach-Object { @($_.oversizedPieces) } | Where-Object { $_ }).Count
        if ($count -lt $Expect.OversizedMin) { $failures.Add("$count oversized pieces; expected at least $($Expect.OversizedMin)") }
    }
    if ($Expect.ContainsKey('SplitViewsExcludeMarker') -and $Expect.SplitViewsExcludeMarker) {
        $split = @($report.materials[0].split | ForEach-Object { $_.view })
        if ($split -contains 'Marker' -or $split -contains 'Nested') { $failures.Add("split views include Marker or Nested: $($split -join ', ')") }
        foreach ($layout in @($report.materials[0].split)) {
            if (@($layout.formats) -contains 'dxf') { $failures.Add("split view $($layout.view) allows dxf") }
        }
    }
    return $failures
}

# @brief Run the format check that matches the file extension.
function Test-ExportFile([string]$Path, [string]$Format, [hashtable]$Expect)
{
    switch ($Format) {
        'svg'       { return Test-SvgOutput $Path $Expect }
        'pdf'       { return Test-PdfOutput $Path $Expect }
        'pdf-tiled' { return Test-PdfOutput $Path $Expect }
        'dxf'       { return Test-DxfOutput $Path $Expect }
        'png'       { return Test-PngOutput $Path $Expect }
        'jpg'       { return Test-JpegOutput $Path $Expect }
        'jpeg'      { return Test-JpegOutput $Path $Expect }
        'ps'        { return Test-PostScriptOutput $Path @{ Eps = $false } }
        'eps'       { return Test-PostScriptOutput $Path @{ Eps = $true } }
        'hpgl'      { return Test-HpglOutput $Path $Expect }
        default     { return @("no file check for format '$Format'") }
    }
}
