#!/usr/bin/env bash
#******************************************************************************
# **  @file   local_build_dmg.sh
# **  @author slspencer
# **
# **  @brief
# **  Local, developer-machine build of the macOS DMG: builds seamly2d,
# **  seamlyme and SeamlyLayout release binaries, runs the Qt and Rust test
# **  suites, then packages all three .app bundles into one DMG via
# **  packaging/macos/macos.pro's seamlysuitedmg target (the same hdiutil
# **  step ci.yml's macos job uses). ci.yml's macos job does not run these
# **  test suites itself - it gates on the linux-test job's coverage instead.
# **  This script runs them anyway for a safer local loop; pass --skip-tests
# **  to match CI's behavior exactly. Treat the DMG this produces as a local
# **  dev build, not a release artifact - releases still go through
# **  `gh workflow run ci.yml`. Signing/notarization needs CI secrets and is
# **  skipped here.
# **
# ** @copyright
# **  Copyright (C) 2026 Seamly2D Project
# **
# **  @license
# **  GPL-3.0-or-later
#******************************************************************************
#
# Usage:
#   packaging/macos/local_build_dmg.sh [--version YY.M.D.mmmm] [--qt-root DIR] [--skip-tests]
#
# Requires on PATH: qmake, cmake, ninja, cargo, ctest, hdiutil, and Xcode
# command-line tools (xcode-select --install). System packages (xerces-c,
# ninja via brew) are never installed by this script - it fails with an
# install hint instead.

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

echo " ===== running local_build_dmg.sh ====="

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
# Same YY.M.D.mmmm formula ci.yml's version job uses.
if [ -z "${VERSION}" ]; then
    read -r YEAR MONTH DAY HOUR MINUTE <<< "$(date +'%Y %m %d %H %M')"
    VERSION="$((10#${YEAR} - 2000)).$((10#${MONTH})).$((10#${DAY})).$((10#${HOUR} * 60 + 10#${MINUTE}))"
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
# Newest macos kit under ~/Qt at or above 6.11.1, so seamly2d/seamlyme/
# SeamlyLayout all deploy against the one Qt runtime the DMG ships.
QT_MINIMUM_VERSION="6.11.1"
if [ -z "${QT_ROOT}" ]; then
    best_ver=""
    for d in "${HOME}/Qt"/*/macos; do
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
    echo "No Qt ${QT_MINIMUM_VERSION}+ macos kit found under \$HOME/Qt. Pass --qt-root <dir>." >&2
    exit 1
fi
missing_modules=()
for module in Qt6WebEngineQuick Qt6WebChannel Qt6Positioning; do
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
for tool in ninja cmake cargo ctest qmake hdiutil xcrun; do
    command -v "${tool}" >/dev/null 2>&1 || {
        echo "required tool '${tool}' not found on PATH - install it first (brew install ninja cmake xerces-c; xcode-select --install; cargo via rustup)." >&2
        exit 1
    }
done
if ! command -v pkg-config >/dev/null 2>&1 || ! pkg-config --exists xerces-c 2>/dev/null; then
    if [ ! -d /opt/homebrew/opt/xerces-c ] && [ ! -d /usr/local/opt/xerces-c ]; then
        echo "xerces-c not found - install it first (brew install xerces-c)." >&2
        exit 1
    fi
fi

# --- Build + test (debug) -----------------------------------------------------------
if [ "${SKIP_TESTS}" -eq 1 ]; then
    echo ""
    echo "unit tests: SKIPPED (--skip-tests) - matches ci.yml's macos job, which does not run its own tests."
else
    echo ""
    echo "=== qmake/make: seamly2d + seamlyme + unit tests ==="
    qmake Seamly.pro
    make -j"$(sysctl -n hw.logicalcpu)"

    echo "=== cmake: SeamlyLayout debug build ==="
    (cd src/app/seamlylayout/qt_frontend && \
        cmake --preset debug -DCMAKE_PREFIX_PATH="${QT_ROOT}" && \
        cmake --build --preset debug)

    echo "=== run tests: Seamly2D & SeamlyMe ==="
    make check

    echo "=== run tests: SeamlyLayout Qt suites ==="
    (cd src/app/seamlylayout/qt_frontend && ctest --preset debug -j"$(sysctl -n hw.logicalcpu)")

    echo "=== run tests: SeamlyLayout Rust crates ==="
    (cd src/app/seamlylayout && cargo test --workspace)
fi

# --- Package (release) ---------------------------------------------------------------
echo ""
echo "=== qmake/make: seamly2d + seamlyme release app bundles ==="
qmake Seamly.pro -config release CONFIG+=noTests
make -j"$(sysctl -n hw.logicalcpu)"

# The CMake target's MACOS_BUNDLE_POST_BUILD deploy script runs macdeployqt
# automatically after this build - no separate deploy step is needed here.
echo "=== cmake: SeamlyLayout release build (auto-deploys Qt frameworks) ==="
(cd src/app/seamlylayout/qt_frontend && \
    cmake --preset release -DCMAKE_PREFIX_PATH="${QT_ROOT}" && \
    cmake --build --preset release)

echo "=== macos.pro: combine all three .app bundles into one DMG (hdiutil) ==="
(cd packaging/macos && qmake macos.pro && make)

echo ""
if [ "${SKIP_TESTS}" -eq 1 ]; then
    echo "unit tests: SKIPPED (--skip-tests)"
else
    echo "unit tests: PASSED (make check, ctest, cargo test)"
fi
echo "DMG OK: ${REPO_ROOT}/packaging/macos/Seamly2D-macos.dmg (unsigned, not notarized)"
if [ "${VERSION_FILES_WERE_CLEAN}" -eq 1 ]; then
    echo "reverting the version stamp (projectversion.cpp/.h, both Info.plist) - packaging/version.sh regenerates it on demand, nothing to keep..."
    git checkout -- "${VERSION_STAMPED_FILES[@]}"
else
    echo "projectversion.cpp/.h and/or Info.plist carried uncommitted changes before this build - not auto-reverting them; check 'git status' for what to keep."
fi
