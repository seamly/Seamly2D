message("Entering app.pro")

TEMPLATE = subdirs
SUBDIRS = \
    seamlyme \
    seamly2d

# ---------------------------------------------------------------------------
# Why seamlylayout/ is NOT in SUBDIRS
#
# src/app/seamlylayout/ holds the third family app, SeamlyLayout. It is tracked
# in this repository like seamlyme, but it is deliberately kept out of the qmake
# build and must stay out (see the root CLAUDE.md).
#
#   1. It is not a qmake project - it is a cmake project due to its use of Rust.
#      'TEMPLATE = subdirs' requires each entry to be a
#      directory containing a matching <name>.pro; 'seamlylayout/' has none. Its
#      entry points are 'qt_frontend/CMakeLists.txt' and a Cargo workspace
#      (Cargo.toml). Listing it here would make qmake fail looking for
#      'seamlylayout/seamlylayout.pro'.
#
#   2. Its build cannot be expressed in qmake. 'qt_frontend/CMakeLists.txt' uses
#      FetchContent to pull 'cxx-qt-cmake', then 'cxx_qt_import_crate' to drive
#      Corrosion, which runs `cargo build` on the 'cxxqt_bridge' Rust crate and
#      links the resulting static library; it also relies on qt_add_qml_module
#      (qmldir / .qmltypes / qmltyperegistrar / compiled-in QML) and
#      'qt_generate_deploy_qml_app_script'. qmake has no equivalent for any of it.
#
#   3. It would impose a Rust toolchain and Ninja on every qmake build. A plain
#      `qmake Seamly.pro && nmake` would then require rustup and cargo, breaking
#      the Linux and macOS jobs in '.github/workflows/ci.yml', which build only the
#      parent apps and never install Rust.
#
# How SeamlyLayout is built instead:
#   * locally  - src/app/seamlylayout/qd.ps1 (debug) or build.ps1 (CMake+Cargo)
#   * in CI    - .github/workflows/ci.yml, 'windows-msi' job (CMake/Ninja + Cargo)
#
# The three apps are integrated at PACKAGING time, not compile time. All three
# apps build against the same Qt release (currently 6.11.1), so
# scripts/packaging/windows/smsi.ps1 builds seamly2d.exe and seamlym.exe with qmake
# and SeamlyLayout with CMake, then stages all three into one install directory
# sharing a single Qt runtime (scripts/packaging/windows/seamly-family.wxs).
# Sharing a Qt version does not mean sharing a build system.
# ---------------------------------------------------------------------------

macx{# For qmake app bundle, seamlyme must exist before seamly2d.app will be created
    seamly2d.depends = seamlyme
}
