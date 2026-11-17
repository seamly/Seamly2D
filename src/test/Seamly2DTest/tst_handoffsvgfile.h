//---------------------------------------------------------------------------------------------------------------------
// @file tst_handoffsvgfile.h
// @author slspencer
// @brief Tests the default file name and the UTF-8 file write of the Export Pieces SVG.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#ifndef TST_HANDOFF_SVG_FILE_H
#define TST_HANDOFF_SVG_FILE_H

#include <QObject>

/**
 * @brief TST_HandoffSvgFile tests layout_handoff::pieces_svg_file_name() and layout_handoff::write_svg_file().
 * @details Every write goes to a QTemporaryDir.
 */
class TST_HandoffSvgFile : public QObject
{
    Q_OBJECT
private slots:
    void fileNameHasPatternNameAndTime_data() const;
    void fileNameHasPatternNameAndTime() const;
    void fileHoldsTheSvgAsUtf8() const;
    void writeErrorReturnsFalse() const;
    void generatedDocumentIsWrittenWithoutChanges() const;
};

#endif // TST_HANDOFF_SVG_FILE_H
