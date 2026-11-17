//---------------------------------------------------------------------------------------------------------------------
// @file tst_handoffsvgfile.cpp
// @author slspencer
// @brief Tests the default file name and the UTF-8 file write of the Export Pieces SVG.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "tst_handoffsvgfile.h"
#include "../layout_handoff/handoff_svg_file.h"
#include "../layout_handoff/handoff_document.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vgeometry/vpointf.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTime>
#include <QtTest>

namespace
{
/**
 * @brief read_file_bytes returns the bytes of a file.
 * @param file_path file to read.
 * @return the file bytes; empty when the file cannot be opened.
 */
QByteArray read_file_bytes(const QString& file_path)
{
    QByteArray bytes;
    QFile file(file_path);
    if (file.open(QIODevice::ReadOnly))
    {
        bytes = file.readAll();
    }
    return bytes;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffSvgFile::fileNameHasPatternNameAndTime_data() const
{
    QTest::addColumn<QString>("pattern_base_name");
    QTest::addColumn<QDateTime>("date_time");
    QTest::addColumn<QString>("expected");

    QTest::newRow("saved pattern") << QStringLiteral("shirt")
                                   << QDateTime(QDate(2026, 11, 15), QTime(9, 5))
                                   << QStringLiteral("shirt_pieces_2611150905.svg");
    QTest::newRow("new pattern") << QStringLiteral("unnamed")
                                 << QDateTime(QDate(2027, 1, 2), QTime(23, 59, 58))
                                 << QStringLiteral("unnamed_pieces_2701022359.svg");
    QTest::newRow("name with space") << QStringLiteral("summer dress")
                                     << QDateTime(QDate(2030, 6, 30), QTime(0, 0))
                                     << QStringLiteral("summer dress_pieces_3006300000.svg");
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffSvgFile::fileNameHasPatternNameAndTime() const
{
    QFETCH(QString, pattern_base_name);
    QFETCH(QDateTime, date_time);
    QFETCH(QString, expected);

    QCOMPARE(layout_handoff::pieces_svg_file_name(pattern_base_name, date_time), expected);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffSvgFile::fileHoldsTheSvgAsUtf8() const
{
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString file_path = QDir(temp_dir.path()).filePath(QStringLiteral("shirt_pieces_2611150905.svg"));

    const QString first_svg = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                             "<svg xmlns=\"http://www.w3.org/2000/svg\">"
                                             "<g data-type=\"piece\" data-name=\"Front\"/>"
                                             "<g data-type=\"piece\" data-name=\"Back\"/></svg>\n");
    QVERIFY(layout_handoff::write_svg_file(file_path, first_svg));
    QCOMPARE(read_file_bytes(file_path), first_svg.toUtf8());

    // A shorter document replaces the file completely; a non-ASCII piece name keeps its UTF-8 bytes.
    const QString second_svg = QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\">"
                                              "<g data-type=\"piece\" data-name=\"Ärmel\"/></svg>\n");
    QVERIFY(layout_handoff::write_svg_file(file_path, second_svg));
    const QByteArray second_bytes = read_file_bytes(file_path);
    QCOMPARE(second_bytes, second_svg.toUtf8());
    QCOMPARE(QString::fromUtf8(second_bytes), second_svg);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffSvgFile::writeErrorReturnsFalse() const
{
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    const QString file_path = QDir(temp_dir.path()).filePath(QStringLiteral("missing_folder/pieces.svg"));

    QVERIFY(!layout_handoff::write_svg_file(file_path, QStringLiteral("<svg/>")));
    QVERIFY(!QFile::exists(file_path));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Checks that individual and multisize documents keep their exact UTF-8 bytes on disk.
void TST_HandoffSvgFile::generatedDocumentIsWrittenWithoutChanges() const
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const Unit unit = Unit::Mm;
    VContainer data(nullptr, &unit);
    data.UpdateGObject(1, new VPointF(0, 0, "A", 0, 0));
    data.UpdateGObject(2, new VPointF(400, 0, "B", 0, 0));
    data.UpdateGObject(3, new VPointF(250, 300, "C", 0, 0));
    VPiece piece;
    for (quint32 id : {1U, 2U, 3U})
    {
        piece.GetPath().Append(VPieceNode(id, Tool::NodePoint));
    }
    piece.SetName(QStringLiteral("Ärmel"));
    piece.GetPatternPieceData().SetVisible(false);
    piece.GetPatternInfo().SetVisible(false);
    piece.GetGrainlineGeometry().SetVisible(false);
    for (bool multisize : {false, true})
    {
        layout_handoff::HandoffDocument document;
        document.set_sheet(QRectF(0, 0, 500, 500), 96);
        if (multisize)
        {
            document.set_multisize({"40", "42"}, "40", "168");
        }
        for (const QString& size : (multisize ? QStringList{"40", "42"} : QStringList{QString()}))
        {
            layout_handoff::HandoffPlacedPiece placed;
            placed.info = layout_handoff::HandoffPieceInfo::create(piece, size, multisize ? "1" : "");
            placed.piece = layout_handoff::HandoffPiece::create(piece, &data);
            document.add_piece(placed);
        }
        const QString svg = document.to_svg_string();
        QVERIFY(svg.contains(QStringLiteral("data-type=\"piece\"")));
        const QString path = QDir(directory.path()).filePath(QStringLiteral("pieces.svg"));
        QVERIFY(layout_handoff::write_svg_file(path, svg));
        QCOMPARE(read_file_bytes(path), svg.toUtf8());
        QDomDocument parsed;
        QVERIFY(parsed.setContent(read_file_bytes(path)));
        QCOMPARE(parsed.elementsByTagName(QStringLiteral("svg")).size(), 1);
    }
}
