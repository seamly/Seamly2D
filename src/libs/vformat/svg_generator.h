//---------------------------------------------------------------------------------------------------------------------
//  @file   svg_generator.h
//  @author Evans PERRET
//  @date   21 Sep, 2024
//
//  @brief
//  Custom SVG generator to handle groups in SVGs
//
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2013-2026 Seamly2D project
//  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
//
//  Seamly2D is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Seamly2D is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

#ifndef SVG_GENERATOR_H
#define SVG_GENERATOR_H

#include <QDomDocument>
#include <QGraphicsScene>
#include <QSet>
#include <QStringList>


class SvgGenerator
{
public:
                        SvgGenerator(QGraphicsRectItem *paper, QString name, QString pattern_name,
                                     QString description, int resolution);
    void                setMultisize(const QStringList &sizes, const QString &base_size,
                                     const QString &height = QString());
    void                setSampleSize(const QString &sample_size);
    void                addSvgFromScene(QGraphicsScene *scene, QGraphicsItem *item = nullptr,
                                        const QString &size = QString(), const QString &piece_set_key = QString());
    void                generate();
    QString             toSvgString();

private:
    QDomDocument        renderSceneToDom(QGraphicsScene *scene);
    void                addComponentGroups(QGraphicsScene *scene, QGraphicsItem *item, QDomDocument &piece_doc,
                                           QDomElement &piece_group, const QString &piece_id);
    QDomDocument        mergeSvgDoms();

    void                removeEmptyGroups(QDomElement &main_group);
    void                removeEmptyPaths(QDomElement &main_group);
    void                cleanSvg(QDomElement &main_group);
    void                setAttribute(QDomElement element, const QString &attr, const QString &value);

    QGraphicsRectItem  *m_paper;
    QString             m_filepath;
    QString             m_pattern_name;       ///< pattern name, written as data-name on the pattern group
    QString             m_description;
    int                 m_resolution;
    int                 m_piece_count;        ///< number of pieces added so far; provides data-type-number for pieces
    QSet<QString>       m_used_ids;           ///< every component id emitted so far; guards uniqueness across pieces
    QSet<QString>       m_used_piece_ids;     ///< every piece id emitted so far; guards uniqueness across pieces

    QStringList         m_sizes;              ///< multisize sizes in pattern units; empty for an individual pattern
    QString             m_base_size;          ///< base size of the multisize measurements, in pattern units
    QString             m_height;             ///< height of every size of a multisize export; empty when unknown
    QString             m_sample_size;        ///< individual pattern size in pattern units; empty when unknown

    QList<QDomDocument> m_dom_list;
    QList<QString>      m_dom_piece_set_keys; ///< piece-set key of each m_dom_list entry; empty = no piece-set
    QList<QString>      m_dom_materials;      ///< material of each m_dom_list entry; empty for a whole-scene entry
};

#endif // SVG_GENERATOR_H
