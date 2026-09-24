#!/bin/bash

# instead of manually editing the version in multiple places, run this script with the desired version number, eg 26.9.2317:
#
# ./packaging/version.sh 26.9.2317
#
# The version number has to be a three part version YY.M.DDHH, where DDHH is
# day * 100 + hour (0-23). The MSI ProductVersion has only three fields.
# This script works under linux and mac (as they have perl and and bash by default), and under windows if you have bash and perl.
#
# Updates these files, unconditionally:
#   src/libs/vmisc/projectversion.cpp -- compiles `extern cost in` constants read by the About dialog 
#   src/libs/vmisc/projectversion.h 
#   packaging/macos/seamly2d/Info.plist -- macos metadata read by Finder, Launch Services, and Gatekeeper
#   packaging/macos/seamlyme/Info.plist -- macos metadata read by Finder, Launch Services, and Gatekeeper

if [ $# -eq 1 ]; then
	VERSIONSTR="${1}"
else
    echo "usage: $0 <version>, where version needs to be a 3 part version number, eg. 26.9.2317"
    exit 1
fi

VERSIONARR=(${VERSIONSTR//./ })

if [ ${#VERSIONARR[@]} -ne 3 ]; then
    echo "error: '${VERSIONSTR}' is not a 3 part version number, eg. 26.9.2317"
    exit 1
fi

# Strip leading zeros from every component.
#
# The components are written into projectversion.cpp as plain C++ integer
# literals. A literal with a leading zero is an *octal* constant there, so a CI
# version such as 26.8.0105 (generated on day 1 at 05:00) either fails to compile
# ("illegal digit '8' for base '8'") or, worse, compiles to the wrong number
# (048 -> 39). '10#' forces base-10 interpretation in the arithmetic expansion.
for i in "${!VERSIONARR[@]}"; do
    if ! [[ ${VERSIONARR[$i]} =~ ^[0-9]+$ ]]; then
        echo "error: version component '${VERSIONARR[$i]}' is not a number"
        exit 1
    fi
    VERSIONARR[$i]=$((10#${VERSIONARR[$i]}))
done

# Rebuild the full version string so the normalized components are also used in
# the string forms (VER_FILEVERSION_STR, Info.plist).
VERSIONSTR="${VERSIONARR[0]}.${VERSIONARR[1]}.${VERSIONARR[2]}"

echo "setting version to"
echo "major: ${VERSIONARR[0]}"
echo "minor: ${VERSIONARR[1]}"
echo "day and hour: ${VERSIONARR[2]}"

perl -pi -e "s,extern const int MAJOR_VERSION = .*;,extern const int MAJOR_VERSION = ${VERSIONARR[0]};,g" src/libs/vmisc/projectversion.cpp
perl -pi -e "s,extern const int MINOR_VERSION = .*;,extern const int MINOR_VERSION = ${VERSIONARR[1]};,g" src/libs/vmisc/projectversion.cpp
perl -pi -e "s,extern const int DEBUG_VERSION = .*;,extern const int DEBUG_VERSION = ${VERSIONARR[2]};,g" src/libs/vmisc/projectversion.cpp

perl -pi -e "s/#define VER_FILEVERSION .*/#define VER_FILEVERSION ${VERSIONARR[0]},${VERSIONARR[1]},${VERSIONARR[2]},0/g" src/libs/vmisc/projectversion.h
perl -pi -e "s,#define VER_FILEVERSION_STR \".*\",#define VER_FILEVERSION_STR \"$VERSIONSTR\",g" src/libs/vmisc/projectversion.h

perl -pi -e "s,<key>CFBundleShortVersionString</key><string>.*</string>,<key>CFBundleShortVersionString</key><string>${VERSIONARR[0]}.${VERSIONARR[1]}.${VERSIONARR[2]}</string>,g" packaging/macos/seamly2d/Info.plist
perl -pi -e "s,<key>CFBundleVersion</key><string>.*</string>,<key>CFBundleVersion</key><string>${VERSIONARR[0]}.${VERSIONARR[1]}.${VERSIONARR[2]}</string>,g" packaging/macos/seamly2d/Info.plist

perl -pi -e "s,<key>CFBundleShortVersionString</key><string>.*</string>,<key>CFBundleShortVersionString</key><string>${VERSIONARR[0]}.${VERSIONARR[1]}.${VERSIONARR[2]}</string>,g" packaging/macos/seamlyme/Info.plist
perl -pi -e "s,<key>CFBundleVersion</key><string>.*</string>,<key>CFBundleVersion</key><string>${VERSIONARR[0]}.${VERSIONARR[1]}.${VERSIONARR[2]}</string>,g" packaging/macos/seamlyme/Info.plist
