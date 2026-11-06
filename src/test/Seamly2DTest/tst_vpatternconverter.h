//---------------------------------------------------------------------------------------------------------------------
//  @file   tst_vpatternconverter.h
//  @author slspencer
//  @brief  Tests schema conversion for pattern documents.
//  @copyright
//  Copyright (C) 2026 Seamly2D Project
//  License: GPL-3.0-or-later
//  <https://www.gnu.org/licenses/gpl-3.0.html>
//---------------------------------------------------------------------------------------------------------------------

#ifndef TST_VPATTERNCONVERTER_H
#define TST_VPATTERNCONVERTER_H

#include <QObject>

class TST_VPatternConverter : public QObject
{
    Q_OBJECT
private slots:
    void samplePatternsConvertToCurrentSchema();
    void repairsGroupsAfterImages();
    void addsMissingGroupsAndImages();
};

#endif // TST_VPATTERNCONVERTER_H
