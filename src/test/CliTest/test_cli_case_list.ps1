# @file   test_cli_case_list.ps1
# @author slspencer
# @brief  Test cases for the Seamly2D command-line layout export.
# @copyright Copyright (C) 2026 Seamly2D Project
# @license GPL-3.0-or-later
#
# Dot-source this file; Get-CliTestCases returns the cases in run order.
#
# Case keys:
#   Name        unique name; also the output file name
#   Group       option group of CLI_1_OPTION_MATRIX.md
#   Program     seamly2d (default), seamlylayout, console (console attach probe), gui (GUI start probe)
#               tester (cli_test_app --batch; skipped when not built) or installed (copies in the MSI folder layout)
#   ZipEntries  tester: number of files the session ZIP must hold
#   Arguments   argument list; tokens: {fixtures} {settings} {out} {outdir} {repo}
#   Exit        expected exit code
#   Format      export format; the output file is {out} = <case dir>\<Name>.<extension>
#   Expect      hashtable for the format check (test_cli_output_checks.ps1)
#   Stdout, Stderr          regular expressions that must match
#   StdoutEmpty            true: standard output must be empty
#   NoOutput    true: no file may exist at {out} after the run
#   OldOutput   true: write a file at {out} before the run; a failed run must keep it unchanged
#   Info        hashtable for Test-InfoOutput on standard output
#   WorkingDirectory       token path the process starts in (default: the case directory)
#   Environment            extra environment variables (tokens allowed)
#   LayoutApp   'missing': point paths/seamlyLayoutApp at a file that is not a program
#   DiffersFrom name of an earlier case whose output must differ (same format)
#   Slow        true: multisize case (about 15 s each)
#   Richmond    true: needs -RichmondRoot ({richmond}); skipped without it

Set-StrictMode -Version 3.0

function Get-CliTestCases
{
    $shirt = '{fixtures}\patterns\male_shirt.sm2d'
    $trousers = '{fixtures}\patterns\trousers.sm2d'
    $trousersMeasurements = '{fixtures}\measurements\individual\trousers.smis'
    $jacket = '{fixtures}\patterns\jacket2_40-146.sm2d'
    $roll36 = '{settings}\settings_paper_roll_36in.json'
    $roll12 = '{settings}\settings_paper_roll_12in_narrow.json'
    $letter = '{settings}\settings_paper_tiled_letter_portrait.json'
    $a4 = '{settings}\settings_paper_tiled_a4_landscape.json'
    $custom = '{settings}\settings_paper_tiled_custom_margins.json'
    $folded = '{settings}\settings_fabric_folded_with_nap.json'
    $openFabric = '{settings}\settings_fabric_open_margins.json'
    $export = @('--layout-export', '--layout-output', '{out}')

    @(
        # --- Help, version and console ------------------------------------------------------------------------------
        @{ Name = 'help'; Group = 'general'; Arguments = @('--help'); Exit = 0
           Stdout = '--layout-export(.|\n)*--layout-material(.|\n)*--layout-view(.|\n)*--layout-oversize(.|\n)*--layout-info' }
        @{ Name = 'version'; Group = 'general'; Arguments = @('--version'); Exit = 0; Stdout = '^Seamly2D \d+\.\d+\.\d+' }
        @{ Name = 'console-help-without-redirection'; Group = 'general'; Program = 'console'; Arguments = @('--help'); Exit = 0
           Stdout = 'Usage:(.|\n)*--layout-export' }
        @{ Name = 'console-error-without-redirection'; Group = 'general'; Program = 'console'
           Arguments = @('--basename', 'x', $shirt); Exit = 64; Stdout = "Unknown option 'basename'" }
        @{ Name = 'gui-start-unchanged'; Group = 'general'; Program = 'gui'; Arguments = @(); Exit = 0 }
        @{ Name = 'gui-start-with-pattern'; Group = 'general'; Program = 'gui'; Arguments = @($shirt); Exit = 0 }

        # --- Removed legacy options ----------------------------------------------------------------------------------
        @{ Name = 'legacy-basename'; Group = 'legacy'; Arguments = @('--basename', 'shirt', $shirt); Exit = 64
           Stderr = "Unknown option 'basename'" }
        @{ Name = 'legacy-exportOnlyDetails'; Group = 'legacy'; Arguments = @('--exportOnlyDetails', $shirt); Exit = 64
           Stderr = "Unknown option 'exportOnlyDetails'" }
        @{ Name = 'legacy-format'; Group = 'legacy'; Arguments = @('-f', '1', $shirt); Exit = 64; Stderr = "Unknown option 'f'" }

        # --- Test mode, inputs and paths -----------------------------------------------------------------------------
        @{ Name = 'test-mode'; Group = 'general'; Arguments = @('--test', $shirt); Exit = 0 }
        @{ Name = 'test-mode-relative-paths'; Group = 'paths'; WorkingDirectory = '{fixtures}'
           Arguments = @('--test', '--mfile', 'measurements\individual\male_shirt.smis', 'patterns\male_shirt.sm2d'); Exit = 0 }
        @{ Name = 'test-mode-two-files'; Group = 'general'; Arguments = @('--test', $shirt, $trousers); Exit = 64
           Stderr = 'single input file' }
        @{ Name = 'test-and-export'; Group = 'general'; Arguments = @('--test') + $export + @($shirt); Exit = 64
           Stderr = '--test cannot be used with --layout-export' }
        @{ Name = 'missing-pattern'; Group = 'paths'; Arguments = $export + @('{fixtures}\patterns\no_such_pattern.sm2d'); Exit = 66
           Stderr = 'no_such_pattern\.sm2d'; NoOutput = $true }
        @{ Name = 'missing-measurements'; Group = 'measurements'; Arguments = $export + @('--mfile', '{fixtures}\no_such.smis', $shirt)
           Exit = 66; Stderr = 'no_such\.smis'; NoOutput = $true }
        @{ Name = 'pattern-measurements-not-found'; Group = 'measurements'; Arguments = $export + @($trousers); Exit = 66
           Stderr = 'could not be found'; NoOutput = $true }

        # --- Required companions and invalid values ------------------------------------------------------------------
        @{ Name = 'export-without-output'; Group = 'export'; Arguments = @('--layout-export', $shirt); Exit = 64
           Stderr = 'requires --layout-output' }
        @{ Name = 'export-without-pattern'; Group = 'export'; Arguments = $export; Exit = 64; Stderr = 'requires one input pattern' }
        @{ Name = 'export-two-patterns'; Group = 'export'; Arguments = $export + @($shirt, $trousers); Exit = 64
           Stderr = 'requires one input pattern' }
        @{ Name = 'option-without-export'; Group = 'export'; Arguments = @('--layout-output', '{out}', $shirt); Exit = 64
           Stderr = 'require --layout-export' }
        @{ Name = 'unknown-format'; Group = 'export'; Arguments = $export + @('--layout-format', 'bmp', $shirt); Exit = 64
           Stderr = 'Unsupported SeamlyLayout export format: bmp' }
        @{ Name = 'text-mode-with-pdf'; Group = 'export'; Arguments = $export + @('--layout-format', 'pdf', '--svg-text-mode', 'hersheyStrokes', $shirt)
           Exit = 64; Stderr = 'Invalid --svg-text-mode' }
        @{ Name = 'unknown-text-mode'; Group = 'export'; Arguments = $export + @('--svg-text-mode', 'bold', $shirt); Exit = 64
           Stderr = 'Invalid --svg-text-mode' }
        @{ Name = 'legacy-layout-scale'; Group = 'legacy'; Arguments = $export + @('--layout-format', 'png', '--layout-scale', '2', $shirt)
           Exit = 64; Stderr = "Unknown option 'layout-scale'" }
        @{ Name = 'oversize-unknown-choice'; Group = 'oversize'; Arguments = $export + @('--layout-oversize', 'shrink', $shirt)
           Exit = 64; Stderr = 'must be split or extend' }
        @{ Name = 'view-with-gsize'; Group = 'multisize'; Arguments = $export + @('--layout-view', 'marker', '--gsize', '40', $jacket)
           Exit = 64; Stderr = '--layout-view selects' }
        @{ Name = 'info-with-format'; Group = 'info'; Arguments = @('--layout-info', '--layout-format', 'pdf', $shirt); Exit = 64
           Stderr = '--layout-info prints a report' }
        @{ Name = 'gsize-invalid-value'; Group = 'multisize'; Arguments = $export + @('--gsize', '999', $jacket); Exit = 64
           Stderr = 'Invalid gradation size value: 999' }
        @{ Name = 'gsize-individual-pattern'; Group = 'multisize'; Arguments = $export + @('--gsize', '40', $shirt); Exit = 65
           Stderr = 'Need a file with multisize measurements'; NoOutput = $true }
        @{ Name = 'settings-missing-file'; Group = 'settings'; Arguments = $export + @('--layout-settings', '{settings}\no_such.json', $shirt)
           Exit = 64; Stderr = 'cannot be read'; NoOutput = $true }
        @{ Name = 'settings-invalid-values'; Group = 'settings'
           Arguments = $export + @('--layout-settings', '{settings}\settings_invalid_values.json', $shirt)
           Exit = 64; Stderr = "marginTOP(.|\n)*tileOrientation"; NoOutput = $true }
        @{ Name = 'settings-invalid-json'; Group = 'settings'
           Arguments = $export + @('--layout-settings', '{settings}\settings_invalid_json.json', $shirt)
           Exit = 64; Stderr = 'not valid JSON'; NoOutput = $true }
        @{ Name = 'output-directory-missing'; Group = 'paths'; Arguments = @('--layout-export', '--layout-output', '{outdir}\no_such_dir\x.svg', $shirt)
           Exit = 64; Stderr = 'output directory does not exist' }
        @{ Name = 'layout-app-installed-folder'; Group = 'general'; Program = 'installed'; Format = 'svg'
           Arguments = $export + @('--layout-settings', $roll36, $shirt); Exit = 0; Expect = @{ MinPieces = 16; WidthIn = 36 } }
        @{ Name = 'layout-app-not-a-program'; Group = 'general'; LayoutApp = 'missing'; Arguments = $export + @($shirt)
           Exit = 69; Stderr = 'SeamlyLayout could not be started'; NoOutput = $true }

        # --- Individual pattern: formats -----------------------------------------------------------------------------
        @{ Name = 'shirt-svg-default-settings'; Group = 'formats'; Format = 'svg'; Arguments = $export + @($shirt); Exit = 0
           Expect = @{ MinPieces = 16; HasText = $true }; StdoutEmpty = $false; Stdout = 'SeamlyLayout: exported' }
        @{ Name = 'shirt-svg-designer-font'; Group = 'text'; Format = 'svg'
           Arguments = $export + @('--layout-format', 'svg', '--svg-text-mode', 'designerFont', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; HasText = $true; WidthIn = 36; NoElementId = 'scale_grid' } }
        @{ Name = 'shirt-svg-scale-grid'; Group = 'settings'; Format = 'svg'
           Arguments = $export + @('--layout-settings', '{settings}\settings_paper_roll_36in_scale_grid.json', $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; WidthIn = 36; ElementId = 'scale_grid' } }
        @{ Name = 'shirt-svg-single-line-font'; Group = 'text'; Format = 'svg'
           Arguments = $export + @('--svg-text-mode', 'singleLineFont', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; HasText = $true; WidthIn = 36 } }
        @{ Name = 'shirt-svg-hershey-strokes'; Group = 'text'; Format = 'svg'
           Arguments = $export + @('--svg-text-mode', 'hersheyStrokes', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; HasText = $false; WidthIn = 36 } }
        @{ Name = 'shirt-svg-as-supplied'; Group = 'text'; Format = 'svg'
           Arguments = $export + @('--svg-text-mode', 'asSupplied', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; WidthIn = 36 } }
        @{ Name = 'shirt-pdf-roll-36in'; Group = 'paper-roll'; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 36; HasFont = $true; MaxPageHeightIn = 400 } }
        @{ Name = 'shirt-pdf-text2paths'; Group = 'text'; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--text2paths', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 36; HasFont = $false } }
        @{ Name = 'shirt-pdf-tiled-text2paths'; Group = 'text'; Format = 'pdf-tiled'
           Arguments = $export + @('--layout-format', 'pdf-tiled', '--text2paths', '--layout-settings', $letter, $shirt)
           Exit = 0; Expect = @{ MinPages = 2; PageWidthIn = 8.5; PageHeightIn = 11; HasFont = $false } }
        @{ Name = 'shirt-svg-text2paths'; Group = 'text'; Format = 'svg'
           Arguments = $export + @('--text2paths', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; HasText = $false; WidthIn = 36 } }
        @{ Name = 'shirt-eps-text2paths'; Group = 'text'; Format = 'eps'
           Arguments = $export + @('--layout-format', 'eps', '--text2paths', '--layout-settings', $roll36, $shirt); Exit = 0; Expect = @{} }
        @{ Name = 'text2paths-with-dxf'; Group = 'text'; Arguments = $export + @('--layout-format', 'dxf', '--text2paths', $shirt)
           Exit = 64; Stderr = '--text2paths applies to' }
        @{ Name = 'text2paths-with-text-mode'; Group = 'text'; Arguments = $export + @('--text2paths', '--svg-text-mode', 'asSupplied', $shirt)
           Exit = 64; Stderr = '--text2paths and --svg-text-mode cannot be used together' }
        @{ Name = 'text2paths-without-export'; Group = 'text'; Arguments = @('--text2paths', $shirt)
           Exit = 64; Stderr = 'require --layout-export' }
        @{ Name = 'shirt-pdf-tiled-letter-portrait'; Group = 'paper-tiled'; Format = 'pdf-tiled'
           Arguments = $export + @('--layout-format', 'pdf-tiled', '--layout-settings', $letter, $shirt)
           Exit = 0; Expect = @{ MinPages = 2; PageWidthIn = 8.5; PageHeightIn = 11; SamePageSize = $true } }
        @{ Name = 'shirt-pdf-tiled-a4-landscape'; Group = 'paper-tiled'; Format = 'pdf-tiled'
           Arguments = $export + @('--layout-format', 'pdf-tiled', '--layout-settings', $a4, $shirt)
           Exit = 0; Expect = @{ MinPages = 2; PageWidthIn = 11.69; PageHeightIn = 8.27; Orientation = 'landscape' } }
        @{ Name = 'shirt-pdf-tiled-custom-margins'; Group = 'margins'; Format = 'pdf-tiled'
           Arguments = $export + @('--layout-format', 'pdf-tiled', '--layout-settings', $custom, $shirt)
           Exit = 0; Expect = @{ MinPages = 2; PageWidthIn = 10; PageHeightIn = 12 } }
        @{ Name = 'shirt-pdf-on-tiled-paper'; Group = 'paper-tiled'; Arguments = $export + @('--layout-format', 'pdf', '--layout-settings', $letter, $shirt)
           Exit = 64; Stderr = 'use pdf-tiled'; NoOutput = $true }
        @{ Name = 'shirt-pdf-tiled-on-roll'; Group = 'paper-roll'; Arguments = $export + @('--layout-format', 'pdf-tiled', '--layout-settings', $roll36, $shirt)
           Exit = 64; Stderr = 'pdf-tiled needs tiled paper'; NoOutput = $true }
        @{ Name = 'shirt-dxf-r13'; Group = 'formats'; Format = 'dxf'
           Arguments = $export + @('--layout-format', 'dxf', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ Version = 'AC1012'; MinBlocks = 16 }; StdoutEmpty = $false; Stdout = '^SeamlyLayout: exported [^\r\n]*\r?\n?$' }
        @{ Name = 'shirt-dxf-aama-r11'; Group = 'formats'; Format = 'dxf'
           Arguments = $export + @('--layout-format', 'dxf', '--layout-export-options', '{settings}\options_dxf_aama_r11.json', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ Version = 'AC1009'; MinBlocks = 16 } }
        @{ Name = 'shirt-dxf-clo3d'; Group = 'formats'; Format = 'dxf'
           Arguments = $export + @('--layout-format', 'dxf', '--layout-export-options', '{settings}\options_dxf_clo3d.json', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ Version = 'AC1012'; MinBlocks = 16 } }
        @{ Name = 'shirt-png-96-ppi'; Group = 'formats'; Format = 'png'
           Arguments = $export + @('--layout-format', 'png', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ WidthPx = 3456 } }
        @{ Name = 'shirt-jpg'; Group = 'formats'; Format = 'jpg'; Arguments = $export + @('--layout-format', 'jpg', '--layout-settings', $roll36, $shirt); Exit = 0; Expect = @{} }
        @{ Name = 'shirt-jpeg'; Group = 'formats'; Format = 'jpeg'; Arguments = $export + @('--layout-format', 'jpeg', '--layout-settings', $roll36, $shirt); Exit = 0; Expect = @{} }
        @{ Name = 'shirt-hpgl'; Group = 'formats'; Format = 'hpgl'; Arguments = $export + @('--layout-format', 'hpgl', '--layout-settings', $roll36, $shirt); Exit = 0; Expect = @{} }
        @{ Name = 'shirt-ps'; Group = 'formats'; Format = 'ps'; Arguments = $export + @('--layout-format', 'ps', '--layout-settings', $roll36, $shirt); Exit = 0; Expect = @{} }
        @{ Name = 'shirt-eps'; Group = 'formats'; Format = 'eps'; Arguments = $export + @('--layout-format', 'eps', '--layout-settings', $roll36, $shirt); Exit = 0; Expect = @{} }
        @{ Name = 'shirt-gcode-unavailable'; Group = 'formats'; Arguments = $export + @('--layout-format', 'gcode', '--layout-settings', $roll36, $shirt)
           Exit = 69; Stderr = 'not available in this build'; NoOutput = $true }
        @{ Name = 'shirt-3mf-unavailable'; Group = 'formats'; Arguments = $export + @('--layout-format', '3mf', '--layout-settings', $roll36, $shirt)
           Exit = 69; Stderr = 'not available in this build'; NoOutput = $true }

        # --- Fabric, nap, margins, material ------------------------------------------------------------------------
        @{ Name = 'shirt-fabric-folded-with-nap'; Group = 'fabric'; Format = 'svg'
           Arguments = $export + @('--layout-settings', $folded, $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; WidthIn = (75 / 2.54) } }
        @{ Name = 'shirt-fabric-open-margins'; Group = 'margins'; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--layout-settings', $openFabric, $shirt)
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 60 } }
        @{ Name = 'shirt-material-fabric'; Group = 'material'; Format = 'svg'
           Arguments = $export + @('--layout-material', 'fabric', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ MinPieces = 16 } }
        @{ Name = 'shirt-material-unknown'; Group = 'material'; Arguments = $export + @('--layout-material', 'Lining', $shirt)
           Exit = 64; Stderr = "no material 'Lining'"; NoOutput = $true }
        @{ Name = 'shirt-view-on-individual'; Group = 'multisize'; Arguments = $export + @('--layout-view', 'marker', $shirt)
           Exit = 64; Stderr = 'this layout has one view'; NoOutput = $true }

        # --- Oversized pieces -------------------------------------------------------------------------------------
        @{ Name = 'shirt-oversize-no-choice'; Group = 'oversize'; OldOutput = $true
           Arguments = $export + @('--layout-format', 'pdf', '--layout-settings', $roll12, $shirt)
           Exit = 70; Stderr = 'do not fit the media(.|\n)*--layout-oversize' }
        @{ Name = 'shirt-oversize-split-pdf'; Group = 'oversize'; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--layout-oversize', 'split', '--layout-settings', $roll12, $shirt)
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 12 } }
        @{ Name = 'shirt-oversize-split-svg'; Group = 'oversize'; Format = 'svg'
           Arguments = $export + @('--layout-oversize', 'split', '--layout-settings', $roll12, $shirt)
           Exit = 0; Expect = @{ MinPieces = 17; WidthIn = 12 } }
        @{ Name = 'shirt-oversize-split-dxf'; Group = 'oversize'
           Arguments = $export + @('--layout-format', 'dxf', '--layout-oversize', 'split', '--layout-settings', $roll12, $shirt)
           Exit = 64; Stderr = 'dxf does not export split layouts'; NoOutput = $true }
        @{ Name = 'shirt-oversize-extend-too-wide'; Group = 'oversize'
           Arguments = $export + @('--layout-format', 'dxf', '--layout-oversize', 'extend', '--layout-settings', $roll12, $shirt)
           Exit = 70; Stderr = 'could not be placed'; NoOutput = $true }
        @{ Name = 'shirt-old-output-replaced'; Group = 'paths'; Format = 'svg'; OldOutput = $true
           Arguments = $export + @('--layout-settings', $roll36, $shirt); Exit = 0; Expect = @{ MinPieces = 16 } }

        # --- Paths and measurement overrides ----------------------------------------------------------------------
        @{ Name = 'shirt-relative-paths'; Group = 'paths'; Format = 'svg'; WorkingDirectory = '{fixtures}'
           Arguments = @('--layout-export', '--layout-output', '{out}', '--mfile', 'measurements\individual\male_shirt.smis',
                         '--layout-settings', $roll36, 'patterns\male_shirt.sm2d')
           Exit = 0; Expect = @{ MinPieces = 16 } }
        @{ Name = 'shirt-relative-settings'; Group = 'paths'; Format = 'svg'; WorkingDirectory = '{settings}'
           Arguments = $export + @('--layout-settings', 'settings_paper_roll_36in.json', $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; WidthIn = 36 } }
        @{ Name = 'shirt-relative-output'; Group = 'paths'; Format = 'svg'
           Arguments = @('--layout-export', '--layout-output', 'shirt-relative-output.svg', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ MinPieces = 16 } }
        @{ Name = 'shirt-measurement-override'; Group = 'measurements'; Format = 'svg'
           Arguments = $export + @('--mfile', '{fixtures}\measurements\individual\male_shirt.smis', '--layout-settings', $roll36, $shirt)
           Exit = 0; Expect = @{ MinPieces = 16 }; DiffersFrom = 'shirt-svg-designer-font' }
        @{ Name = 'trousers-svg'; Group = 'measurements'; Format = 'svg'
           Arguments = $export + @('--mfile', $trousersMeasurements, '--layout-settings', $roll36, $trousers)
           Exit = 0; Expect = @{ MinPieces = 8; WidthIn = 36 } }
        @{ Name = 'trousers-pdf-tiled'; Group = 'paper-tiled'; Format = 'pdf-tiled'
           Arguments = $export + @('--layout-format', 'pdf-tiled', '--mfile', $trousersMeasurements, '--layout-settings', $letter, $trousers)
           Exit = 0; Expect = @{ MinPages = 2; PageWidthIn = 8.5; PageHeightIn = 11 } }
        @{ Name = 'trousers-dxf'; Group = 'formats'; Format = 'dxf'
           Arguments = $export + @('--layout-format', 'dxf', '--mfile', $trousersMeasurements, '--layout-settings', $roll36, $trousers)
           Exit = 0; Expect = @{ Version = 'AC1012'; MinBlocks = 8 } }

        # --- Layout info --------------------------------------------------------------------------------------------
        @{ Name = 'info-shirt-roll'; Group = 'info'; Arguments = @('--layout-info', '--layout-settings', $roll36, $shirt); Exit = 0
           Info = @{ Multisize = $false; Materials = @('Fabric'); TiledPaper = $false; ViewsInclude = @('') } }
        @{ Name = 'info-shirt-oversized'; Group = 'info'; Arguments = @('--layout-info', '--layout-settings', $roll12, $shirt); Exit = 0
           Info = @{ OversizedMin = 5 } }
        @{ Name = 'info-jacket-roll'; Group = 'info'; Slow = $true; Arguments = @('--layout-info', '--layout-settings', $roll36, $jacket); Exit = 0
           Info = @{ Multisize = $true; Materials = @('Fabric', 'Lining', 'Interfacing'); ViewsInclude = @('Marker', 'Nested', 'Base Size 500', 'Size 400'); SplitViewsExcludeMarker = $true } }

        # --- Multisize ---------------------------------------------------------------------------------------------
        @{ Name = 'jacket-no-material'; Group = 'material'; Slow = $true; Arguments = $export + @('--layout-settings', $roll36, $jacket)
           Exit = 64; Stderr = '3 materials: Fabric, Lining, Interfacing'; NoOutput = $true }
        @{ Name = 'jacket-fabric-marker-pdf'; Group = 'multisize'; Slow = $true; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--layout-material', 'Fabric', '--layout-view', 'marker', '--layout-settings', $roll36, $jacket)
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 36 } }
        @{ Name = 'jacket-fabric-nested-svg'; Group = 'multisize'; Slow = $true; Format = 'svg'
           Arguments = $export + @('--layout-material', 'Fabric', '--layout-view', 'Nested', '--layout-settings', $roll36, $jacket)
           Exit = 0; Expect = @{ WidthIn = 36; MinPieces = 8 } }
        @{ Name = 'jacket-fabric-base-dxf'; Group = 'multisize'; Slow = $true; Format = 'dxf'
           Arguments = $export + @('--layout-format', 'dxf', '--layout-material', 'Fabric', '--layout-view', 'base', '--layout-settings', $roll36, $jacket)
           Exit = 0; Expect = @{ Version = 'AC1012'; MinBlocks = 8; Companion = 'rul' } }
        @{ Name = 'jacket-lining-size-400-dxf'; Group = 'multisize'; Slow = $true; Format = 'dxf'
           Arguments = $export + @('--layout-format', 'dxf', '--layout-material', 'Lining', '--layout-view', '400', '--layout-settings', $roll36, $jacket)
           Exit = 0; Expect = @{ Version = 'AC1012'; MinBlocks = 5 } }
        @{ Name = 'jacket-marker-dxf'; Group = 'multisize'; Slow = $true
           Arguments = $export + @('--layout-format', 'dxf', '--layout-material', 'Fabric', '--layout-view', 'marker', '--layout-settings', $roll36, $jacket)
           Exit = 64; Stderr = 'dxf exports one size'; NoOutput = $true }
        @{ Name = 'jacket-unknown-view'; Group = 'multisize'; Slow = $true
           Arguments = $export + @('--layout-material', 'Fabric', '--layout-view', '999', '--layout-settings', $roll36, $jacket)
           Exit = 64; Stderr = "no view '999'(.|\n)*Marker, Nested"; NoOutput = $true }
        @{ Name = 'jacket-interfacing-tiled-default-view'; Group = 'paper-tiled'; Slow = $true; Format = 'pdf-tiled'
           Arguments = $export + @('--layout-format', 'pdf-tiled', '--layout-material', 'Interfacing', '--layout-settings', $letter, $jacket)
           Exit = 0; Expect = @{ MinPages = 1; PageWidthIn = 8.5; PageHeightIn = 11 } }
        @{ Name = 'jacket-fabric-folded-with-nap'; Group = 'fabric'; Slow = $true; Format = 'svg'
           Arguments = $export + @('--layout-material', 'Fabric', '--layout-view', 'Size 500', '--layout-settings', $folded, $jacket)
           Exit = 0; Expect = @{ MinPieces = 5; WidthIn = (75 / 2.54) } }
        @{ Name = 'jacket-gsize-gheight-one-size'; Group = 'multisize'; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--layout-material', 'Lining', '--gsize', '40', '--gheight', '146', '--layout-settings', $roll36, $jacket)
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 36 } }
        @{ Name = 'jacket-handoff-not-truncated'; Group = 'transport'; Slow = $true; Format = 'svg'
           Environment = @{ SEAMLYLAYOUT_SAVE_HANDOFF = '{outdir}\handoff.svg' }
           Arguments = $export + @('--layout-material', 'Lining', '--layout-view', 'marker', '--layout-settings', $roll36, $jacket)
           Exit = 0; Expect = @{ WidthIn = 36 }; Handoff = @{ MinBytes = 1000000; Contains = 'data-sizes=' } }
        @{ Name = 'jacket-split-marker-unavailable'; Group = 'oversize'; Slow = $true
           Arguments = $export + @('--layout-material', 'Fabric', '--layout-view', 'marker', '--layout-oversize', 'split', '--layout-settings', $roll12, $jacket)
           Exit = 64; NoOutput = $true; Stderr = '\S' }
        @{ Name = 'jacket-split-default-view-pdf'; Group = 'oversize'; Slow = $true; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--layout-material', 'Fabric', '--layout-oversize', 'split', '--layout-settings', $roll12, $jacket)
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 12 } }

        # --- Direct layout flags (win over --layout-settings) -----------------------------------------------------
        @{ Name = 'flags-roll-width-pdf'; Group = 'flags'; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--layout-unit', 'in', '--layout-roll-width', '24', $shirt)
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 24; MaxPageHeightIn = 400 } }
        @{ Name = 'flags-override-settings-file'; Group = 'flags'; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--layout-settings', $roll36, '--layout-roll-width', '30', $shirt)
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 30 } }
        @{ Name = 'flags-tile-a4-landscape-mm'; Group = 'flags'; Format = 'pdf-tiled'
           Arguments = $export + @('--layout-format', 'pdf-tiled', '--layout-unit', 'mm', '--layout-tile-size', 'A4',
                                   '--layout-tile-orientation', 'landscape', '--layout-margins', '10', $shirt)
           Exit = 0; Expect = @{ MinPages = 2; PageWidthIn = 11.69; PageHeightIn = 8.27 } }
        @{ Name = 'flags-custom-tile'; Group = 'flags'; Format = 'pdf-tiled'
           Arguments = $export + @('--layout-format', 'pdf-tiled', '--layout-unit', 'in', '--layout-tile-width', '10',
                                   '--layout-tile-height', '12', '--layout-tile-orientation', 'portrait', $shirt)
           Exit = 0; Expect = @{ MinPages = 2; PageWidthIn = 10; PageHeightIn = 12 } }
        @{ Name = 'flags-fabric-folded-nap'; Group = 'flags'; Format = 'svg'
           Arguments = $export + @('--layout-unit', 'cm', '--layout-fabric-width', '150', '--layout-fabric-folded', 'yes',
                                   '--layout-selvedge-width', '1', '--layout-nap', 'down', $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; WidthIn = (75 / 2.54) } }
        @{ Name = 'flags-ignore-margins'; Group = 'flags'; Format = 'svg'
           Arguments = $export + @('--svg-text-mode', 'designerFont', '--layout-settings', $roll36, '--layout-ignore-margins', $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; WidthIn = 36 }; DiffersFrom = 'shirt-svg-designer-font' }
        @{ Name = 'flags-mode-gap-no-grainline'; Group = 'flags'; Format = 'svg'
           Arguments = $export + @('--svg-text-mode', 'designerFont', '--layout-settings', $roll36, '--layout-mode', 'any',
                                   '--layout-gap', '0.3', '--layout-no-grainline', 'free', $shirt)
           Exit = 0; Expect = @{ MinPieces = 16; WidthIn = 36 }; DiffersFrom = 'shirt-svg-designer-font' }
        @{ Name = 'flags-conflict-fabric-paper'; Group = 'flags'
           Arguments = $export + @('--layout-fabric-width', '60', '--layout-paper', 'roll', $shirt)
           Exit = 64; Stderr = 'conflict'; NoOutput = $true }
        @{ Name = 'flags-not-a-number'; Group = 'flags'; Arguments = $export + @('--layout-gap', 'wide', $shirt)
           Exit = 64; Stderr = "gap: 'wide' is not a number"; NoOutput = $true }
        @{ Name = 'flags-invalid-value'; Group = 'flags'; Arguments = $export + @('--layout-tile-orientation', 'Portrait', $shirt)
           Exit = 64; Stderr = 'tileOrientation'; NoOutput = $true }
        @{ Name = 'flags-nap-with-mode-any'; Group = 'flags'; Arguments = $export + @('--layout-nap', 'up', '--layout-mode', 'any', $shirt)
           Exit = 64; Stderr = 'nap needs layout mode withNap'; NoOutput = $true }
        @{ Name = 'flags-without-export'; Group = 'flags'; Arguments = @('--layout-gap', '1', $shirt)
           Exit = 64; Stderr = 'require --layout-export' }
        @{ Name = 'flags-info-narrow-roll'; Group = 'flags'; Arguments = @('--layout-info', '--layout-roll-width', '12', $shirt)
           Exit = 0; Info = @{ TiledPaper = $false; OversizedMin = 5 } }

        # --- Pieces as placed in Piece mode (no layout run) --------------------------------------------------
        @{ Name = 'no-packing-svg'; Group = 'no-packing'; Format = 'svg'
           Arguments = $export + @('--layout-no-packing', $shirt); Exit = 0; Expect = @{ MinPieces = 16 } }
        @{ Name = 'no-packing-pdf'; Group = 'no-packing'; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--layout-no-packing', $shirt); Exit = 0; Expect = @{ Pages = 1 } }
        @{ Name = 'no-packing-dxf'; Group = 'no-packing'; Format = 'dxf'
           Arguments = $export + @('--layout-format', 'dxf', '--layout-no-packing', $shirt); Exit = 0; Expect = @{ Version = 'AC1012'; MinBlocks = 16 } }
        @{ Name = 'no-packing-jacket-lining'; Group = 'no-packing'; Format = 'svg'; Slow = $true
           Arguments = $export + @('--layout-material', 'Lining', '--layout-no-packing', '--gsize', '40', '--gheight', '146', $jacket)
           Exit = 0; Expect = @{ MinPieces = 5 } }
        @{ Name = 'no-packing-with-view'; Group = 'no-packing'; Arguments = $export + @('--layout-no-packing', '--layout-view', 'marker', $jacket)
           Exit = 64; Stderr = '--layout-no-packing exports the pieces as placed' }

        # --- Rust/Iced tester in batch mode -------------------------------------------------------------------
        @{ Name = 'tester-shirt-pdf-roll'; Group = 'tester'; Program = 'tester'
           Arguments = @('--batch', '--pattern', $shirt, '--settings', $roll36, '--format', 'pdf', '--output-dir', '{outdir}')
           Exit = 0; ZipEntries = 1; Stdout = 'PASS: (.|\n)*ARCHIVE: ' }
        @{ Name = 'tester-jacket-auto-split'; Group = 'tester'; Program = 'tester'; Slow = $true
           Arguments = @('--batch', '--pattern', $jacket, '--settings', $roll12, '--format', 'pdf', '--basename', 'jacket 2', '--output-dir', '{outdir}')
           Exit = 0; ZipEntries = 3; Stdout = 'split pieces: true; materials: 3' }
        @{ Name = 'tester-refuses-marker-dxf'; Group = 'tester'; Program = 'tester'; Slow = $true
           Arguments = @('--batch', '--pattern', $jacket, '--settings', $roll36, '--view', 'Marker', '--format', 'dxf', '--output-dir', '{outdir}')
           Exit = 2; Stderr = 'exports one size' }

        # --- Richmond pattern end to end (from git history; see -RichmondRoot) --------------------------------
        @{ Name = 'richmond-pattern-info'; Group = 'richmond'; Richmond = $true
           Arguments = @('--layout-info', '--layout-settings', $roll36, '{richmond}\richmond-shirt_v1_v061-test.sm2d')
           Exit = 0; Info = @{ Multisize = $false; Materials = @('Fabric'); TiledPaper = $false } }
        @{ Name = 'richmond-pattern-svg'; Group = 'richmond'; Richmond = $true; Format = 'svg'
           Arguments = $export + @('--layout-settings', $roll36, '{richmond}\richmond-shirt_v1_v061-test.sm2d')
           Exit = 0; Expect = @{ MinPieces = 12; WidthIn = 36; HasText = $true } }
        @{ Name = 'richmond-pattern-pdf'; Group = 'richmond'; Richmond = $true; Format = 'pdf'
           Arguments = $export + @('--layout-format', 'pdf', '--layout-settings', $roll36, '{richmond}\richmond-shirt_v1_v061-test.sm2d')
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 36; MaxPageHeightIn = 400 } }
        @{ Name = 'richmond-pattern-pdf-tiled'; Group = 'richmond'; Richmond = $true; Format = 'pdf-tiled'
           Arguments = $export + @('--layout-format', 'pdf-tiled', '--layout-settings', $letter, '{richmond}\richmond-shirt_v1_v061-test.sm2d')
           Exit = 0; Expect = @{ MinPages = 2; PageWidthIn = 8.5; PageHeightIn = 11 } }
        @{ Name = 'richmond-pattern-dxf'; Group = 'richmond'; Richmond = $true; Format = 'dxf'
           Arguments = $export + @('--layout-format', 'dxf', '--layout-settings', $roll36, '{richmond}\richmond-shirt_v1_v061-test.sm2d')
           Exit = 0; Expect = @{ Version = 'AC1012'; MinBlocks = 12 } }
        @{ Name = 'richmond-pattern-measurements'; Group = 'richmond'; Richmond = $true; Format = 'svg'
           Arguments = $export + @('--mfile', '{richmond}\2025-06-08-Sue.smis', '--layout-settings', $roll36,
                                   '{richmond}\richmond-shirt_v1_v061-test.sm2d')
           Exit = 0; Expect = @{ MinPieces = 12; WidthIn = 36 } }

        # --- SeamlyLayout headless with the Richmond handoff ------------------------------------------------------
        @{ Name = 'richmond-handoff-svg'; Group = 'seamlylayout'; Program = 'seamlylayout'; Format = 'svg'
           Arguments = @('--export', 'svg', '--output', '{out}', '--settings', $roll36, '{repo}\src\app\seamlylayout\crates\cxxqt_bridge\test_data\richmond-shirt-handoff_pieces.svg')
           Exit = 0; Expect = @{ MinPieces = 12; WidthIn = 36 } }
        @{ Name = 'richmond-handoff-pdf'; Group = 'seamlylayout'; Program = 'seamlylayout'; Format = 'pdf'
           Arguments = @('--export', 'pdf', '--output', '{out}', '--settings', $roll36, '{repo}\src\app\seamlylayout\crates\cxxqt_bridge\test_data\richmond-shirt-handoff_pieces.svg')
           Exit = 0; Expect = @{ Pages = 1; PageWidthIn = 36 } }
        @{ Name = 'richmond-handoff-dxf'; Group = 'seamlylayout'; Program = 'seamlylayout'; Format = 'dxf'
           Arguments = @('--export', 'dxf', '--output', '{out}', '--settings', $roll36, '{repo}\src\app\seamlylayout\crates\cxxqt_bridge\test_data\richmond-shirt-handoff_pieces.svg')
           Exit = 0; Expect = @{ Version = 'AC1012'; MinBlocks = 12 } }
        @{ Name = 'richmond-handoff-info'; Group = 'seamlylayout'; Program = 'seamlylayout'
           Arguments = @('--info', '--settings', $roll36, '{repo}\src\app\seamlylayout\crates\cxxqt_bridge\test_data\richmond-shirt-handoff_pieces.svg')
           Exit = 0; Info = @{ Multisize = $false; TiledPaper = $false } }
    ) | ForEach-Object { [pscustomobject]$_ }
}
