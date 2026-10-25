//---------------------------------------------------------------------------------------------------------------------
//  @file   LabelTextManager.cpp
//  @author Douglas S Caskey
//  @date   1 Mar, 2026
//
//  @copyright
//  Copyright (C) 2017 - 2026 Seamly, LLC
//  https://github.com/fashionfreedom/seamly2d
//
//  @brief
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
//  along with Seamly2D. if not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
//  @file   LabelTextManager.cpp
//  @author Bojan Kverh
//  @date   July 19, 2016
//
//  @brief
//  @copyright
//  This source code is part of the Valentina project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2017 Valentina project
//  <https://bitbucket.org/dismine/valentina> All Rights Reserved.
//
//  Valentina is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Valentina is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
//  @file   LabelTextManager.cpp
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

#include "LabelTextManager.h"

#include <QDate>
#include <QDebug>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHash>
#include <QLatin1String>
#include <QLocale>
#include <QMap>
#include <QTime>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vmath.h"
#include "floatItemData/vpiecelabeldata.h"
#include "vcontainer.h"

namespace
{
/// @brief Fit results kept before the cache starts over.
constexpr int max_fitted_sizes = 1000;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief preparePlaceholders maps each label placeholder to its pattern value; piece placeholders stay empty.
 * @param document pattern; must not be null.
 */
QMap<QString, QString> preparePlaceholders(const VAbstractPattern* document)
{
    SCASSERT(document != nullptr)

    QMap<QString, QString> placeholders;

    // Pattern tags
    const QLocale locale(qApp->Settings()->getLocale());
    placeholders.insert(pl_date, locale.toString(QDate::currentDate(), document->GetLabelDateFormat()));
    placeholders.insert(pl_time, locale.toString(QTime::currentTime(), document->GetLabelTimeFormat()));

    placeholders.insert(pl_patternName, document->GetPatternName());
    placeholders.insert(pl_patternNumber, document->GetPatternNumber());
    placeholders.insert(pl_author, document->GetCompanyName());
    placeholders.insert(pl_customer, document->GetCustomerName());
    placeholders.insert(pl_pExt, QFileInfo(qApp->getFilePath()).suffix());
    placeholders.insert(pl_pFileName, QFileInfo(qApp->getFilePath()).baseName());

    placeholders.insert(pl_mExt, QFileInfo(document->MPath()).suffix());
    placeholders.insert(pl_mFileName, QFileInfo(document->MPath()).baseName());

    // Size and height are known only when the pattern has measurements.
    QString current_size;
    QString current_height;
    if (qApp->patternType() == MeasurementsType::Multisize || qApp->patternType() == MeasurementsType::Individual)
    {
        current_size = QString::number(VContainer::size());
        current_height = QString::number(VContainer::height());
    }
    placeholders.insert(pl_size, current_size);
    placeholders.insert(pl_height, current_height);

    // Piece tags
    placeholders.insert(pl_pLetter, QString());
    placeholders.insert(pl_pAnnotation, QString());
    placeholders.insert(pl_pOrientation, QString());
    placeholders.insert(pl_pRotation, QString());
    placeholders.insert(pl_pTilt, QString());
    // Fold position is no longer a piece property; an old label template prints nothing for it.
    placeholders.insert(pl_pFoldPosition, QString());
    placeholders.insert(pl_pName, QString());
    placeholders.insert(pl_pQuantity, QString());
    placeholders.insert(pl_wOnFold, QString());
    placeholders.insert(pl_mFabric, QObject::tr("Fabric"));
    placeholders.insert(pl_mLining, QObject::tr("Lining"));
    placeholders.insert(pl_mInterfacing, QObject::tr("Interfacing"));
    placeholders.insert(pl_mInterlining, QObject::tr("Interlining"));
    placeholders.insert(pl_wCut, QObject::tr("Cut"));

    return placeholders;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief initPiecePlaceholders fills the piece placeholders from the piece name and its label data.
 */
void initPiecePlaceholders(QMap<QString, QString>& placeholders, const QString& name, const VPieceLabelData& data)
{
    placeholders[pl_pLetter] = data.GetLetter();
    placeholders[pl_pAnnotation] = data.GetAnnotation();
    placeholders[pl_pOrientation] = data.GetOrientation();
    placeholders[pl_pRotation] = VPieceLabelData::rotationDisplayName(data.getRotationWay());
    placeholders[pl_pTilt] = VPieceLabelData::tiltDisplayText(data.GetTilt(), data.getTiltDegrees());
    placeholders[pl_pName] = name;
    placeholders[pl_pQuantity] = QString::number(data.GetQuantity());

    if (data.IsOnFold())
    {
        placeholders[pl_wOnFold] = QObject::tr("on fold");
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief replacePlaceholders replaces each %placeholder% in a line with its value.
 */
QString replacePlaceholders(const QMap<QString, QString>& placeholders, QString line)
{
    const QChar percent('%');
    for (auto i = placeholders.constBegin(); i != placeholders.constEnd(); ++i)
    {
        line.replace(percent + i.key() + percent, i.value());
    }
    return line;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief prepareLines makes a text line of each template line that is not empty.
 */
QList<TextLine> prepareLines(const QVector<VLabelTemplateLine>& lines)
{
    QList<TextLine> text_lines;
    for (const VLabelTemplateLine& template_line : lines)
    {
        if (!template_line.line.isEmpty())
        {
            TextLine text_line;
            text_line.text = template_line.line;
            text_line.alignment = static_cast<Qt::Alignment>(template_line.alignment);
            text_line.font_size_increment = template_line.fontSizeIncrement;
            text_line.bold = template_line.bold;
            text_line.italic = template_line.italic;
            text_lines << text_line;
        }
    }
    return text_lines;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief fitKey returns the cache key of a font fit: the box size, the font, and every line with its style.
 *
 * The fit sets every pixel size it measures, so the key ignores the current one. Text lengths in the key keep two
 * different line lists from making the same key.
 */
QString fitKey(const QFont& font, const QList<TextLine>& lines, qreal width, qreal height)
{
    QFont key_font = font;
    key_font.setPixelSize(LabelTextManager::min_font_size);
    QString key = QString::number(width, 'g', 17) + QLatin1Char(' ') + QString::number(height, 'g', 17)
                  + QLatin1Char(' ') + key_font.key();
    for (const TextLine& line : lines)
    {
        key += QLatin1Char(' ') + QString::number(line.font_size_increment) + QLatin1Char(line.bold ? 'b' : '-')
               + QLatin1Char(line.italic ? 'i' : '-') + QString::number(line.text.size()) + QLatin1Char(':')
               + line.text;
    }
    return key;
}
} // namespace

QList<TextLine> LabelTextManager::m_pattern_label_lines = QList<TextLine>();

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief TextLine makes an empty, centered line.
 */
TextLine::TextLine()
    : text()
    , font_size_increment(LabelTextManager::min_font_size)
    , bold(false)
    , italic(false)
    , alignment(Qt::AlignCenter)
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LabelTextManager makes a manager with no lines and the default font.
 */
LabelTextManager::LabelTextManager()
    : m_font()
    , m_lines()
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LabelTextManager copies the font and the lines of another manager.
 */
LabelTextManager::LabelTextManager(const LabelTextManager& text)
    : m_font(text.GetFont())
    , m_lines(text.GetAllSourceLines())
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief operator= copies the font and the lines of another manager.
 */
LabelTextManager& LabelTextManager::operator=(const LabelTextManager& text)
{
    if (&text != this)
    {
        m_font = text.GetFont();
        m_lines = text.GetAllSourceLines();
    }
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSpacing returns the vertical spacing between the lines, in pixels.
 */
int LabelTextManager::GetSpacing() const
{
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setFont sets the base font of the label text.
 */
void LabelTextManager::setFont(const QFont& font)
{
    m_font = font;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetFont returns the base font of the label text.
 */
const QFont& LabelTextManager::GetFont() const
{
    return m_font;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetFontSize sets the base font size in pixels; a size below min_font_size sets min_font_size.
 */
void LabelTextManager::SetFontSize(int font_size)
{
    m_font.setPixelSize(font_size < min_font_size ? min_font_size : font_size);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetAllSourceLines returns the label text lines.
 */
QList<TextLine> LabelTextManager::GetAllSourceLines() const
{
    return m_lines;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSourceLinesCount returns the number of label text lines.
 */
int LabelTextManager::GetSourceLinesCount() const
{
    return m_lines.count();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSourceLine returns one label text line.
 * @param index line index; must be in range.
 */
const TextLine& LabelTextManager::GetSourceLine(int index) const
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < m_lines.count());
    return m_lines.at(index);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FitFontSize sets the base font size just big enough for the lines to fit into a width by height box.
 *
 * Every pixel size loads its own font engine, which is slow. A multisize layout fits the same lines into the same
 * box once for each size, so the result of each fit is kept.
 */
void LabelTextManager::FitFontSize(qreal width, qreal height)
{
    thread_local QHash<QString, int> fitted_sizes;
    const QString key = fitKey(m_font, m_lines, width, height);

    int font_size = 0;
    const auto fitted = fitted_sizes.constFind(key);
    if (fitted != fitted_sizes.constEnd())
    {
        font_size = fitted.value();
    }
    else
    {
        // Start with the size that fits the line count into the height.
        if (GetSourceLinesCount() > 0)
        {
            font_size = 3 * qFloor(height / GetSourceLinesCount()) / 4;
        }
        if (font_size < min_font_size)
        {
            font_size = min_font_size;
        }

        // Find the widest line at that size.
        int max_width = 0;
        TextLine widest_line;
        for (const TextLine& line : m_lines)
        {
            QFont line_font = m_font;
            line_font.setPixelSize(font_size + line.font_size_increment);
            line_font.setBold(line.bold);
            line_font.setItalic(line.italic);
            const int line_width = QFontMetrics(line_font).horizontalAdvance(line.text);
            if (line_width > max_width)
            {
                max_width = line_width;
                widest_line = line;
            }
        }

        // Shrink the font until the widest line fits.
        if (max_width > width)
        {
            font_size = largestFittingSize(m_font, widest_line, font_size, max_width, width);
        }

        if (fitted_sizes.size() >= max_fitted_sizes)
        {
            fitted_sizes.clear();
        }
        fitted_sizes.insert(key, font_size);
        qDebug() << "Font size" << GetSourceLinesCount() << font_size;
    }

    SetFontSize(font_size);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief largestFittingSize returns the largest base font size below too_wide_size at which a line fits a width.
 * @param font           font family and style; the line sets bold, italic and its own size increment.
 * @param too_wide_size  base font size at which the line is wider than width.
 * @param too_wide_width line width in pixels at too_wide_size.
 * @param width          available width in pixels.
 * @return min_font_size when no larger size fits.
 */
int LabelTextManager::largestFittingSize(const QFont& font, const TextLine& line, int too_wide_size,
                                         int too_wide_width, qreal width)
{
    QFont line_font = font;
    line_font.setBold(line.bold);
    line_font.setItalic(line.italic);

    const auto line_width = [&line_font, &line](int size)
    {
        line_font.setPixelSize(size + line.font_size_increment);
        return QFontMetrics(line_font).horizontalAdvance(line.text);
    };

    // Each pixel size loads its own font engine, which is slow. The line width grows about linearly with the
    // pixel size, so start at the estimated size and step from there, not down from too_wide_size.
    int size = too_wide_size - 1;
    if (too_wide_width > 0)
    {
        size = qFloor(width * (too_wide_size + line.font_size_increment) / too_wide_width) - line.font_size_increment;
    }
    size = qMax(min_font_size, qMin(size, too_wide_size - 1));

    while (size > min_font_size && line_width(size) > width)
    {
        --size;
    }
    while (size + 1 < too_wide_size && line_width(size + 1) <= width)
    {
        ++size;
    }
    return size;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Update sets the lines from the piece label template, with the placeholders replaced.
 * @param name piece name.
 * @param data piece label data.
 */
void LabelTextManager::Update(const QString& name, const VPieceLabelData& data)
{
    QMap<QString, QString> placeholders = preparePlaceholders(qApp->getCurrentDocument());
    initPiecePlaceholders(placeholders, name, data);

    QVector<VLabelTemplateLine> lines = data.GetLabelTemplate();
    for (VLabelTemplateLine& line : lines)
    {
        line.line = replacePlaceholders(placeholders, line.line);
    }

    m_lines = prepareLines(lines);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Update sets the lines from the pattern label template, with the placeholders replaced.
 *
 * The pattern label lines are kept for every manager until the pattern changes.
 * @param document pattern; its changed flag is cleared after a new parse.
 */
void LabelTextManager::Update(VAbstractPattern* document)
{
    m_lines.clear();

    if (m_pattern_label_lines.isEmpty() || document->GetPatternWasChanged())
    {
        QVector<VLabelTemplateLine> lines = document->getPatternLabelTemplate();

        // An empty template with no kept lines leaves nothing to parse.
        if (!lines.isEmpty() || !m_pattern_label_lines.isEmpty())
        {
            const QMap<QString, QString> placeholders = preparePlaceholders(document);
            for (VLabelTemplateLine& line : lines)
            {
                line.line = replacePlaceholders(placeholders, line.line);
            }

            document->SetPatternWasChanged(false);
            m_pattern_label_lines = prepareLines(lines);
        }
    }

    m_lines = m_pattern_label_lines;
}
