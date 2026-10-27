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

# Settings shared by every project.
message("Entering layout_handoff.pro")
include(../../../common.pri)

QT += core gui widgets printsupport xml

TARGET = layout_handoff

TEMPLATE = lib

CONFIG += staticlib

# The library has no stable.h, so it opts out of the release precompiled header from common.pri.
CONFIG -= precompile_header

include(warnings.pri)
include(layout_handoff.pri)

# Static library: the linker puts it into the apps, so it has no install rule

# Library file directory
DESTDIR = bin

# Generated moc files directory
MOC_DIR = moc

# Object files directory
OBJECTS_DIR = obj

# Generated rcc files directory
RCC_DIR = rcc

# No effect: default_post.prf adds -O2 after this -O3, and GCC and Clang use the last -O flag.
CONFIG(release, debug|release){
    !*msvc* {
        QMAKE_CXXFLAGS -= -O2
        QMAKE_CXXFLAGS += -O3
    }
}

include (../libs.pri)
