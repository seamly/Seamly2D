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
# Runtime folders (created by the app at first launch, not by this script — all resolved
# via Qt's QStandardPaths::AppConfigLocation under the shared "Seamly" organization name,
# see Task 15/Task 16):
#   ~/Library/Application Support/Seamly/SeamlyLayout/settings/      — layout settings JSON
#   ~/Library/Application Support/Seamly/SeamlyLayout/preferences/   — user preferences JSON
#   ~/Library/Application Support/Seamly/SeamlyLayout/input/         — default import folder
#   ~/Library/Application Support/Seamly/SeamlyLayout/output/        — default export/log folder
# The app bundle itself (Contents/Resources/settings/, copied below) is read-only after
# signing/notarizing and is never written to at runtime.
#
# Legacy migration (automatic at first launch):
#   - layout-settings/ / layout-preferences/ found under the current "Seamly/SeamlyLayout"
#     AppConfigLocation root are copied to the canonical settings/preferences folder names
#   - the entire pre-Task-15 "Seamly Systems/SeamlyLayout" AppConfigLocation tree
#     (~/Library/Application Support/Seamly Systems/SeamlyLayout/) is copied forward into
#     the new "Seamly/SeamlyLayout" root the first time it resolves empty
# Both migrations are copy-if-missing and non-destructive; see PreferencesModel.cpp /
# SettingsModel.cpp (appConfigRootPath() / migrateLegacyOrganizationTree()).

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
