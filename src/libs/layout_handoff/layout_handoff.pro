#-------------------------------------------------
#
# @file   layout_handoff.pro
# @author slspencer
# Copyright (C) 2026 Seamly2D Project
# SPDX-License-Identifier: GPL-3.0-or-later
#
# @brief
# Static library that writes the tagged pieces SVG. Seamly2D sends it to SeamlyLayout with --svg-stdin.
#
# Data source: the open pattern in Seamly2D.
# - VPiece: piece geometry and piece properties.
# - VContainer: pattern data that calculates the piece points and lengths.
# - VAbstractPattern: pattern document with the pattern label template.
# - VCommonSettings: colors, line styles, label font and show flags.
# Constraint:
# - Reads no canvas items (QGraphicsScene), so the export does not depend on what the canvas shows.
# Exception:
# - Label placeholders read qApp state through LabelTextManager.

# Classes:
# - HandoffDocument: SVG with one pattern group per material and one group per piece.
# - HandoffPiece: seam line, cut line, notches, internal and cutout paths, grainline, fold line.
# - HandoffLabel: piece label and pattern label.
# - HandoffPieceInfo: piece properties for the data-* attributes.
# - HandoffIds: unique SVG ids for pieces and components.
# - handoff_turn_points: turn points matched across the sizes of a multisize piece.
# - handoff_sheet: piece bounds and the flat sheet, with no QGraphicsScene.
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
