// @file TestHandoffAnnotationAlignment.h
// @author slspencer
// @brief Tests the shared multisize annotation offset without application state.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

#ifndef TEST_HANDOFF_ANNOTATION_ALIGNMENT_H
#define TEST_HANDOFF_ANNOTATION_ALIGNMENT_H

#include <QObject>

class TestHandoffAnnotationAlignment : public QObject
{
    Q_OBJECT
private slots:
    void retainsFittingBaseOffset() const;
    void choosesNearestSharedOffset() const;
    void fitsEveryLabelAnchor() const;
    void keepsBaseOffsetWhenImpossible() const;
    void usesLowerFoldLineEndOnlyWhenEverySizeHasOne() const;
    void skipsMissingBaseAndHiddenComponents() const;
    void handlesConcaveOutlines() const;
    void includesBoundaryOnlyFits() const;
    void jacketCaptureFitsEverySize() const;
    void stackPointTakesLongestHorizontalEdge() const;
    void stackPointTakesLowestCornerWithoutRightAngle() const;
    void stackPointMapsClosingIndexToFirstVertex() const;
    void stackPointCentresWithoutTurnPoints() const;
    void stackPointTakesNearestPlaceInOtherSizes() const;
    void stackPointTurnsWithGrainFrame() const;
    void stackPointTakesLowerFoldLineEnd() const;
    void stackPointNeedsThreeVertices() const;
};

#endif
