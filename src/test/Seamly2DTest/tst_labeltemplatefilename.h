//---------------------------------------------------------------------------------------------------------------------
// @file tst_labeltemplatefilename.h
// @author slspencer
// @brief Tests label template source-file display.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#ifndef TST_LABELTEMPLATEFILENAME_H
#define TST_LABELTEMPLATEFILENAME_H

#include <QObject>

class TST_LabelTemplateFileName : public QObject
{
    Q_OBJECT
private slots:
    void selectedFile();
    void defaultFile();
    void noFile();
    void builtInFile();
    void insertedPlaceholderReplacesEmptyLine();
};

#endif // TST_LABELTEMPLATEFILENAME_H
