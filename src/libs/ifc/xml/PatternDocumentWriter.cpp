//---------------------------------------------------------------------------------------------------------------------
// @file PatternDocumentWriter.cpp
// @author slspencer
// @brief Writes current-format document and draft-block scaffolding.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "PatternDocumentWriter.h"
#include "vpatternconverter.h"

void PatternDocumentWriter::createEmptyPattern(QDomDocument &document, Unit pattern_unit, const QString &comment)
{
    document.clear();
    QDomElement pattern = document.createElement(QStringLiteral("pattern"));
    pattern.appendChild(document.createComment(comment));

    QDomElement version = document.createElement(QStringLiteral("version"));
    version.appendChild(document.createTextNode(VPatternConverter::PatternMaxVerStr));
    pattern.appendChild(version);

    QDomElement unit = document.createElement(QStringLiteral("unit"));
    unit.appendChild(document.createTextNode(UnitsToStr(pattern_unit)));
    pattern.appendChild(unit);

    pattern.appendChild(document.createElement(QStringLiteral("description")));
    pattern.appendChild(document.createElement(QStringLiteral("notes")));
    pattern.appendChild(document.createElement(QStringLiteral("measurements")));
    pattern.appendChild(document.createElement(QStringLiteral("variables")));

    document.appendChild(pattern);
    document.insertBefore(document.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""),
                          document.firstChild());
}

QDomElement PatternDocumentWriter::createDraftImages(QDomDocument &document, QDomElement draft_block)
{
    if (draft_block.isNull())
    {
        return QDomElement();
    }

    QDomElement groups = draft_block.firstChildElement(QStringLiteral("groups"));
    QDomElement images = draft_block.firstChildElement(QStringLiteral("images"));
    if (groups.isNull())
    {
        groups = document.createElement(QStringLiteral("groups"));
        if (images.isNull())
        {
            draft_block.appendChild(groups);
        }
        else
        {
            draft_block.insertBefore(groups, images);
        }
    }
    if (images.isNull())
    {
        images = document.createElement(QStringLiteral("images"));
        draft_block.insertAfter(images, groups);
    }
    else if (images.previousSiblingElement() != groups)
    {
        draft_block.insertBefore(groups, images);
    }
    return images;
}
