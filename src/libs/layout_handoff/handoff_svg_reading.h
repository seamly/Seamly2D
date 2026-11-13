// @file handoff_svg_reading.h
// @author slspencer
// @brief Reads tagged groups, matrix transforms, path vertices and text anchors back from the handoff SVG.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

#ifndef HANDOFF_SVG_READING_H
#define HANDOFF_SVG_READING_H

#include <QDomElement>
#include <QPointF>
#include <QString>
#include <QTransform>
#include <QVector>

namespace layout_handoff
{
/**
 * @brief Finds the first direct child of @p parent with this `data-type`.
 * @return the child; a null element when no child has the type.
 */
QDomElement find_tagged_child(const QDomElement& parent, const QString& type);

/**
 * @brief Reads the `matrix(a,b,c,d,e,f)` transform that HandoffDocument writes on a group.
 * @return the transform; identity when the element has no matrix transform.
 */
QTransform read_matrix(const QDomElement& element);

/**
 * @brief Appends the path vertices or the text anchors in the subtree of @p element, in document order.
 * @details Child groups with their own `data-type` are skipped. The path data must be the absolute polylines
 *          that HandoffDocument writes: each number pair is one vertex.
 * @param element the element to read.
 * @param parent  transform of the parent of @p element.
 * @param text    true reads the (x, y) anchor of each `text` element; false reads the vertices of `path` elements.
 * @param points  receives the points, with the transforms down to them applied.
 */
void collect_points(const QDomElement& element, const QTransform& parent, bool text, QVector<QPointF>& points);
} // namespace layout_handoff

#endif // HANDOFF_SVG_READING_H
