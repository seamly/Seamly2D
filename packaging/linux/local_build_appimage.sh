#!/usr/bin/env bash
#******************************************************************************
# **  @file   local_build_appimage.sh
# **  @author slspencer
# **
# **  @brief
# **  Local, developer-machine build of the Linux x86_64 AppImage: builds
# **  seamly2d, seamlyme and SeamlyLayout release binaries, runs the Qt and
# **  Rust test suites, then packages all three into one AppImage with
# **  linuxdeploy. Mirrors ci.yml's linux-test and linux jobs. Treat the
# **  AppImage this produces as a local dev build, not a release artifact -
# **  releases still go through `gh workflow run ci.yml`.
# **
# ** @copyright
# **  Copyright (C) 2026 Seamly2D Project
# **
# **  @license
# **  GPL-3.0-or-later
#******************************************************************************
#
# Usage:
#   packaging/linux/local_build_appimage.sh [--version YY.M.DDHH] [--qt-root DIR] [--skip-tests]
#
# Requires on PATH: qmake, cmake, ninja, cargo, ctest, xvfb-run, pdftops
# (poppler-utils), and the libxerces-c-dev headers. Auto-downloads
# linuxdeploy + linuxdeploy-plugin-qt into packaging/linux/tools/ if missing
# (user-space download, no sudo). System packages are never installed by
# this script - it fails with an install hint instead.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

VERSION=""
QT_ROOT=""
SKIP_TESTS=0

while [ $# -gt 0 ]; do
    case "$1" in
        --version) VERSION="$2"; shift 2 ;;
        --qt-root) QT_ROOT="$2"; shift 2 ;;
        --skip-tests) SKIP_TESTS=1; shift ;;
        *) echo "unknown argument: $1" >&2; exit 1 ;;
    esac
done

echo " ===== running local_build_appimage.sh ====="

#------------------------------------------------------------------------------
# @brief Compare two dot-separated numeric versions.
# @return 0 (true) if the first is >= the second.
#------------------------------------------------------------------------------
version_ge() {
    local IFS=.
    local -a a=($1) b=($2)
    local i ai bi
    for i in 0 1 2; do
        ai=${a[i]:-0}
        bi=${b[i]:-0}
        if (( ai > bi )); then return 0; fi
        if (( ai < bi )); then return 1; fi
    done
    return 0
}

# --- Version --------------------------------------------------------------------
# Same YY.M.DDHH formula ci.yml's version job uses.
if [ -z "${VERSION}" ]; then
    read -r YEAR MONTH DAY HOUR MINUTE <<< "$(date +'%Y %m %d %H %M')"
    VERSION="$((10#${YEAR} - 2000)).$((10#${MONTH})).$((10#${DAY} * 100 + 10#${HOUR}))"
fi
echo "version: ${VERSION}"

VERSION_STAMPED_FILES=(
    src/libs/vmisc/projectversion.cpp
    src/libs/vmisc/projectversion.h
    packaging/macos/seamly2d/Info.plist
    packaging/macos/seamlyme/Info.plist
)
VERSION_FILES_WERE_CLEAN=1
if [ -n "$(git status --porcelain -- "${VERSION_STAMPED_FILES[@]}")" ]; then
    VERSION_FILES_WERE_CLEAN=0
fi
echo "stamping version into projectversion.cpp/.h and Info.plist..."
bash packaging/version.sh "${VERSION}"

# --- Locate the Qt kit ------------------------------------------------------------
# Newest gcc_64 kit under ~/Qt at or above 6.11.1, so seamly2d/seamlyme/
# SeamlyLayout all deploy against the one Qt runtime the AppImage ships.
QT_MINIMUM_VERSION="6.11.1"
if [ -z "${QT_ROOT}" ]; then
    best_ver=""
    for d in "${HOME}/Qt"/*/gcc_64; do
        [ -d "${d}" ] || continue
        ver="$(basename "$(dirname "${d}")")"
        version_ge "${ver}" "${QT_MINIMUM_VERSION}" || continue
        if [ -z "${best_ver}" ] || version_ge "${ver}" "${best_ver}"; then
            best_ver="${ver}"
            QT_ROOT="${d}"
        fi
    done
fi
if [ -z "${QT_ROOT}" ] || [ ! -d "${QT_ROOT}" ]; then
    echo "No Qt ${QT_MINIMUM_VERSION}+ gcc_64 kit found under \$HOME/Qt. Pass --qt-root <dir>." >&2
    exit 1
fi
missing_modules=()
for module in Qt6WebEngineQuick Qt6WebChannel Qt6Positioning Qt6SerialPort; do
    [ -d "${QT_ROOT}/lib/cmake/${module}" ] || missing_modules+=("${module}")
done
if [ ${#missing_modules[@]} -gt 0 ]; then
    echo "Qt kit at '${QT_ROOT}' is missing required module(s): ${missing_modules[*]} - install them via the Qt Maintenance Tool." >&2
    exit 1
fi
echo "Qt kit: ${QT_ROOT}"
export PATH="${QT_ROOT}/bin:${PATH}"
export QMAKE="${QT_ROOT}/bin/qmake"

# --- Check required tools ----------------------------------------------------------
for tool in ninja cmake cargo ctest qmake xvfb-run pdftops; do
    command -v "${tool}" >/dev/null 2>&1 || {
        echo "required tool '${tool}' not found on PATH - install it first (apt install ninja-build cmake xvfb poppler-utils; cargo via rustup)." >&2
        exit 1
    }
done
if ! ldconfig -p 2>/dev/null | grep -q libxerces-c; then
    echo "libxerces-c-dev not found - install it first (apt install libxerces-c-dev)." >&2
    exit 1
fi

# --- linuxdeploy tools (user-space download, no sudo) -------------------------------
TOOLS_DIR="${SCRIPT_DIR}/tools"
mkdir -p "${TOOLS_DIR}"
LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
if [ ! -x "${LINUXDEPLOY}" ]; then
    echo "downloading linuxdeploy..."
    curl -sL "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -o "${LINUXDEPLOY}"
    chmod +x "${LINUXDEPLOY}"
fi
if [ ! -x "${LINUXDEPLOY_QT}" ]; then
    echo "downloading linuxdeploy-plugin-qt..."
    curl -sL "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" -o "${LINUXDEPLOY_QT}"
    chmod +x "${LINUXDEPLOY_QT}"
fi
export PATH="${TOOLS_DIR}:${PATH}"

# --- Build + test (debug) -----------------------------------------------------------
if [ "${SKIP_TESTS}" -eq 1 ]; then
    echo ""
    echo "unit tests: SKIPPED (--skip-tests) - Seamly2D/SeamlyMe/SeamlyLayout suites are deferred to CI."
else
    echo ""
    echo "=== qmake/make: seamly2d + seamlyme + unit tests ==="
    qmake Seamly.pro
    make -j"$(nproc)"

    echo "=== cmake: SeamlyLayout debug build ==="
    (cd src/app/seamlylayout/qt_frontend && \
        cmake --preset debug -DCMAKE_PREFIX_PATH="${QT_ROOT}" && \
        cmake --build --preset debug)

    echo "=== run tests: Seamly2D & SeamlyMe (xvfb) ==="
    xvfb-run -a make check

    echo "=== run tests: SeamlyLayout Qt suites ==="
    (cd src/app/seamlylayout/qt_frontend && ctest --preset debug -j"$(nproc)")

    echo "=== run tests: SeamlyLayout Rust crates ==="
    (cd src/app/seamlylayout && cargo test --workspace)
fi

# --- Package (release) ---------------------------------------------------------------
echo ""
echo "=== qmake/make: seamly2d + seamlyme release, install into AppDir ==="
rm -rf "${REPO_ROOT}/AppDir"
qmake Seamly.pro -config release CONFIG+=noTests
make -j"$(nproc)"
make INSTALL_ROOT="${REPO_ROOT}/AppDir" install

echo "=== cmake: SeamlyLayout release build ==="
(cd src/app/seamlylayout/qt_frontend && \
    cmake --preset release -DCMAKE_PREFIX_PATH="${QT_ROOT}" && \
    cmake --build --preset release)

echo "=== cmake --install: stage SeamlyLayout into AppDir ==="
(cd src/app/seamlylayout/qt_frontend && \
    cmake --install build/Release --prefix "${REPO_ROOT}/AppDir/usr")

echo "=== linuxdeploy: package AppImage ==="
mkdir -p AppDir/usr/bin
cp "$(command -v pdftops)" AppDir/usr/bin
if [ -d /usr/share/X11/xkb ]; then
    mkdir -p AppDir/usr/share/X11/xkb
    cp -r /usr/share/X11/xkb/* AppDir/usr/share/X11/xkb
else
    echo "warning: /usr/share/X11/xkb not found - the AppImage may lack keyboard layout data." >&2
fi
OUTPUT="Seamly2D-x86_64.AppImage" \
QML_SOURCES_PATHS="src/app/seamlylayout/qt_frontend/qml" \
    "${LINUXDEPLOY}" --appdir AppDir \
    --desktop-file=packaging/assets/seamly2d.desktop \
    --desktop-file=packaging/linux/seamlylayout.desktop \
    --plugin qt --output appimage

echo ""
if [ "${SKIP_TESTS}" -eq 1 ]; then
    echo "unit tests: SKIPPED (--skip-tests)"
else
    echo "unit tests: PASSED (make check, ctest, cargo test)"
fi
echo "AppImage OK: ${REPO_ROOT}/Seamly2D-x86_64.AppImage"
if [ "${VERSION_FILES_WERE_CLEAN}" -eq 1 ]; then
    echo "reverting the version stamp (projectversion.cpp/.h, both Info.plist) - packaging/version.sh regenerates it on demand, nothing to keep..."
    git checkout -- "${VERSION_STAMPED_FILES[@]}"
else
    echo "projectversion.cpp/.h and/or Info.plist carried uncommitted changes before this build - not auto-reverting them; check 'git status' for what to keep."
fi
