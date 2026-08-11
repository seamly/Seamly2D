#!/bin/bash
#
# @file   test_version_script.sh
# @author slspencer
# @date   2026
# @brief  Unit tests for scripts/version.sh.
#
# Verifies that version.sh writes valid, base-10 version numbers into
# src/libs/vmisc/projectversion.cpp and src/libs/vmisc/projectversion.h. The
# regression under test: a CI version whose last component has a leading zero
# (eg. 2026.8.11.048, generated at 00:48 UTC) used to be emitted verbatim as a
# C++ octal literal, which fails to compile on '8'/'9' and silently yields the
# wrong number otherwise.
#
# Usage: ./scripts/test_version_script.sh
#
# @copyright 2026 Seamly2D Project
# @license   GPL-3.0-or-later
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version. This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details. You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

CPP_FILE="src/libs/vmisc/projectversion.cpp"
HEADER_FILE="src/libs/vmisc/projectversion.h"

FAILURES=0

# @brief Report a passing or failing assertion.
# @param $1 1 when the assertion held, 0 otherwise
# @param $2 description of the assertion
check() {
    if [ "$1" -eq 1 ]; then
        echo "  PASS: $2"
    else
        echo "  FAIL: $2"
        FAILURES=$((FAILURES + 1))
    fi
}

# @brief Read back one 'extern const int <name> = <value>;' from projectversion.cpp.
# @param $1 the constant's name
# @return the assigned value, verbatim, on stdout
read_constant() {
    sed -n "s/^extern const int $1 = \(.*\);$/\1/p" "${REPO_ROOT}/${CPP_FILE}"
}

# @brief Run version.sh against a version string and assert on what it wrote.
# @param $1 version string passed to version.sh
# @param $2..$5 expected major, minor, debug and patch values in the .cpp
# @param $6 expected VER_FILEVERSION_STR value in the .h
run_case() {
    local version="$1" expect_major="$2" expect_minor="$3" expect_debug="$4" expect_patch="$5" expect_str="$6"

    echo "case: ${version}"

    (cd "${REPO_ROOT}" && ./scripts/version.sh "${version}" >/dev/null)
    if [ $? -ne 0 ]; then
        check 0 "version.sh exits 0"
        return
    fi

    local major minor debug patch str
    major="$(read_constant MAJOR_VERSION)"
    minor="$(read_constant MINOR_VERSION)"
    debug="$(read_constant DEBUG_VERSION)"
    patch="$(read_constant SUPER_MINOR__VERSION)"
    str="$(sed -n 's/^#define VER_FILEVERSION_STR "\(.*\)"$/\1/p' "${REPO_ROOT}/${HEADER_FILE}")"

    [ "${major}" = "${expect_major}" ] && check 1 "MAJOR_VERSION == ${expect_major}" \
        || check 0 "MAJOR_VERSION == ${expect_major} (got '${major}')"
    [ "${minor}" = "${expect_minor}" ] && check 1 "MINOR_VERSION == ${expect_minor}" \
        || check 0 "MINOR_VERSION == ${expect_minor} (got '${minor}')"
    [ "${debug}" = "${expect_debug}" ] && check 1 "DEBUG_VERSION == ${expect_debug}" \
        || check 0 "DEBUG_VERSION == ${expect_debug} (got '${debug}')"
    [ "${patch}" = "${expect_patch}" ] && check 1 "SUPER_MINOR__VERSION == ${expect_patch}" \
        || check 0 "SUPER_MINOR__VERSION == ${expect_patch} (got '${patch}')"
    [ "${str}" = "${expect_str}" ] && check 1 "VER_FILEVERSION_STR == ${expect_str}" \
        || check 0 "VER_FILEVERSION_STR == ${expect_str} (got '${str}')"

    # No component may keep a leading zero: that is an octal literal in C++.
    local octal
    octal="$(grep -c '^extern const int .* = 0[0-9]' "${REPO_ROOT}/${CPP_FILE}")"
    [ "${octal}" = "0" ] && check 1 "no octal integer literals written" \
        || check 0 "no octal integer literals written (found ${octal})"
}

# @brief Assert that version.sh rejects a malformed version string.
# @param $1 the version string expected to be rejected
run_reject_case() {
    echo "case: ${1} (expected to be rejected)"
    (cd "${REPO_ROOT}" && ./scripts/version.sh "${1}" >/dev/null 2>&1)
    [ $? -ne 0 ] && check 1 "version.sh exits non-zero" || check 0 "version.sh exits non-zero"
}

# The tests rewrite tracked files, so stash the contents of every file
# version.sh touches and restore them afterwards no matter how the script exits.
TOUCHED_FILES=(
    "${CPP_FILE}"
    "${HEADER_FILE}"
    "dist/macx/seamly2d/Info.plist"
    "dist/macx/seamlyme/Info.plist"
)

BACKUP_DIR="$(mktemp -d)"
for file in "${TOUCHED_FILES[@]}"; do
    mkdir -p "${BACKUP_DIR}/$(dirname "${file}")"
    cp "${REPO_ROOT}/${file}" "${BACKUP_DIR}/${file}"
done

restore() {
    for file in "${TOUCHED_FILES[@]}"; do
        cp "${BACKUP_DIR}/${file}" "${REPO_ROOT}/${file}"
    done
    rm -rf "${BACKUP_DIR}"
}
trap restore EXIT

# An ordinary release version passes through unchanged.
run_case "2023.1.1.1046" 2023 1 1 1046 "2023.1.1.1046"

# The regression: 00:48 UTC produced '048', an illegal octal literal.
run_case "2026.8.11.048" 2026 8 11 48 "2026.8.11.48"

# A leading zero that *is* valid octal is still wrong (047 == 39), so it must
# also be normalized rather than left alone.
run_case "2026.8.11.047" 2026 8 11 47 "2026.8.11.47"

# Leading zeros in any position, and an all-zero component, are handled.
run_case "2026.08.09.0000" 2026 8 9 0 "2026.8.9.0"

# Malformed input is rejected instead of corrupting the source files.
run_reject_case "2026.8.11"
run_reject_case "2026.8.11.x48"

echo
if [ "${FAILURES}" -eq 0 ]; then
    echo "all version.sh tests passed"
    exit 0
fi

echo "${FAILURES} version.sh test(s) failed"
exit 1
