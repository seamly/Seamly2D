//---------------------------------------------------------------------------------------------------------------------
//  @file   vtextmanager.cpp
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
//  @file   vtextmanager.cpp
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

#include <QDate>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHash>
#include <QLatin1String>
#include <QRegularExpression>
#include <QApplication>
#include <QDebug>

#include "../ifc/xml/vabstractpattern.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vmath.h"
#include "../vpatterndb/vcontainer.h"
#include "LabelTextManager.h"

//---------------------------------------------------------------------------------------------------------------------
/// @brief TextLine::TextLine default constructor
//---------------------------------------------------------------------------------------------------------------------
TextLine::TextLine()
    : m_text()
    , m_iFontSize(MIN_FONT_SIZE)
    , bold(false)
    , italic(false)
    , m_eAlign(Qt::AlignCenter)
{}

QList<TextLine> LabelTextManager::m_patternLabelLines = QList<TextLine>();

namespace
{

//---------------------------------------------------------------------------------------------------------------------
QMap<QString, QString> PreparePlaceholders(const VAbstractPattern *doc)
{
    SCASSERT(doc != nullptr)

    QMap<QString, QString> placeholders;

    // Pattern tags
    QLocale locale(qApp->Settings()->getLocale());

    const QString date = locale.toString(QDate::currentDate(), doc->GetLabelDateFormat());
    placeholders.insert(pl_date, date);

    const QString time = locale.toString(QTime::currentTime(), doc->GetLabelTimeFormat());
    placeholders.insert(pl_time, time);

    placeholders.insert(pl_patternName, doc->GetPatternName());
    placeholders.insert(pl_patternNumber, doc->GetPatternNumber());
    placeholders.insert(pl_author, doc->GetCompanyName());
    placeholders.insert(pl_customer, doc->GetCustomerName());
    placeholders.insert(pl_pExt, QFileInfo(qApp->getFilePath()).suffix());
    placeholders.insert(pl_pFileName, QFileInfo(qApp->getFilePath()).baseName());

    placeholders.insert(pl_mExt, QFileInfo(doc->MPath()).suffix());
    placeholders.insert(pl_mFileName, QFileInfo(doc->MPath()).baseName());

    QString curSize;
    QString curHeight;
    if (qApp->patternType() == MeasurementsType::Multisize)
    {
        curSize = QString::number(VContainer::size());
        curHeight = QString::number(VContainer::height());
    }
    else if (qApp->patternType() == MeasurementsType::Individual)
    {
        curSize = QString::number(VContainer::size());
        curHeight = QString::number(VContainer::height());
    }

    placeholders.insert(pl_size, curSize);
    placeholders.insert(pl_height, curHeight);

    // Piece tags
    placeholders.insert(pl_pLetter, "");
    placeholders.insert(pl_pAnnotation, "");
    placeholders.insert(pl_pOrientation, "");
    placeholders.insert(pl_pRotation, "");
    placeholders.insert(pl_pTilt, "");
    // Fold position is no longer a piece property; an old label template prints nothing for it.
    placeholders.insert(pl_pFoldPosition, "");
    placeholders.insert(pl_pName, "");
    placeholders.insert(pl_pQuantity, "");
    placeholders.insert(pl_wOnFold, "");
    placeholders.insert(pl_mFabric, QObject::tr("Fabric"));
    placeholders.insert(pl_mLining, QObject::tr("Lining"));
    placeholders.insert(pl_mInterfacing, QObject::tr("Interfacing"));
    placeholders.insert(pl_mInterlining, QObject::tr("Interlining"));
    placeholders.insert(pl_wCut, QObject::tr("Cut"));

    return placeholders;
}

//---------------------------------------------------------------------------------------------------------------------
void InitPiecePlaceholders(QMap<QString, QString> &placeholders, const QString &name, const VPieceLabelData& data)
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
QString ReplacePlaceholders(const QMap<QString, QString> &placeholders, QString line)
{
    QChar per('%');
    auto i = placeholders.constBegin();
    while (i != placeholders.constEnd())
    {
        line.replace(per+i.key()+per, i.value());
        ++i;
    }
    return line;
}

//---------------------------------------------------------------------------------------------------------------------
QList<TextLine> PrepareLines(const QVector<VLabelTemplateLine> &lines)
{
    QList<TextLine> textLines;

    for (int i=0; i < lines.size(); ++i)
    {
        if (not lines.at(i).line.isEmpty())
        {
            TextLine tl;
            tl.m_text = lines.at(i).line;
            tl.m_eAlign = static_cast<Qt::Alignment>(lines.at(i).alignment);
            tl.m_iFontSize = lines.at(i).fontSizeIncrement;
            tl.bold = lines.at(i).bold;
            tl.italic = lines.at(i).italic;

            textLines << tl;
        }
    }

    return textLines;
}
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief LabelTextManager::LabelTextManager constructor
//---------------------------------------------------------------------------------------------------------------------
LabelTextManager::LabelTextManager()
     : m_font()
     , m_liLines()
{}

//---------------------------------------------------------------------------------------------------------------------
LabelTextManager::LabelTextManager(const LabelTextManager &text)
    : m_font(text.GetFont())
    , m_liLines(text.GetAllSourceLines())
{}

//---------------------------------------------------------------------------------------------------------------------
LabelTextManager &LabelTextManager::operator=(const LabelTextManager &text)
{
    if ( &text == this )
    {
        return *this;
    }
    m_font = text.GetFont();
    m_liLines = text.GetAllSourceLines();
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
// @brief GetSpacing returns the vertical spacing between the lines
// @return spacing
//---------------------------------------------------------------------------------------------------------------------
int LabelTextManager::GetSpacing() const
{
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------
// @brief SetFont set the text base font
// @param font text base font
//---------------------------------------------------------------------------------------------------------------------
void LabelTextManager::setFont(const QFont& font)
{
    m_font = font;
}

//---------------------------------------------------------------------------------------------------------------------
// @brief GetFont returns the text base font
// @return text base font
//---------------------------------------------------------------------------------------------------------------------
const QFont& LabelTextManager::GetFont() const
{
    return m_font;
}

//---------------------------------------------------------------------------------------------------------------------
// @brief SetFontSize sets the font size
// @param iFS font size in pixels
//---------------------------------------------------------------------------------------------------------------------
void LabelTextManager::SetFontSize(int iFS)
{
    iFS < MIN_FONT_SIZE ? m_font.setPixelSize(MIN_FONT_SIZE) : m_font.setPixelSize(iFS);
}

//---------------------------------------------------------------------------------------------------------------------
QList<TextLine> LabelTextManager::GetAllSourceLines() const
{
    return m_liLines;
}

//---------------------------------------------------------------------------------------------------------------------
// @brief LabelTextManager::GetSourceLinesCount returns the number of input text lines
// @return number of text lines that were added to the list by calling AddLine
//---------------------------------------------------------------------------------------------------------------------
int LabelTextManager::GetSourceLinesCount() const
{
    return m_liLines.count();
}

//---------------------------------------------------------------------------------------------------------------------
// @brief LabelTextManager::GetSourceLine returns the reference to i-th text line
// @param i index of the requested line
// @return reference to the requested TextLine object
//---------------------------------------------------------------------------------------------------------------------
const TextLine& LabelTextManager::GetSourceLine(int i) const
{
    Q_ASSERT(i >= 0);
    Q_ASSERT(i < m_liLines.count());
    return m_liLines.at(i);
}

//---------------------------------------------------------------------------------------------------------------------
// @brief LabelTextManager::FitFontSize sets the font size just big enough, so that the text fits into rectangle of
// size (fW, fH)
// @param fW rectangle width
// @param fH rectangle height
//---------------------------------------------------------------------------------------------------------------------
void LabelTextManager::FitFontSize(qreal fW, qreal fH)
{
    // Every pixel size loads its own font engine, which is slow. A multisize layout fits the same lines into
    // the same box once for each size, so the result of each fit is kept. The fit sets every pixel size it
    // measures, so the key ignores the current one. Text lengths in the key keep two different line lists
    // from making the same key.
    thread_local QHash<QString, int> fittedSizes;
    QFont keyFont = m_font;
    keyFont.setPixelSize(MIN_FONT_SIZE);
    QString key = QString::number(fW, 'g', 17) + QLatin1Char(' ') + QString::number(fH, 'g', 17)
                  + QLatin1Char(' ') + keyFont.key();
    for (const TextLine &tl : m_liLines)
    {
        key += QLatin1Char(' ') + QString::number(tl.m_iFontSize) + QLatin1Char(tl.bold ? 'b' : '-')
               + QLatin1Char(tl.italic ? 'i' : '-') + QString::number(tl.m_text.size()) + QLatin1Char(':')
               + tl.m_text;
    }

    const auto fitted = fittedSizes.constFind(key);
    if (fitted != fittedSizes.constEnd())
    {
        SetFontSize(fitted.value());
        return;
    }

    int iFS = 0;
    if (GetSourceLinesCount() > 0)
    {//division by zero
        iFS = 3*qFloor(fH/GetSourceLinesCount())/4;
    }

    if (iFS < MIN_FONT_SIZE)
    {
        iFS = MIN_FONT_SIZE;
    }

    // get ratio between char width and height

    int iMaxLen = 0;
    TextLine maxLine;
    QFont fnt;
    for (int i = 0; i < GetSourceLinesCount(); ++i)
    {
        const TextLine& tl = GetSourceLine(i);
        fnt = m_font;
        fnt.setPixelSize(iFS + tl.m_iFontSize);
        fnt.setBold(tl.bold);
        fnt.setItalic(tl.italic);
        QFontMetrics fm(fnt);
        const int iTW = fm.horizontalAdvance(tl.m_text);
        if (iTW > iMaxLen)
        {
            iMaxLen = iTW;
            maxLine = tl;
        }
    }
    if (iMaxLen > fW)
    {
        iFS = largestFittingSize(m_font, maxLine, iFS, iMaxLen, fW);
    }

    if (fittedSizes.size() >= 1000)
    {
        fittedSizes.clear();
    }
    fittedSizes.insert(key, iFS);

    SetFontSize(iFS);
    qDebug() << "Font size" << GetSourceLinesCount() << iFS;
}

//---------------------------------------------------------------------------------------------------------------------
// @brief LabelTextManager::largestFittingSize returns the largest base font size below tooWideSize at which line fits
// into width.
// @param font font family and style; the line sets bold, italic and its own size increment
// @param tooWideSize base font size at which the line is wider than width
// @param tooWideWidth line width in pixels at tooWideSize
// @param width available width in pixels
// @return MIN_FONT_SIZE when no larger size fits
//---------------------------------------------------------------------------------------------------------------------
int LabelTextManager::largestFittingSize(const QFont &font, const TextLine &line, int tooWideSize, int tooWideWidth,
                                         qreal width)
{
    QFont fnt = font;
    fnt.setBold(line.bold);
    fnt.setItalic(line.italic);

    const auto lineWidth = [&fnt, &line](int size)
    {
        fnt.setPixelSize(size + line.m_iFontSize);
        return QFontMetrics(fnt).horizontalAdvance(line.m_text);
    };

    // Each pixel size loads its own font engine, which is slow. The line width grows about linearly with the
    // pixel size, so start at the estimated size and step from there, not down from tooWideSize.
    int size = tooWideSize - 1;
    if (tooWideWidth > 0)
    {
        size = qFloor(width * (tooWideSize + line.m_iFontSize) / tooWideWidth) - line.m_iFontSize;
    }
    size = qMax(MIN_FONT_SIZE, qMin(size, tooWideSize - 1));

    while (size > MIN_FONT_SIZE && lineWidth(size) > width)
    {
        --size;
    }
    while (size + 1 < tooWideSize && lineWidth(size + 1) <= width)
    {
        ++size;
    }
    return size;
}

//---------------------------------------------------------------------------------------------------------------------
// @brief LabelTextManager::Update updates the text lines with detail data
// @param qsName detail name
// @param data reference to the detail data
//---------------------------------------------------------------------------------------------------------------------
void LabelTextManager::Update(const QString& qsName, const VPieceLabelData& data)
{
    m_liLines.clear();

    QMap<QString, QString> placeholders = PreparePlaceholders(qApp->getCurrentDocument());
    InitPiecePlaceholders(placeholders, qsName, data);

    QVector<VLabelTemplateLine> lines = data.GetLabelTemplate();

    for (int i=0; i<lines.size(); ++i)
    {
        lines[i].line = ReplacePlaceholders(placeholders, lines.at(i).line);
    }

    m_liLines = PrepareLines(lines);
}

//---------------------------------------------------------------------------------------------------------------------
// @brief LabelTextManager::Update updates the text lines with pattern info
// @param pDoc pointer to the abstract pattern object
//---------------------------------------------------------------------------------------------------------------------
void LabelTextManager::Update(VAbstractPattern *pDoc)
{
    m_liLines.clear();

    if (m_patternLabelLines.isEmpty() || pDoc->GetPatternWasChanged())
    {
        QVector<VLabelTemplateLine> lines = pDoc->getPatternLabelTemplate();
        if (lines.isEmpty() && m_patternLabelLines.isEmpty())
        {
            return; // Nothing to parse
        }

        const QMap<QString, QString> placeholders = PreparePlaceholders(pDoc);

        for (int i=0; i<lines.size(); ++i)
        {
            lines[i].line = ReplacePlaceholders(placeholders, lines.at(i).line);
        }

        pDoc->SetPatternWasChanged(false);
        m_patternLabelLines = PrepareLines(lines);
    }

    m_liLines = m_patternLabelLines;
}
