<!-- MIT License: https://opensource.org/licenses/MIT -->
# Unit Test Commands

This document contains unit test commands for testing the SeamlyLayout workspace crates.

## General Test Commands

### Run Build Validation Suite (includes 3MF validation)
```bash
pwsh -File scripts/run_build_suite.ps1
```

### Run Build Validation Suite (Release)
```bash
pwsh -File scripts/run_build_suite.ps1 -Release
```

### Run Build Validation Suite with Custom 3MF
```bash
pwsh -File scripts/run_build_suite.ps1 -ThreeMfPath output/your_file.3mf
```

### Run All Tests in Workspace
```bash
cargo test
```

### Run Tests for Specific Crate
```bash
cargo test -p <crate_name>
```

### Run Tests with Output
```bash
cargo test -- --nocapture
```

### Run Tests in Release Mode
```bash
cargo test --release
```

### Run Tests and Show Documentation
```bash
cargo test --doc
```

### Run Tests with Verbose Output
```bash
cargo test -- --verbose
```

## seamly_svg2ezdxf Crate Tests

### Run All Tests
```bash
cargo test -p seamly_svg2ezdxf
```

### Run Specific Test
```bash
cargo test -p seamly_svg2ezdxf test_convert_simple_line
```

### Run Tests with Output (See Print Statements)
```bash
cargo test -p seamly_svg2ezdxf -- --nocapture
```

### Run Tests and Show Test Names
```bash
cargo test -p seamly_svg2ezdxf -- --show-output
```

### Run Tests for Specific Module
```bash
cargo test -p seamly_svg2ezdxf converter_tests
```

### List All Available Tests (Without Running)
```bash
cargo test -p seamly_svg2ezdxf -- --list
```

### Run Tests Matching a Pattern
```bash
cargo test -p seamly_svg2ezdxf test_convert
```

### Run Tests and Stop on First Failure
```bash
cargo test -p seamly_svg2ezdxf -- --exact
```

### Run Tests with Thread Count
```bash
cargo test -p seamly_svg2ezdxf -- --test-threads=1
```

## Specific Test Cases for seamly_svg2ezdxf

### Available Tests
The following tests are available for `seamly_svg2ezdxf`:

1. `test_convert_simple_line` - Tests SVG line to DXF LINE conversion
2. `test_convert_simple_circle` - Tests SVG circle to DXF CIRCLE conversion
3. `test_convert_pattern_pieces_to_blocks` - Tests pattern piece extraction to blocks
4. `test_layer_mapping` - Tests layer mapping from SVG to ASTM layers
5. `test_coordinate_transformation` - Tests Y-axis coordinate transformation
6. `test_text_conversion` - Tests SVG text to DXF TEXT conversion

### Run Individual Tests

#### Test Line Conversion
```bash
cargo test -p seamly_svg2ezdxf test_convert_simple_line
```

#### Test Circle Conversion
```bash
cargo test -p seamly_svg2ezdxf test_convert_simple_circle
```

#### Test Pattern Piece Extraction
```bash
cargo test -p seamly_svg2ezdxf test_convert_pattern_pieces_to_blocks
```

#### Test Layer Mapping
```bash
cargo test -p seamly_svg2ezdxf test_layer_mapping
```

#### Test Coordinate Transformation
```bash
cargo test -p seamly_svg2ezdxf test_coordinate_transformation
```

#### Test Text Conversion
```bash
cargo test -p seamly_svg2ezdxf test_text_conversion
```

### Run All Conversion Tests
```bash
cargo test -p seamly_svg2ezdxf test_convert
```

### Run All Tests with Output
```bash
cargo test -p seamly_svg2ezdxf -- --nocapture
```

## Other Crates

### geometry Crate Tests
```bash
cargo test -p geometry
```

### svg_dom Crate Tests
```bash
cargo test -p svg_dom
```

### app_core Crate Tests
```bash
cargo test -p app_core
```

### layout_engine Crate Tests
```bash
cargo test -p layout_engine
```

### ui_desktop Crate Tests
```bash
cargo test -p ui_desktop
```

### cli Crate Tests
```bash
cargo test -p cli
```

### ezdxf2dxfastm Crate Tests
```bash
cargo test -p ezdxf2dxfastm
```

## Test Filtering

### Run Tests Matching Pattern Across All Crates
```bash
cargo test --test '*pattern*'
```

### Run Tests in Specific File
```bash
cargo test -p seamly_svg2ezdxf --test converter_test
```

### Exclude Tests Matching Pattern
```bash
cargo test -p seamly_svg2ezdxf -- --skip slow
```

## Test Output and Debugging

### Run Single Test with Full Output
```bash
cargo test -p seamly_svg2ezdxf test_convert_simple_line -- --nocapture --exact
```

### Run Tests with Backtrace (for Debugging)
```bash
RUST_BACKTRACE=1 cargo test -p seamly_svg2ezdxf
```

### Run Tests and Generate Test Report
```bash
cargo test -p seamly_svg2ezdxf -- --test-threads=1 --report-time
```

### Run Tests with Coverage (requires cargo-tarpaulin)
```bash
cargo tarpaulin -p seamly_svg2ezdxf
```

## Integration Tests

### Run Integration Tests
```bash
cargo test --test '*'
```

### Run Tests in tests/ Directory
```bash
cargo test --test integration_test
```

## Documentation Tests

### Run Documentation Tests
```bash
cargo test --doc
```

### Run Documentation Tests for Specific Crate
```bash
cargo test -p seamly_svg2ezdxf --doc
```

## Benchmark Tests

### Run Benchmarks (if configured)
```bash
cargo bench
```

### Run Benchmarks for Specific Crate
```bash
cargo bench -p geometry
```

## Continuous Integration Commands

### Run All Tests (CI Style)
```bash
cargo test --workspace --all-features
```

### Run Tests with Warnings as Errors
```bash
cargo test --workspace -- -D warnings
```

### Run Tests and Check Formatting
```bash
cargo fmt --check && cargo test
```

### Run Tests and Linting
```bash
cargo clippy --tests -- -D warnings && cargo test
```

## Quick Reference

| Command | Description |
|---------|-------------|
| `cargo test` | Run all tests |
| `cargo test -p <crate>` | Run tests for specific crate |
| `cargo test -- --nocapture` | Show output from tests |
| `cargo test <test_name>` | Run specific test |
| `cargo test -- --list` | List all tests without running |
| `cargo test --doc` | Run documentation tests |
| `RUST_BACKTRACE=1 cargo test` | Run with backtrace for debugging |

## Test Results

### Current Test Status for seamly_svg2ezdxf

As of implementation:
- ✅ `test_convert_simple_line` - PASSING
- ✅ `test_convert_simple_circle` - PASSING
- ✅ `test_convert_pattern_pieces_to_blocks` - PASSING
- ✅ `test_coordinate_transformation` - PASSING
- ✅ `test_text_conversion` - PASSING
- ✅ `test_layer_mapping` - PASSING (updated)

**Total: 6 tests, all passing**

## Example Test Workflow

### 1. Run All Tests
```bash
cargo test
```

### 2. Run Tests for seamly_svg2ezdxf with Output
```bash
cargo test -p seamly_svg2ezdxf -- --nocapture
```

### 3. Run Specific Test
```bash
cargo test -p seamly_svg2ezdxf test_convert_simple_line -- --nocapture
```

### 4. Debug Failing Test
```bash
RUST_BACKTRACE=full cargo test -p seamly_svg2ezdxf test_convert_simple_line -- --nocapture --exact
```

### 5. Run All Tests and Show Summary
```bash
cargo test -p seamly_svg2ezdxf -- --test-threads=1
```

## Notes

- Use `--nocapture` to see `println!` output from tests
- Use `--exact` to run only the exact test name match
- Use `--test-threads=1` to run tests sequentially (useful for debugging)
- Use `RUST_BACKTRACE=1` or `RUST_BACKTRACE=full` for detailed error information
- Tests are located in `#[cfg(test)]` modules within source files or in `tests/` directories
