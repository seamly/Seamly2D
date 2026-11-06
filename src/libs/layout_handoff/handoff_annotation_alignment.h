// @file handoff_annotation_alignment.h
// @author slspencer
// @brief Fits multisize annotation anchors at one offset from their stack points.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

#ifndef HANDOFF_ANNOTATION_ALIGNMENT_H
#define HANDOFF_ANNOTATION_ALIGNMENT_H

#include <QDomDocument>
#include <QStringList>

namespace layout_handoff
{
/**
 * @brief Aligns the generated handoff's multisize annotations without changing pattern data.
 * @details Supports the absolute polylines and matrix transforms emitted by HandoffDocument.
 *          Uses fold starts when every size has one, otherwise outline bounding-box centres.
 *          Each component keeps the base offset if it fits; otherwise uses the nearest shared offset.
 *          Hidden components and sets without a base size remain unchanged.
 * @return Components with no shared fit; these retain the base offset in every size.
 */
QStringList align_handoff_annotations(QDomDocument &document);
}

#endif
