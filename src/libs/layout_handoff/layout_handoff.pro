#-------------------------------------------------
#
# @file   layout_handoff.pro
# @author slspencer
# Copyright (C) 2026 Seamly2D Project
# SPDX-License-Identifier: GPL-3.0-or-later
#
#-------------------------------------------------

# File with common stuff for whole
message("Entering layout_handoff.pro")
include(../../../common.pri)

QT += core gui widgets printsupport xml

# Name of library
TARGET = layout_handoff

# We want create library
TEMPLATE = lib

CONFIG += staticlib

# The library has no stable.h, so it opts out of the release precompiled header from common.pri.
CONFIG -= precompile_header

include(layout_handoff.pri)

# This is static library so no need in "make install"

# directory for executable file
DESTDIR = bin

# files created moc
MOC_DIR = moc

# objecs files
OBJECTS_DIR = obj

# Directory for files created rcc
RCC_DIR = rcc

include(warnings.pri)

CONFIG(release, debug|release){
    !*msvc* {
        QMAKE_CXXFLAGS -= -O2 # Disable default optimization level
        QMAKE_CXXFLAGS += -O3 # For layout_handoff library enable speed optimizations
    }
}

include (../libs.pri)
