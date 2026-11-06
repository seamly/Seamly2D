//---------------------------------------------------------------------------------------------------------------------
// @file tst_patternpiececopy.cpp
// @author slspencer
// @brief Tests independent piece reconstruction and live draft references.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "tst_patternpiececopy.h"
#include "xml/PatternPieceCopy.h"
#include "xml/vpatternconverter.h"
#include "xml/vdomdocument.h"
#include "exception/vexception.h"
#include "../../libs/vtools/undocommands/CopyPatternPiece.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <QUndoStack>

namespace
{
class CopyTestDocument : public VAbstractPattern
{
public:
    void CreateEmptyFile() override {}
    void IncrementReferens(quint32) const override {}
    void DecrementReferens(quint32) const override {}
    QStringList GetCurrentAlphabet() const override { return QStringList(); }
    QString GenerateLabel(const LabelType &, const QString &) const override { return QString(); }
    QString generateSuffix(const QString &) const override { return QString(); }
    void UpdateToolData(const quint32 &, VContainer *) override {}
    void LiteParseTree(const Document &) override {}
};

void appendCopy(QDomDocument &document, const PatternPieceCopyResult &copy)
{
    const QDomNodeList blocks = document.elementsByTagName(QStringLiteral("draftBlock"));
    for (int i = 0; i < blocks.size(); ++i)
    {
        const QDomElement block = blocks.at(i).toElement();
        for (const CopiedModelingElement &modeling : copy.modeling)
        {
            if (modeling.block_name == block.attribute(QStringLiteral("name")))
            {
                block.firstChildElement(QStringLiteral("modeling")).appendChild(modeling.element);
            }
        }
        if (copy.block_name == block.attribute(QStringLiteral("name")))
        {
            block.firstChildElement(QStringLiteral("pieces")).appendChild(copy.piece);
        }
    }
}
}

void TST_PatternPieceCopy::bundledAndUnionPieces_data()
{
    QTest::addColumn<QString>("source_path");
    const QDir source(QString::fromUtf8(SRCDIR));
    QDirIterator patterns(source.absoluteFilePath(QStringLiteral("../../app/share/samples/patterns")),
                          QStringList{QStringLiteral("*.sm2d")}, QDir::Files, QDirIterator::Subdirectories);
    while (patterns.hasNext())
    {
        const QString path = patterns.next();
        QTest::newRow(qPrintable(QFileInfo(path).fileName())) << path;
    }
    for (const QString &path : {QStringLiteral("../CollectionTest/share/steampunk_trousers/steampunk_trousers.sm2d")})
    {
        QTest::newRow(qPrintable(path)) << source.absoluteFilePath(path);
    }
}

void TST_PatternPieceCopy::bundledAndUnionPieces()
{
    QFETCH(QString, source_path);
    QTemporaryDir files;
    QVERIFY(files.isValid());
    const QString file_path = QDir(files.path()).filePath(QStringLiteral("copy.sm2d"));
    QFile input(source_path);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QDomDocument document;
    QVERIFY(document.setContent(&input));
    input.close();
    document.documentElement().setAttribute(QStringLiteral("readOnly"), true);
    QFile file(file_path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(document.toByteArray());
    file.close();
    QString converted_path;
    try
    {
        VPatternConverter converter(file_path);
        converted_path = converter.Convert();
        QFile converted(converted_path);
        QVERIFY(converted.open(QIODevice::ReadOnly));
        QVERIFY(document.setContent(&converted));
    }
    catch (VException &error)
    {
        QFAIL(qPrintable(error.ErrorMessage()));
    }

    quint32 next_id = 0;
    // Include Union's virtual child IDs, not only XML id attributes.
    QVector<QDomElement> pending{document.documentElement()};
    for (int i = 0; i < pending.size(); ++i)
    {
        const QDomElement element = pending.at(i);
        next_id = qMax(next_id, element.attribute(QStringLiteral("id")).toUInt());
        if (element.tagName() == QStringLiteral("child"))
        {
            next_id = qMax(next_id, element.text().toUInt());
        }
        for (QDomElement child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
        {
            pending.append(child);
        }
    }
    QVector<QDomElement> pieces;
    const QDomNodeList original_pieces = document.elementsByTagName(QStringLiteral("piece"));
    for (int i = 0; i < original_pieces.size(); ++i)
    {
        pieces.append(original_pieces.at(i).toElement());
    }
    QVERIFY(!pieces.isEmpty());
    for (const QDomElement &piece : pieces)
    {
        const quint32 source_id = piece.attribute(QStringLiteral("id")).toUInt();
        const PatternPieceCopyResult copy = PatternPieceCopy::reconstruct(document, source_id,
                                                                          [&next_id] { return ++next_id; });
        QVERIFY2(copy.error.isEmpty(), qPrintable(copy.error));
        QVERIFY(copy.ids.contains(source_id));
        QVERIFY(!copy.modeling.isEmpty());
        QCOMPARE(copy.piece.attribute(QStringLiteral("flip")), piece.attribute(QStringLiteral("flip")));
        QCOMPARE(copy.piece.attribute(QStringLiteral("united")), QStringLiteral("false"));
        for (quint32 id : copy.ids.keys())
        {
            QVERIFY(id != copy.ids.value(id));
        }
        appendCopy(document, copy);
        const PatternPieceCopyResult repeated = PatternPieceCopy::reconstruct(
            document, copy.ids.value(source_id), [&next_id] { return ++next_id; });
        QVERIFY2(repeated.error.isEmpty(), qPrintable(repeated.error));
        QCOMPARE(repeated.piece.attribute(QStringLiteral("name")),
                 copy.piece.attribute(QStringLiteral("name")) + QStringLiteral("-1"));
        appendCopy(document, repeated);
    }
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(document.toByteArray());
    file.close();
    VDomDocument::ValidateXML(VPatternConverter::CurrentSchema, file_path);

    // Optional standalone-parser verification uses the same reconstruction tested above.
    const QString output_directory = qEnvironmentVariable("SEAMLY_COPY_FIXTURES");
    if (!output_directory.isEmpty())
    {
        const QString output = QDir(output_directory).filePath(QFileInfo(source_path).baseName()
                                                               + QStringLiteral("-copied.sm2d"));
        QFile::remove(output);
        QVERIFY(QFile::copy(file_path, output));
    }
}

void TST_PatternPieceCopy::independentPathsAnchorsAndNestedUnion()
{
    QDomDocument document;
    QVERIFY(document.setContent(QStringLiteral(R"(
<pattern><draftBlock name="Block"><calculation><point id="1"/><point id="2"/></calculation><modeling>
<point id="3" idObject="1"/>
<point id="4" idObject="2"/>
<tools id="5"><piece><nodes><node idObject="3"/></nodes></piece>
<piece><nodes><node idObject="4"/></nodes></piece><children><nodes><child>6</child></nodes></children></tools>
<point id="7" idObject="6" idTool="5"/>
<tools id="8"><piece><nodes><node idObject="7"/></nodes></piece>
<piece><nodes><node idObject="4"/></nodes></piece><children><nodes><child>9</child></nodes></children></tools>
<point id="10" idObject="9" idTool="8"/>
<point id="11" idObject="1" idTool="13"/>
<path id="12" idTool="14"><nodes><node idObject="10"/></nodes></path>
</modeling><pieces><piece id="13" name="Collar" united="true" flip="2"><data letter="A" centerAnchor="11"/>
<nodes><node idObject="10"/></nodes><iPaths><record path="12"/></iPaths><anchors><record>11</record></anchors>
</piece><piece id="14" name="Union input"/></pieces></draftBlock></pattern>)")));
    const QByteArray before = document.toByteArray();
    quint32 id = 100;
    const auto copy = PatternPieceCopy::reconstruct(document, 13, [&id] { return ++id; });
    QVERIFY2(copy.error.isEmpty(), qPrintable(copy.error));
    QCOMPARE(document.toByteArray(), before);
    QCOMPARE(copy.ids.size(), 11);
    QCOMPARE(copy.modeling.size(), 8);
    QVERIFY(!copy.ids.contains(1));
    QVERIFY(!copy.ids.contains(2));
    QCOMPARE(copy.piece.attribute(QStringLiteral("name")), QStringLiteral("Collar-1"));
    QCOMPARE(copy.piece.firstChildElement(QStringLiteral("data")).attribute(QStringLiteral("letter")),
             QStringLiteral("A-2"));
    QCOMPARE(copy.piece.firstChildElement(QStringLiteral("data")).attribute(QStringLiteral("centerAnchor")).toUInt(),
             copy.ids.value(11));
    QCOMPARE(copy.modeling.constLast().element.attribute(QStringLiteral("idTool")).toUInt(), copy.ids.value(13));
    appendCopy(document, copy);
    const auto again = PatternPieceCopy::reconstruct(document, copy.ids.value(13), [&id] { return ++id; });
    QVERIFY2(again.error.isEmpty(), qPrintable(again.error));
    QCOMPARE(again.piece.attribute(QStringLiteral("name")), QStringLiteral("Collar-1-1"));
}

void TST_PatternPieceCopy::refusesMissingDependencies()
{
    QDomDocument document;
    QVERIFY(document.setContent(QStringLiteral(
        "<pattern><draftBlock name=\"Block\"><pieces><piece id=\"2\"><nodes>"
        "<node idObject=\"99\"/></nodes></piece></pieces></draftBlock></pattern>")));
    const QByteArray before = document.toByteArray();
    quint32 id = 100;
    const auto copy = PatternPieceCopy::reconstruct(document, 2, [&id] { return ++id; });
    QVERIFY(!copy.error.isEmpty());
    QVERIFY(copy.piece.isNull());
    QCOMPARE(id, 100U);
    QCOMPARE(document.toByteArray(), before);
}

void TST_PatternPieceCopy::undoRedoRestoresPrivateDependencies()
{
    CopyTestDocument document;
    QVERIFY(document.setContent(QStringLiteral(
        "<pattern><draftBlock name=\"Block\"><calculation><point id=\"1\"/></calculation>"
        "<modeling><point id=\"2\" idObject=\"1\"/></modeling><pieces><piece id=\"3\" name=\"Collar\">"
        "<nodes><node idObject=\"2\"/></nodes></piece></pieces></draftBlock></pattern>")));
    const QByteArray original = document.toByteArray();
    quint32 id = 100;
    const auto copy = PatternPieceCopy::reconstruct(document, 3, [&id] { return ++id; });
    QVERIFY2(copy.error.isEmpty(), qPrintable(copy.error));
    QUndoStack stack;
    auto *command = new CopyPatternPiece(copy, &document);
    connect(command, &VUndoCommand::NeedFullParsing, &document, &VAbstractPattern::NeedFullParsing);
    QSignalSpy parse_requests(&document, &VAbstractPattern::UndoCommand);
    stack.push(command);
    const QByteArray copied = document.toByteArray();
    QVERIFY(copied != original);
    QCOMPARE(stack.count(), 1);
    QCOMPARE(parse_requests.count(), 1);
    stack.undo();
    QCOMPARE(document.toByteArray(), original);
    stack.redo();
    QCOMPARE(document.toByteArray(), copied);
    QCOMPARE(parse_requests.count(), 3);
}
