<!-- MIT License: https://opensource.org/licenses/MIT -->
# DXF-ASTM Export UI Workflow

This document describes the user interface workflow for exporting SVG layout data to DXF-ASTM format in the SeamlyLayout desktop application.

## Overview

The DXF-ASTM export workflow is fully integrated into the desktop UI (`ui_desktop` crate) and provides a user-friendly interface for exporting pattern layouts to DXF format with optional teaching version generation.

## User Workflow

### Step 1: Prepare Layout

1. **Import SVG File**: Click "Import" button to load a pattern SVG file
2. **Configure Settings**: Click "Settings" button to configure layout parameters
3. **Process Layout**: Click "Process Layout" button to generate the layout
4. **Verify Layout**: Review the layout in the right canvas

### Step 2: Initiate Export

1. **Open Export Menu**: Click "Export ▼" button to open the export format dropdown
2. **Select Format**: Click "DXF-ASTM" from the dropdown menu
3. **File Save Dialog**: A file save dialog opens
   - Default filename: Based on input SVG filename with `.dxf` extension
   - User can change the filename and location
   - File filter: DXF files (*.dxf)

### Step 3: Teaching Version Dialog

After selecting the file path, a dialog appears asking about creating a teaching version:

**Dialog Content**:
- **Question**: "Create teaching version with comments?"
- **Explanation**: "This will create a .txt file with inline comments explaining each line of the DXF file. This may add a few seconds to the export time."
- **Buttons**:
  - **Cancel**: Cancels the export entirely
  - **No**: Exports only the DXF file (no teaching version)
  - **Yes**: Exports both DXF file and teaching version (.txt file)

### Step 4: Export Execution

#### If User Selects "Yes" (Teaching Version):

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

4. **Generate Teaching Version**:
   - Reads the exported DXF file
   - Adds inline comments explaining each line
   - Saves as `.txt` file in same directory as DXF file
   - Filename: Same as DXF file but with `.txt` extension

5. **Success Message**: 
   - Status bar shows: "Exported to [filename]"
   - Canvas message shows: "DXF-ASTM saved: [filename]"

#### If User Selects "No" (No Teaching Version):

1. **Convert SVG to ezdxf**: Same as above
2. **Save ezdxf for Debugging**: Same as above
3. **Export to DXF-ASTM**: Same as above
4. **Skip Teaching Version**: No `.txt` file created
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
- `DxfTeachingDialogYes` - User wants teaching version
- `DxfTeachingDialogNo` - User doesn't want teaching version
- `DxfTeachingDialogCancel` - User cancels export

**Shell State Fields**:
- `dxf_teaching_dialog_open: bool` - Controls dialog visibility
- `pending_dxf_path: Option<PathBuf>` - Stores path while dialog is open

### Export Function Flow

```rust
fn export_dxf_astm_to_path(&mut self, path: &Path, create_teaching_version: bool) {
    // 1. Convert SVG to ezdxf Drawing
    let drawing = self.convert_seamly_svg_2_ezdxf(layout_flat)?;
    
    // 2. Save ezdxf to output folder (for debugging)
    self.save_ezdxf(&drawing)?;
    
    // 3. Export to DXF-ASTM
    let options = DxfAstmExportOptions {
        include_header: false,
        validate_entities: true,
        sanitize_text: true,
        create_teaching_version, // User's choice
    };
    export_dxf_astm(&drawing, path, &options)?;
}
```

### Dialog Implementation

The teaching version dialog is implemented as a modal overlay similar to the PDF export dialog:

```rust
fn dxf_teaching_dialog() -> Element<'static, Message> {
    // Question text
    // Explanation text
    // Three buttons: Cancel, No, Yes
    // Modal overlay styling
}
```

## File Outputs

### DXF File (.dxf)

- **Location**: User-selected path
- **Format**: DXF R12 (AC1009)
- **Structure**:
  - HEADER section (minimal/empty)
  - BLOCKS section (pattern piece definitions)
  - ENTITIES section (INSERT entities for blocks)
  - EOF marker

### Teaching Version File (.txt)

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

- **Teaching Version Generation**: Adds 2-5 seconds for large files
- **File Size**: Teaching version files are typically 2-3x larger than DXF files
- **Dialog Timing**: Dialog appears immediately after file path selection
- **Non-blocking**: Export runs synchronously (UI may freeze briefly for large files)

### File Management

- **Default Location**: User's last used directory (handled by file dialog)
- **Filename Suggestion**: Based on input SVG filename
- **Teaching Version**: Automatically named (same as DXF with .txt extension)
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
