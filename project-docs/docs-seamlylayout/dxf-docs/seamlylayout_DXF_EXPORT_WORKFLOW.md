<!-- MIT License: https://opensource.org/licenses/MIT -->
# DXF-ASTM Export UI Workflow

This document describes the user interface workflow for exporting SVG layout data to DXF-ASTM format in the SeamlyLayout desktop application.

## Overview

The DXF-ASTM export workflow is fully integrated into the desktop UI (`ui_desktop` crate) and provides a user-friendly interface for exporting pattern layouts to DXF format with optional annotated version generation.

## User Workflow

### Step 1: Prepare Layout

1. **Import SVG File**: Click "Import" button to load a pattern SVG file
2. **Configure Settings**: Click "Settings" button to configure layout parameters
3. **Process Layout**: Click "Process Layout" button to generate the layout
4. **Verify Layout**: Review the layout in the right canvas

### Step 2: Initiate Export

1. **Open Export Menu**: Click "Export ▼" button to open the export format dropdown
2. **Select Format**: Click "DXF-ASTM (R12)", "DXF-ASTM (R13)" or "DXF-ASTM (CLO3D)" from the dropdown menu. CLO3D writes R13
3. **Missing Piece Data Dialog**: Shown only when a piece lacks DXF-ASTM data
   - Checks each piece with a boundary for: a "Cut N" label line (`Quantity:`), label text (layer 15), a grainline (layer 7)
   - Names each incomplete piece and what it lacks
   - States that this affects the usability of the exported DXF file
   - **Export anyway**: continue to the annotated version dialog
   - **Cancel**: stop the export
   - Checks the shown tab only; Export all tabs uses the same pieces for every size
   - Code: `Drawing::missing_piece_data` (`seamly_svg2ezdxf`), `exports::dxf_missing_piece_data_message`, `AppController::dxf_missing_piece_data`, `DxfMissingDataDialog.qml`
   - Example: male_shirt warns about CollarBaseInterface, CollarTopInterface, CuffInterface
4. **Annotated Version Dialog** (`DxfAnnotatedDialog.qml`): opens before the save dialog, so the default name can carry the choice
   - **Annotated Version**: DXF file plus annotated version (`.txt` file)
   - **Standard**: DXF file only
   - **Cancel**: stop the export
5. **File Save Dialog**: opens with a default name
   - Standard: `<importedBaseName>_<R12|R13|CLO3D>_YYYYMMDDHHMM.dxf`, e.g. `male_shirt_R13_202610011721.dxf`
   - Annotated: `<importedBaseName>_<R12|R13|CLO3D>_annotated_YYYYMMDDHHMM.dxf`, e.g. `male_shirt_CLO3D_annotated_202610011721.dxf`
   - Segment code: `exports::dxf_export_name_segment` → `AppController::dxfExportNameSegment`
   - User can change the filename and location
   - File filter: DXF files (*.dxf)
   - **Cancel**: stop the export

### Step 3: Annotated Version File Name

- The `.txt` file keeps the `.dxf` base name: `male_shirt_R12_annotated_202610011721.dxf` → `male_shirt_R12_annotated_202610011721.txt`
- Export all tabs: the tab label goes before the extension of both files

### Step 4: Export Execution

#### If User Selects "Yes" (Annotated Version):

1. **Convert SVG to ezdxf**: 
   - Converts layout SVG DOM to ezdxf Drawing object
   - Extracts pattern pieces as blocks
   - Converts all SVG elements to DXF entities
   - Applies coordinate transformation (Y-axis inversion)

2. **Save ezdxf for Debugging**:
   - Saves ezdxf Drawing to `output/` folder as `.txt` file
   - Filename format: `layout_ezdxf_YYYYMMDDHHMM.txt`
   - Human-readable format for inspection

3. **Export to DXF-ASTM**:
   - Converts ezdxf Drawing to DXF R12 format
   - Writes DXF file to user-selected path
   - Creates INSERT entities for all blocks in ENTITIES section

4. **Generate Annotated Version**:
   - Reads the exported DXF file
   - Adds inline comments explaining each line
   - Saves as `.txt` file in same directory as DXF file
   - Filename: Same as DXF file but with `.txt` extension

5. **Success Message**: 
   - Status bar shows: "Exported to [filename]"
   - Canvas message shows: "DXF-ASTM saved: [filename]"

#### If User Selects "No" (No Annotated Version):

1. **Convert SVG to ezdxf**: Same as above
2. **Save ezdxf for Debugging**: Same as above
3. **Export to DXF-ASTM**: Same as above
4. **Skip Annotated Version**: No `.txt` file created
5. **Success Message**: Same as above

#### If User Selects "Cancel":

- Export is cancelled
- No files are created
- Dialog closes
- User returns to main interface

## Technical Implementation

### UI Components

**Message Enum Variants**:
- `ExportFormatSelected("DXF-ASTM")` - User selected DXF-ASTM from dropdown
- `DxfAstmSavePathPicked(Option<PathBuf>)` - File save dialog result
- `DxfAnnotatedDialogYes` - User wants annotated version
- `DxfAnnotatedDialogNo` - User doesn't want annotated version
- `DxfAnnotatedDialogCancel` - User cancels export

**Shell State Fields**:
- `dxf_annotated_dialog_open: bool` - Controls dialog visibility
- `pending_dxf_path: Option<PathBuf>` - Stores path while dialog is open

### Export Function Flow

```rust
fn export_dxf_astm_to_path(&mut self, path: &Path, create_annotated_version: bool) {
    // 1. Convert SVG to ezdxf Drawing
    let drawing = self.convert_seamly_svg_2_ezdxf(layout_flat)?;
    
    // 2. Save ezdxf to output folder (for debugging)
    self.save_ezdxf(&drawing)?;
    
    // 3. Export to DXF-ASTM
    let options = DxfAstmExportOptions {
        include_header: false,
        validate_entities: true,
        sanitize_text: true,
        create_annotated_version, // User's choice
    };
    export_dxf_astm(&drawing, path, &options)?;
}
```

### Dialog Implementation

The annotated version dialog is implemented as a modal overlay similar to the PDF export dialog:

```rust
fn dxf_annotated_dialog() -> Element<'static, Message> {
    // Question text
    // Explanation text
    // Three buttons: Cancel, No, Yes
    // Modal overlay styling
}
```

## File Outputs

### DXF File (.dxf)

- **Location**: User-selected path
- **Format**: DXF R12 (AC1009) or DXF R13 (AC1012), from the menu item
- **R12 structure**:
  - HEADER section (minimal/empty)
  - BLOCKS section (pattern piece definitions)
  - ENTITIES section (INSERT entities for blocks)
  - EOF marker
- **R13 structure**: HEADER, CLASSES, TABLES, BLOCKS, ENTITIES, OBJECTS, EOF; handles on every entity. See `seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`

### Annotated Version File (.txt)

- **Location**: Same directory as DXF file
- **Format**: DXF content with inline comments
- **Comments**: 
  - Explain group codes (0, 2, 8, 10, 20, etc.)
  - Explain entity types (LINE, CIRCLE, POLYLINE, etc.)
  - Explain coordinates and values
  - Positioned two tabs to the right of DXF data

### ezdxf Debug File (.txt)

- **Location**: `output/` folder
- **Format**: Human-readable ezdxf Drawing representation
- **Purpose**: Debugging and inspection of intermediate representation
- **Filename**: `layout_ezdxf_YYYYMMDDHHMM.txt`

## Error Handling

### Export Failures

If any step fails:
- **Status bar**: Shows error message
- **Canvas message**: Shows error details
- **No files created**: Export is aborted
- **User can retry**: After fixing issues

### Common Errors

1. **No flattened layout**: "Export failed: No flattened layout to export"
2. **SVG conversion error**: "Export failed: SVG to ezdxf conversion error: [details]"
3. **DXF write error**: "Export failed: DXF-ASTM write error: [details]"
4. **File permission error**: "Export failed: [IO error details]"

## User Experience Considerations

### Performance

- **Annotated Version Generation**: Adds 2-5 seconds for large files
- **File Size**: Annotated version files are typically 2-3x larger than DXF files
- **Dialog Timing**: Annotated version dialog appears before the file save dialog
- **Non-blocking**: Export runs synchronously (UI may freeze briefly for large files)

### File Management

- **Default Location**: User's last used directory (handled by file dialog)
- **Filename Suggestion**: Input base name, variant, `_annotated` for the annotated version, timestamp
- **Annotated Version**: Automatically named (same as DXF with .txt extension)
- **Overwrite Warning**: File dialog handles existing file warnings

## Future Enhancements

### Potential Improvements

1. **Async Export**: Run export in background thread to prevent UI freezing
2. **Progress Indicator**: Show progress bar during export
3. **Batch Export**: Export multiple layouts at once
4. **Export Options Dialog**: Similar to PDF dialog with more options
5. **Preview**: Show DXF structure preview before export
6. **Block Positioning**: Position blocks based on layout algorithm results

## References

- **DXF Export Architecture**: `docs/DXF_EXPORT_ARCHITECTURE.md`
- **SVG to ezdxf Workflow**: `docs/SEAMLY_SVG2EZDXF_WORKFLOW.md`
- **DXF Export Summary**: `docs/DXF_EXPORT_SUMMARY.md`
- **Testing Guide**: `docs/TESTING_SEAMLY_SVG2EZDXF.md`
