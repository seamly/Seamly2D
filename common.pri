win32{
    # Because "copy" doesn't support files that contain plus sign (+) in name we will use xcopy instead.
    unset(QMAKE_COPY)
    QMAKE_COPY = xcopy /y

    unset(QMAKE_COPY_FILE)
    QMAKE_COPY_FILE = xcopy /y

    unset(QMAKE_INSTALL_FILE)
    QMAKE_INSTALL_FILE = xcopy /y

    unset(QMAKE_INSTALL_PROGRAM)
    QMAKE_INSTALL_PROGRAM = xcopy /y

    VCOPY = $$QMAKE_COPY /D
}

# Use platform-appropriate copy commands when staging runtime files into the target directory.
unix{
    macx{
        VCOPY = $$QMAKE_COPY
    } else {
        VCOPY = $$QMAKE_COPY -u
    }
}

macx{
    QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64
}

# The build shipped libcrypto-1_1-x64.dll and libssl-1_1-x64.dll here until
# 2026-08-15. Do not add them back. Qt 6.11's TLS plugin loads libssl-3-x64 and
# libcrypto-3-x64 only, so the 1.1 files were dead weight. Fervor is the one
# TLS user, and it falls back to Schannel, which is what the arm64 build has
# always done - INSTALL_OPENSSL never applied to the win32-arm64-msvc spec.

# Ensure xerces-c_3_3.dll is deployed into the target folder
win32-msvc {
    INSTALL_XERCES = ../../libs/xerces-c/msvc/lib/xerces-c_3_3.dll
}
win32-arm64-msvc {
    INSTALL_XERCES = ../../libs/xerces-c/msvc-arm64/lib/xerces-c_3_3.dll
}

# MSVC: force utf-8 source for ° symbol and other utf-8 strings in source files
# Source: https://stackoverflow.com/questions/48705747/how-utf-8-may-not-work-in-qt-5
win32:!win32-g++: QMAKE_CXXFLAGS += /utf-8

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT


CONFIG(debug, debug|release){
    # Debug builds do not define release-only macros.
} else {
    # Release builds disable assertions and debug-only behavior.
    message("Release mode: V_NO_ASSERT V_NO_DEBUG defined")
    DEFINES += V_NO_ASSERT V_NO_DEBUG
}

# Qt6 requires C++17 minimum
CONFIG += c++17

# Prevent building both debug and release builds
CONFIG -= debug_and_release debug_and_release_target

# Source code location is recorded only for debug builds.
# We need this information also in release builds. For this need define QT_MESSAGELOGCONTEXT.
DEFINES += QT_MESSAGELOGCONTEXT

# Copies the given files to the destination directory
defineTest(copyToDestdir) {
    files = $$1
    DDIR = $$2
    mkpath($$DDIR)

    for(FILE, files) {
        unix{
            QMAKE_POST_LINK += ln -s -f $$quote($$FILE) $$quote($$DDIR/$$basename(FILE)) $$escape_expand(\\n\\t)
        } else {
            !exists($$DDIR/$$basename(FILE)) {
                # Replace slashes in paths with backslashes for Windows
                win32{
                    FILE ~= s,/,\\,g
                    DDIR ~= s,/,\\,g
                }
                message("copy:" $$quote($$FILE))
                QMAKE_POST_LINK += $$VCOPY $$quote($$FILE) $$quote($$DDIR) $$escape_expand(\\n\\t)
            }

            QMAKE_CLEAN += $$DDIR/$$basename(FILE)
        }
    }

    export(QMAKE_POST_LINK)
    export(QMAKE_CLEAN)
}

# Always copies the given files to the destination directory.
defineTest(forceCopyToDestdir) {
    files = $$1
    DDIR = $$2
    mkpath($$DDIR)

    for(FILE, files) {
        unix{
            QMAKE_POST_LINK += ln -s -f $$quote($$FILE) $$quote($$DDIR/$$basename(FILE)) $$escape_expand(\\n\\t)
        } else {
            # Replace slashes in paths with backslashes for Windows
            win32{
                FILE ~= s,/,\\,g
                DDIR ~= s,/,\\,g
            }
            QMAKE_POST_LINK += $$VCOPY $$quote($$FILE) $$quote($$DDIR) $$escape_expand(\\n\\t)
            QMAKE_CLEAN += $$DDIR/$$basename(FILE)
        }
    }

    export(QMAKE_POST_LINK)
    export(QMAKE_CLEAN)
}

# @brief Adds a windeployqt post-link step for an executable on MSVC.
# @param 1 Executable path, for example $$DESTDIR/$${TARGET}.exe.
# @return true. Does nothing for non-MSVC builds.
#
# This helper always deploys. Callers handle CONFIG+=deferDeploy.
# The main applications defer deployment for MSI packaging; the test target
# deploys immediately because nmake check runs it independently.
#
# qtPrepareTool() selects windeployqt from this qmake's Qt installation instead
# of using the first version found on PATH. This prevents deploying incompatible
# Qt libraries from another installation.
#
# Native x64 and arm64 builds use the same command and do not need --qtpaths.
# That option is only for cross-compiled kits, whose windeployqt needs the
# host-qtpaths.bat wrapper to find the target Qt installation.
defineTest(deployQtRuntime) {
    EXE = $$shell_path($$1)

    win32-msvc|win32-arm64-msvc {
        qtPrepareTool(WINDEPLOYQT, windeployqt)
        QMAKE_POST_LINK += $$WINDEPLOYQT $$EXE
        export(QMAKE_POST_LINK)
    }

    return(true)
}


# Release builds enable the shared precompiled header used for the project-wide stable includes.
CONFIG(debug, debug|release){
    # Debug mode, intentionally left empty
} else {
    # Release mode - Turn on creating precompiled headers (PCH).
    CONFIG += precompile_header 
    # Header file with all all static headers: libraries, static local headers.
    PRECOMPILED_HEADER = stable.h 
    *msvc*{
        PRECOMPILED_SOURCE = stable.cpp # MSVC need also cpp file.
    }
}

# Release builds name the binary with the current Git short SHA; debug builds return unknown.
defineReplace(FindBuildRevision){
CONFIG(debug, debug|release){
    # Debug mode
    return(\\\"unknown\\\")
}else{
    # Release mode - build revision number for using in version
    # get short form of latest commit's changeset hash, i.e. a 12-character hexadecimal string
    DVCS_HASH=$$system("git rev-parse --short=12 HEAD") #get SHA1 commit hash
    message("common.pri: Latest commit hash:" $${DVCS_HASH})

    isEmpty(DVCS_HASH){
       DVCS_HASH = \\\"unknown\\\" # if we can't find build revision left unknown.
    } else {
       DVCS_HASH=\\\"Git:$${DVCS_HASH}\\\"
    }

    return($${DVCS_HASH})
}
}

# Default prefix. Use for creating the binary installation path.
DEFAULT_PREFIX = /usr

# Debug builds enable compiler warnings; release builds disable them.
CONFIG(debug, debug|release){
    # Debug mode
    message("Normal mode: compiler warnings enabled")
    CONFIG += warn_on
} else {
    message("Release mode: no compiler warnings")
    CONFIG += warn_off
}
