// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file license_notices_tests.rs
// @brief Fails while the committed license notices no longer match Cargo.lock or the Qt version.
//
// The installers ship packaging/licenses/rust_crate_notices.txt and qt_notices.txt as committed
// files. Regenerate both with packaging/licenses/generate_license_notices.py when this test fails.

use std::path::PathBuf;

/// @brief Path under the SeamlyLayout workspace root (two levels above this crate).
fn workspace_path(relative: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../..").join(relative)
}

/// @brief 64-bit FNV-1a as 16 hex digits; must match fnv1a64() in generate_license_notices.py.
fn fnv1a64(data: &[u8]) -> String {
    let mut hash: u64 = 0xcbf2_9ce4_8422_2325;
    for byte in data {
        hash ^= u64::from(*byte);
        hash = hash.wrapping_mul(0x0000_0100_0000_01b3);
    } // for byte
    format!("{hash:016x}")
} // fn fnv1a64

/// @brief Value after `key` on the first line that starts with it.
fn header_value(text: &str, key: &str) -> Option<String> {
    text.lines().find_map(|line| line.strip_prefix(key)).map(|v| v.trim().to_string())
} // fn header_value

const REGENERATE: &str = "run: python src/app/seamlylayout/packaging/licenses/generate_license_notices.py --qt-root <Qt kit>";

#[test]
fn rust_crate_notices_match_cargo_lock() {
    // Normalise CRLF so a Windows checkout gives the same fingerprint as Linux.
    let lock = std::fs::read(workspace_path("Cargo.lock")).expect("Cargo.lock readable");
    let lock = String::from_utf8_lossy(&lock).replace("\r\n", "\n");
    let expected = format!("fnv1a64:{}", fnv1a64(lock.as_bytes()));

    let notices = std::fs::read_to_string(workspace_path("packaging/licenses/rust_crate_notices.txt"))
        .expect("rust_crate_notices.txt readable");
    let recorded = header_value(&notices, "Cargo.lock fingerprint:");
    assert_eq!(recorded.as_deref(), Some(expected.as_str()), "rust_crate_notices.txt is stale; {REGENERATE}");
} // rust_crate_notices_match_cargo_lock

#[test]
fn qt_notices_match_ci_qt_version() {
    // ci.yml sets the Qt release every package is built with: QT_VERSION: '6.11.1'
    let ci = std::fs::read_to_string(workspace_path("../../../.github/workflows/ci.yml")).expect("ci.yml readable");
    let ci_version = ci
        .lines()
        .find_map(|line| line.trim().strip_prefix("QT_VERSION:"))
        .and_then(|value| value.split('\'').nth(1))
        .map(str::to_string)
        .expect("QT_VERSION in ci.yml");

    let notices = std::fs::read_to_string(workspace_path("packaging/licenses/qt_notices.txt"))
        .expect("qt_notices.txt readable");
    let recorded = header_value(&notices, "Qt version:");
    assert_eq!(recorded, Some(ci_version), "qt_notices.txt is stale; {REGENERATE}");
} // qt_notices_match_ci_qt_version
