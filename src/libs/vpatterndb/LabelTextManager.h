//---------------------------------------------------------------------------------------------------------------------
//  @file   LabelTextManager.h
//  @author Bojan Kverh
//  @date   July 19, 2016
//
//  @copyright
//  This source code is part of the Valentine project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2013-2015 Seamly2D project
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

//---------------------------------------------------------------------------------------------------------------------
//  @file   LabelTextManager.h
//  @author slspencer
//
//  @brief
//  Text lines of a piece label or the pattern label, and the font size that fits them into the label.
//
//  @copyright
//  Copyright (C) 2026 Seamly2D Project
//  https://github.com/fashionfreedom/seamly2d
//
//  SPDX-License-Identifier: GPL-3.0-or-later
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
//  along with Seamly2D. If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

#ifndef LABELTEXTMANAGER_H
#define LABELTEXTMANAGER_H

#include <QFont>
#include <QList>
#include <QString>
#include <Qt>
#include <QtGlobal>

class VPieceLabelData;
class VAbstractPattern;

/**
 * @brief TextLine is one line of label text with its font style.
 */
struct TextLine
{
    TextLine();

    QString       text;                ///< Line text, with the placeholders replaced.
    int           font_size_increment; ///< Pixels added to the label base font size for this line.
    bool          bold;                ///< True for bold text.
    bool          italic;              ///< True for italic text.
    Qt::Alignment alignment;           ///< Horizontal alignment of the line in the label.
};

/**
 * @brief LabelTextManager holds the text lines of a label and finds the font size that fits them into the label.
 */
class LabelTextManager
{
public:
    /// @brief Smallest base font size, in pixels.
    static constexpr int min_font_size = 5;

    LabelTextManager();
    virtual ~LabelTextManager() = default;

    LabelTextManager(const LabelTextManager& text);
    LabelTextManager& operator=(const LabelTextManager& text);

    virtual int GetSpacing() const;

    void         setFont(const QFont& font);
    const QFont& GetFont() const;
    void         SetFontSize(int font_size);
    void         FitFontSize(qreal width, qreal height);

    static int largestFittingSize(const QFont& font, const TextLine& line, int too_wide_size, int too_wide_width,
                                  qreal width);

    QList<TextLine> GetAllSourceLines() const;
    int             GetSourceLinesCount() const;
    const TextLine& GetSourceLine(int index) const;

    void Update(const QString& name, const VPieceLabelData& data);
    void Update(VAbstractPattern* document);

private:
    QFont           m_font;  ///< Base font of the label text.
    QList<TextLine> m_lines; ///< Label text lines.

    static QList<TextLine> m_pattern_label_lines; ///< Pattern label lines, kept until the pattern changes.
};

#endif // LABELTEXTMANAGER_H
