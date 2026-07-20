#!/usr/bin/env bash
# project: SeamlyLayout
# author: slspencer, copyright 2026
# MIT License: https://opensource.org/licenses/MIT
#
# build_dmg.sh — Create a macOS disk image (.dmg) for SeamlyLayout
#
# Prerequisites:
#   • macOS 13+ (Ventura) or later
#   • Xcode command-line tools (xcode-select --install)
#   • Qt 6.10.1 installed at /usr/local/Qt/6.10.1/macos
#   • create-dmg (brew install create-dmg)
#   • A signed Release build:  cmake --build --preset release
#
# Runtime folders (created by the app at first launch, not by this script):
#   ~/seamlyLayout/settings/     — layout settings JSON files
#   ~/seamlyLayout/preferences/  — user preferences JSON
#
# Legacy migration (automatic at first launch):
#   If layout-settings/ or layout-preferences/ exist under
#   ~/Library/Application Support/SeamlyLayout/ from a pre-0.1.0 install,
#   the app copies them to the canonical folder names on first run.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/qt_frontend/build/Release"
APP_BUNDLE="${BUILD_DIR}/SeamlyLayout.app"
SETTINGS_DIR="${REPO_ROOT}/qt_frontend/settings"
LICENSES_DIR="${REPO_ROOT}/packaging/licenses"
OUTPUT_DIR="${SCRIPT_DIR}/Output"
DMG_NAME="SeamlyLayout-0.1.0-macOS"

if [ ! -d "${APP_BUNDLE}" ]; then
    echo "ERROR: App bundle not found at ${APP_BUNDLE}" >&2
    echo "Run cmake --build --preset release first." >&2
    exit 1
fi

echo "=== Running macdeployqt ==="
QT_BIN="/usr/local/Qt/6.10.1/macos/bin"
"${QT_BIN}/macdeployqt" "${APP_BUNDLE}" \
    -qmldir="${REPO_ROOT}/qt_frontend/qml" \
    -always-overwrite

echo "=== Copying packaged defaults into bundle ==="
RESOURCES="${APP_BUNDLE}/Contents/Resources"
mkdir -p "${RESOURCES}/settings"
# preferences.json is per-user — copy only the default settings JSON files.
for f in "${SETTINGS_DIR}"/*.json; do
    name="$(basename "${f}")"
    if [ "${name}" != "preferences.json" ]; then
        cp "${f}" "${RESOURCES}/settings/${name}"
    fi
done

echo "=== Copying licenses ==="
mkdir -p "${RESOURCES}/licenses"
cp "${LICENSES_DIR}/LGPL-3.0.txt"        "${RESOURCES}/licenses/"
cp "${LICENSES_DIR}/qt-source-notice.txt" "${RESOURCES}/licenses/"

echo "=== Building DMG ==="
mkdir -p "${OUTPUT_DIR}"
create-dmg \
    --volname "SeamlyLayout" \
    --volicon "${REPO_ROOT}/qt_frontend/assets/images/seamly-layout.icns" \
    --window-pos 200 120 \
    --window-size 660 400 \
    --icon-size 100 \
    --icon "SeamlyLayout.app" 160 185 \
    --hide-extension "SeamlyLayout.app" \
    --app-drop-link 500 185 \
    "${OUTPUT_DIR}/${DMG_NAME}.dmg" \
    "${APP_BUNDLE}"

echo "=== Done: ${OUTPUT_DIR}/${DMG_NAME}.dmg ==="
