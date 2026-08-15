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
    # Debug mode, intentionally left empty
} else {
    # Release mode
    message("Release mode: V_NO_ASSERT V_NO_DEBUG defined")
    DEFINES += V_NO_ASSERT V_NO_DEBUG
}

CONFIG += c++14

# Only do debug or release builds also on windows
CONFIG -= debug_and_release debug_and_release_target

# Since Qt 5.4.0 the source code location is recorded only in debug builds.
# We need this information also in release builds. For this need define QT_MESSAGELOGCONTEXT.
DEFINES += QT_MESSAGELOGCONTEXT

# Copies the given files to the destination directory
defineTest(copyToDestdir) {
    files = $$1
    DDIR = $$2
    mkpath($$DDIR)

    for(FILE, files) {
        unix{
            QMAKE_POST_LINK += ln -s -f $$quote($$FILE) $$quote($$DDIR/$$basename(FILE)) & $$escape_expand(\\n\\t)
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

# Alwayse copies the given files to the destination directory
defineTest(forceCopyToDestdir) {
    files = $$1
    DDIR = $$2
    mkpath($$DDIR)

    for(FILE, files) {
        unix{
            QMAKE_POST_LINK += ln -s -f $$quote($$FILE) $$quote($$DDIR/$$basename(FILE)) & $$escape_expand(\\n\\t)
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

# @brief  Append a windeployqt post-link step for the given executable (MSVC only).
# @param  1  Path of the executable to deploy beside, e.g. $$DESTDIR/$${TARGET}.exe
# @return true (always) — a no-op on every non-MSVC mkspec.
#
# Why this is a shared helper rather than a block copied into each .pro: the
# three MSVC targets (seamly2d, seamlyme, Seamly2DTests) each kept their own
# copy of this step, and they drifted — the arm64 MSI build broke because one
# copy passed a --qtpaths wrapper that the arm64 kit does not ship.
#
# qtPrepareTool() resolves windeployqt out of $$[QT_INSTALL_BINS] — the Qt that
# *this* qmake belongs to — instead of letting the shell find the first
# windeployqt on PATH. That matters because an unrelated Qt earlier on PATH
# (e.g. Qt Design Studio's reduced 6.8.x kit) would otherwise deploy its own,
# older Qt DLLs beside an exe linked against the build kit. Qt's binary
# compatibility is forward-only, so that mismatch produces a build tree whose
# exes cannot start — and which the MSI packaging script would ship verbatim.
#
# x64 and arm64 are handled IDENTICALLY, with no --qtpaths flag, because every
# Windows build is NATIVE: ci.yml's windows-msi job builds x64 on
# windows-latest and arm64 on windows-11-arm, each installing its own host kit
# (win64_msvc2022_64 / win64_msvc2022_arm64). Nothing is cross-compiled, so the
# windeployqt qtPrepareTool() picks is always an executable the runner can run
# and always belongs to the kit being deployed — it resolves its own paths.
#
# --qtpaths is only needed by a CROSS-COMPILED kit
# (win64_msvc2022_arm64_cross_compiled), whose windeployqt is an x64 binary that
# cannot infer the arm64 target's paths and must be pointed at the
# host-qtpaths.bat wrapper install-qt-action generates. Passing the flag anyway
# is what broke the arm64 MSI build — the native kit ships no such wrapper, and
# windeployqt fails with '"...\bin\host-qtpaths.bat" does not exist'. Restore
# the flag only if a cross-compiled kit is ever reintroduced.
defineTest(deployQtRuntime) {
    EXE = $$shell_path($$1)

    win32-msvc|win32-arm64-msvc {
        qtPrepareTool(WINDEPLOYQT, windeployqt)
        QMAKE_POST_LINK += $$WINDEPLOYQT $$EXE
        export(QMAKE_POST_LINK)
    }

    return(true)
}

CONFIG(debug, debug|release){
    # Debug mode, intentionally left empty
} else {
    CONFIG += precompile_header # Turn on creation precompiled headers (PCH).
    PRECOMPILED_HEADER = stable.h # Header file with all all static headers: libraries, static local headers.
    *msvc*{
        PRECOMPILED_SOURCE = stable.cpp # MSVC need also cpp file.
    }
}

defineReplace(FindBuildRevision){
CONFIG(debug, debug|release){
    # Debug mode
    return(\\\"unknown\\\")
}else{
    # Release mode
    #build revision number for using in version
    #get the short form of the latest commit's changeset hash, i.e. a 12-character hexadecimal string
    DVCS_HESH=$$system("git rev-parse --short=12 HEAD") #get SHA1 commit hash
    message("common.pri: Latest commit hash:" $${DVCS_HESH})

    isEmpty(DVCS_HESH){
       DVCS_HESH = \\\"unknown\\\" # if we can't find build revision left unknown.
    } else {
       DVCS_HESH=\\\"Git:$${DVCS_HESH}\\\"
    }

    return($${DVCS_HESH})
}
}

# Default prefix. Use for creation install path.
DEFAULT_PREFIX = /usr

# In debug mode we use all usefull for us compilers keys for checking errors.
CONFIG(debug, debug|release){
    # Debug mode
    message("Normal mode: compiler warnings enabled")
    CONFIG += warn_on
} else {
    message("Release mode: no compiler warnings")
    CONFIG += warn_off
}
