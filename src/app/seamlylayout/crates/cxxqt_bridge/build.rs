// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// build.rs — CXX-Qt code generation for the cxxqt_bridge crate.
//
// Scans src/lib.rs for `#[cxx_qt::bridge]` modules and emits the
// corresponding C++ header and source files into Cargo's OUT_DIR.
// The Qt CMake build picks these up via `cxx_qt_import_crate`.
//
// Note: in cxx-qt-build v0.7, the builder type is `CxxQtBuilder`
// (was `CxxQtBuild` in earlier versions).
use cxx_qt_build::CxxQtBuilder;

// Entry point for the build script.
//
// Scans src/lib.rs for `#[cxx_qt::bridge]` blocks and generates C++ glue
// into Cargo's OUT_DIR for the Qt cmake build to compile and link.
fn main() {
    CxxQtBuilder::new()  // construct the CXX-Qt build runner
        .file("src/lib.rs") // scan this file for #[cxx_qt::bridge] modules
        .build();            // emit generated C++ into OUT_DIR
} // fn main
