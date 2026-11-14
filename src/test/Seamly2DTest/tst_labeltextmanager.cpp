/******************************************************************************
 **  @file   tst_labeltextmanager.cpp
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for LabelTextManager
 **
 **  @copyright
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
 **
 **  Seamly2D is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Seamly2D is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
 **
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **
 *****************************************************************************/

#include "tst_labeltextmanager.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/LabelTextManager.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vtools/dialogs/support/editlabeltemplate_dialog.h"

#include <QtTest>
#include <QFont>
#include <QFontMetrics>
#include <QListWidget>
#include <QTabWidget>

namespace
{
/// @brief Pattern document with no drafting tools, used to test label templates.
class TestPatternDocument : public VAbstractPattern
{
public:
    TestPatternDocument()
    {
        setContent(QStringLiteral("<pattern><measurements/></pattern>"));
    }

    void CreateEmptyFile() override {}
    void IncrementReferens(quint32) const override {}
    void DecrementReferens(quint32) const override {}
    QStringList GetCurrentAlphabet() const override { return QStringList(); }
    QString GenerateLabel(const LabelType &, const QString &) const override { return QString(); }
    QString generateSuffix(const QString &) const override { return QString(); }
    void UpdateToolData(const quint32 &, VContainer *) override {}
    void LiteParseTree(const Document &) override {}
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief lineWidth returns the pixel width of a label line at a base font size.
 */
int lineWidth(const QFont &font, const TextLine &line, int size)
{
    QFont fnt = font;
    fnt.setBold(line.bold);
    fnt.setItalic(line.italic);
    fnt.setPixelSize(size + line.font_size_increment);
    return QFontMetrics(fnt).horizontalAdvance(line.text);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void TST_LabelTextManager::fileNamePlaceholders_data() const
{
    QTest::addColumn<QString>("pattern_path");
    QTest::addColumn<QString>("measurements_path");
    QTest::addColumn<QString>("expected");

    QTest::newRow("extensions") << QStringLiteral("patterns/dress.sm2d") << QStringLiteral("../measurements/body.smis")
                               << QStringLiteral("dress.sm2d|body.smis|sm2d|smis");
    QTest::newRow("multiple dots") << QStringLiteral("patterns/dress.v2.sm2d")
                                  << QStringLiteral("measurements/body.v3.smms")
                                  << QStringLiteral("dress.v2.sm2d|body.v3.smms|sm2d|smms");
    QTest::newRow("no extension") << QStringLiteral("patterns/dress") << QStringLiteral("measurements/body")
                                 << QStringLiteral("dress|body||");
    QTest::newRow("empty paths") << QString() << QString() << QStringLiteral("|||");
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LabelTextManager::fileNamePlaceholders() const
{
    QFETCH(QString, pattern_path);
    QFETCH(QString, measurements_path);
    QFETCH(QString, expected);

    TestPatternDocument document;
    document.SetMPath(measurements_path);
    VLabelTemplateLine line{};
    line.line = QStringLiteral("%pFileName%|%mFileName%|%pExt%|%mExt%");
    document.setPatternLabelTemplate({line});
    VPieceLabelData piece_data;
    piece_data.SetLabelTemplate({line});

    const QString previous_path = qApp->getFilePath();
    VAbstractPattern *previous_document = qApp->getCurrentDocument();
    qApp->setFilePath(pattern_path);
    qApp->setCurrentDocument(&document);

    LabelTextManager pattern_label;
    pattern_label.Update(&document);
    LabelTextManager piece_label;
    piece_label.Update(QStringLiteral("Front"), piece_data);

    qApp->setCurrentDocument(previous_document);
    qApp->setFilePath(previous_path);

    QCOMPARE(pattern_label.GetSourceLinesCount(), 1);
    QCOMPARE(pattern_label.GetSourceLine(0).text, expected);
    QCOMPARE(piece_label.GetSourceLinesCount(), 1);
    QCOMPARE(piece_label.GetSourceLine(0).text, expected);

    qApp->setFilePath(pattern_path);
    EditLabelTemplateDialog editor(&document);
    qApp->setFilePath(previous_path);
    editor.SetTemplate({line});
    auto *tabs = editor.findChild<QTabWidget *>(QStringLiteral("tabWidget"));
    auto *preview = editor.findChild<QListWidget *>(QStringLiteral("listWidgetPreview"));
    auto *preview_tab = editor.findChild<QWidget *>(QStringLiteral("tabPreview"));
    QVERIFY(tabs != nullptr);
    QVERIFY(preview != nullptr);
    QVERIFY(preview_tab != nullptr);
    tabs->setCurrentWidget(preview_tab);
    QCOMPARE(preview->count(), 1);
    QCOMPARE(preview->item(0)->text(), expected);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief blankLineMarker checks that an "<empty>" line is a blank line with one line height in the label and preview.
 */
void TST_LabelTextManager::blankLineMarker() const
{
    VLabelTemplateLine first_line{};
    first_line.line = QStringLiteral("Front");
    VLabelTemplateLine blank_line{};
    blank_line.line = QStringLiteral("<empty>");
    blank_line.fontSizeIncrement = 2;
    VLabelTemplateLine last_line{};
    last_line.line = QStringLiteral("Cut 2");
    const QVector<VLabelTemplateLine> lines{first_line, blank_line, last_line};

    TestPatternDocument document;
    document.setPatternLabelTemplate(lines);
    VPieceLabelData piece_data;
    piece_data.SetLabelTemplate(lines);

    VAbstractPattern* previous_document = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    LabelTextManager pattern_label;
    pattern_label.Update(&document);
    LabelTextManager piece_label;
    piece_label.Update(QStringLiteral("Front"), piece_data);

    qApp->setCurrentDocument(previous_document);

    // The blank line keeps its place and its font size increment, so it takes one line height.
    for (const LabelTextManager* label : {&pattern_label, &piece_label})
    {
        QCOMPARE(label->GetSourceLinesCount(), 3);
        QCOMPARE(label->GetSourceLine(0).text, QStringLiteral("Front"));
        QCOMPARE(label->GetSourceLine(1).text, QString());
        QCOMPARE(label->GetSourceLine(1).font_size_increment, 2);
        QCOMPARE(label->GetSourceLine(2).text, QStringLiteral("Cut 2"));
    }

    EditLabelTemplateDialog editor(&document);
    editor.SetTemplate(lines);
    auto* tabs = editor.findChild<QTabWidget*>(QStringLiteral("tabWidget"));
    auto* preview = editor.findChild<QListWidget*>(QStringLiteral("listWidgetPreview"));
    auto* preview_tab = editor.findChild<QWidget*>(QStringLiteral("tabPreview"));
    QVERIFY(tabs != nullptr);
    QVERIFY(preview != nullptr);
    QVERIFY(preview_tab != nullptr);
    tabs->setCurrentWidget(preview_tab);
    QCOMPARE(preview->count(), 3);
    QCOMPARE(preview->item(1)->text(), QString());
}

//---------------------------------------------------------------------------------------------------------------------
TST_LabelTextManager::TST_LabelTextManager(QObject *parent)
    : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LabelTextManager::LargestFittingSize_data() const
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("increment");
    QTest::addColumn<bool>("bold");
    QTest::addColumn<bool>("italic");
    QTest::addColumn<int>("tooWideSize");
    QTest::addColumn<qreal>("width");

    QTest::newRow("plain line") << QStringLiteral("Fabric, Cut 1") << 0 << false << false << 47 << 100.0;
    QTest::newRow("bold line with increment") << QStringLiteral("Detail 1") << 2 << true << false << 47 << 80.0;
    QTest::newRow("italic line") << QStringLiteral("Interfacing, Cut 2 on fold") << 0 << false << true << 40
                                 << 150.0;
    QTest::newRow("nothing fits") << QStringLiteral("WWWWWWWWWW") << 6 << true << false << 20 << 10.0;
    QTest::newRow("start at the minimum") << QStringLiteral("Front") << 0 << false << false
                                          << LabelTextManager::min_font_size << 1.0;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LargestFittingSize checks that the result fits and the next size up does not.
 *
 * The search starts from an estimate instead of stepping down from the size that is too wide. It must still
 * return the same size as that step-down search: the largest one that fits, or LabelTextManager::min_font_size.
 */
void TST_LabelTextManager::LargestFittingSize() const
{
    QFETCH(QString, text);
    QFETCH(int, increment);
    QFETCH(bool, bold);
    QFETCH(bool, italic);
    QFETCH(int, tooWideSize);
    QFETCH(qreal, width);

    TextLine line;
    line.text = text;
    line.font_size_increment = increment;
    line.bold = bold;
    line.italic = italic;

    const QFont font;
    const int tooWideWidth = lineWidth(font, line, tooWideSize);
    QVERIFY2(tooWideWidth > width, "The row must start at a size that is too wide.");

    const int size = LabelTextManager::largestFittingSize(font, line, tooWideSize, tooWideWidth, width);

    QVERIFY(size >= LabelTextManager::min_font_size);
    QVERIFY(size <= qMax(LabelTextManager::min_font_size, tooWideSize - 1));
    if (size > LabelTextManager::min_font_size)
    {
        QVERIFY2(lineWidth(font, line, size) <= width, "The result must fit.");
    }
    if (size + 1 < tooWideSize)
    {
        QVERIFY2(lineWidth(font, line, size + 1) > width, "The next size up must not fit.");
    }
}
