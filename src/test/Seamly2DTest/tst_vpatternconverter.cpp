//---------------------------------------------------------------------------------------------------------------------
//  @file   tst_vpatternconverter.cpp
//  @author slspencer
//  @brief  Tests pattern schema conversion for existing pattern documents.
//  @copyright
//  Copyright (C) 2026 Seamly2D Project
//  License: GPL-3.0-or-later
//  <https://www.gnu.org/licenses/gpl-3.0.html>
//---------------------------------------------------------------------------------------------------------------------

#include "tst_vpatternconverter.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QDomDocument>
#include <QDomElement>
#include <QTemporaryDir>
#include <QtTest>

#include "xml/vpatternconverter.h"
#include "xml/vdomdocument.h"
#include "xml/PatternDocumentWriter.h"
#include "exception/vexception.h"

namespace
{
QString samplePatternsDirectory()
{
    return QDir(QString::fromUtf8(SRCDIR)).absoluteFilePath(QStringLiteral("../../app/share/samples/patterns"));
}

bool writeDocument(const QString &file_path, const QDomDocument &document, QString &error);
bool readDocument(const QString &file_path, QDomDocument &document, QString &error);

bool convertAndCheck(const QString &input_path, QString &error)
{
    try
    {
        QDomDocument input_document;
        if (!readDocument(input_path, input_document, error))
        {
            return false;
        }
        input_document.documentElement().setAttribute(QStringLiteral("readOnly"), true);
        if (!writeDocument(input_path, input_document, error))
        {
            return false;
        }

        VPatternConverter converter(input_path);
        const QString converted_path = converter.Convert();
        VDomDocument::ValidateXML(VPatternConverter::CurrentSchema, converted_path);

        QFile converted_file(converted_path);
        if (!converted_file.open(QIODevice::ReadOnly))
        {
            error = converted_file.errorString();
            return false;
        }

        QDomDocument document;
        QString parse_error;
        if (!document.setContent(&converted_file, &parse_error))
        {
            error = parse_error;
            return false;
        }

        if (document.documentElement().firstChildElement(QStringLiteral("version")).text()
            != VPatternConverter::PatternMaxVerStr)
        {
            error = QStringLiteral("Converted pattern has an unexpected schema version.");
            return false;
        }

        const QDomNodeList blocks = document.elementsByTagName(QStringLiteral("draftBlock"));
        for (int i = 0; i < blocks.size(); ++i)
        {
            const QDomElement block = blocks.at(i).toElement();
            const QDomElement groups = block.firstChildElement(QStringLiteral("groups"));
            const QDomElement images = block.firstChildElement(QStringLiteral("images"));
            if (groups.isNull() || images.isNull() || images.previousSiblingElement() != groups)
            {
                error = QStringLiteral("Converted draft block is missing groups or images in schema order.");
                return false;
            }
        }
        return !blocks.isEmpty();
    }
    catch (VException &exception)
    {
        error = exception.ErrorMessage();
        return false;
    }
}

bool writeDocument(const QString &file_path, const QDomDocument &document, QString &error)
{
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        error = file.errorString();
        return false;
    }

    if (file.write(document.toByteArray(2)) < 0)
    {
        error = file.errorString();
        return false;
    }
    return true;
}

bool readDocument(const QString &file_path, QDomDocument &document, QString &error)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        error = file.errorString();
        return false;
    }
    return document.setContent(&file, &error);
}
}

void TST_VPatternConverter::samplePatternsConvertToCurrentSchema()
{
    QTemporaryDir test_files;
    QVERIFY(test_files.isValid());

    QDirIterator iterator(samplePatternsDirectory(), QStringList() << QStringLiteral("*.sm2d"), QDir::Files,
                          QDirIterator::Subdirectories);
    int converted_count = 0;
    QStringList converted_names;
    while (iterator.hasNext())
    {
        const QString source_path = iterator.next();
        const QString copy_path = QDir(test_files.path()).filePath(QFileInfo(source_path).fileName());
        QVERIFY2(QFile::copy(source_path, copy_path), qPrintable(source_path));

        QString error;
        QVERIFY2(convertAndCheck(copy_path, error), qPrintable(error));
        ++converted_count;
        converted_names.append(QFileInfo(source_path).fileName());
    }

    QVERIFY(converted_count > 0);
    QVERIFY(converted_names.contains(QStringLiteral("jacket1_52-176.sm2d")));
}

void TST_VPatternConverter::emptyDocumentUsesCurrentSchema()
{
    QTemporaryDir test_files;
    QVERIFY(test_files.isValid());
    QDomDocument document;
    PatternDocumentWriter::createEmptyPattern(document, Unit::Cm, QStringLiteral("Empty pattern"));
    const QString file_path = QDir(test_files.path()).filePath(QStringLiteral("empty.sm2d"));
    QString error;
    QVERIFY2(writeDocument(file_path, document, error), qPrintable(error));
    QCOMPARE(document.documentElement().firstChildElement(QStringLiteral("version")).text(),
             VPatternConverter::PatternMaxVerStr);
    VDomDocument::ValidateXML(VPatternConverter::CurrentSchema, file_path);
}

void TST_VPatternConverter::legacyMeasurementsLabelIncludesOneExtension()
{
    QTemporaryDir test_files;
    QVERIFY(test_files.isValid());
    QDomDocument document;
    QVERIFY(document.setContent(QStringLiteral(
        "<pattern readOnly=\"true\"><version>0.5.1</version><unit>cm</unit>"
        "<showMeasurements>true</showMeasurements><measurements>body.smis</measurements>"
        "<draw name=\"Front\"><calculation/><modeling/><details/></draw></pattern>")));
    const QString file_path = QDir(test_files.path()).filePath(QStringLiteral("legacy.sm2d"));
    QString error;
    QVERIFY2(writeDocument(file_path, document, error), qPrintable(error));
    VPatternConverter converter(file_path);
    const QString converted_path = converter.Convert();
    QVERIFY2(readDocument(converted_path, document, error), qPrintable(error));
    const QDomElement label = document.documentElement().firstChildElement(QStringLiteral("patternLabel"));
    QCOMPARE(label.elementsByTagName(QStringLiteral("line")).count(), 1);
    QCOMPARE(label.firstChildElement(QStringLiteral("line")).attribute(QStringLiteral("text")),
             QStringLiteral("%mFileName%"));
}

void TST_VPatternConverter::newDraftBlockCreatesImages()
{
    QTemporaryDir test_files;
    QVERIFY(test_files.isValid());
    QDomDocument document;
    PatternDocumentWriter::createEmptyPattern(document, Unit::Cm, QStringLiteral("New pattern"));
    QDomElement block = document.createElement(QStringLiteral("draftBlock"));
    block.setAttribute(QStringLiteral("name"), QStringLiteral("New block"));
    block.appendChild(document.createElement(QStringLiteral("calculation")));
    block.appendChild(document.createElement(QStringLiteral("modeling")));
    block.appendChild(document.createElement(QStringLiteral("pieces")));
    document.documentElement().appendChild(block);
    const QDomElement images = PatternDocumentWriter::createDraftImages(document, block);
    QVERIFY(!images.isNull());
    QCOMPARE(images.previousSiblingElement().tagName(), QStringLiteral("groups"));
    QCOMPARE(PatternDocumentWriter::createDraftImages(document, block), images);
    QCOMPARE(block.elementsByTagName(QStringLiteral("images")).count(), 1);
    block.appendChild(images.previousSiblingElement());
    QCOMPARE(PatternDocumentWriter::createDraftImages(document, block).previousSiblingElement().tagName(),
             QStringLiteral("groups"));

    const QString file_path = QDir(test_files.path()).filePath(QStringLiteral("new_block.sm2d"));
    QString error;
    QVERIFY2(writeDocument(file_path, document, error), qPrintable(error));
    VDomDocument::ValidateXML(VPatternConverter::CurrentSchema, file_path);
}

void TST_VPatternConverter::previousSchemaGetsRequiredImages()
{
    QTemporaryDir test_files;
    QVERIFY(test_files.isValid());
    const QString file_path = QDir(test_files.path()).filePath(QStringLiteral("previous_schema.sm2d"));
    const QString source_path = QDir(samplePatternsDirectory()).filePath(QStringLiteral("male_shirt.sm2d"));
    QVERIFY(QFile::copy(source_path, file_path));
    QDomDocument document;
    QString error;
    QVERIFY2(readDocument(file_path, document, error), qPrintable(error));
    document.documentElement().setAttribute(QStringLiteral("readOnly"), true);
    QVERIFY2(writeDocument(file_path, document, error), qPrintable(error));
    VPatternConverter initial_converter(file_path);
    const QString current_path = initial_converter.Convert();
    QVERIFY2(readDocument(current_path, document, error), qPrintable(error));
    document.documentElement().firstChildElement(QStringLiteral("version")).firstChild()
            .setNodeValue(QStringLiteral("0.7.7"));
    const QDomNodeList blocks = document.elementsByTagName(QStringLiteral("draftBlock"));
    for (int i = 0; i < blocks.size(); ++i)
    {
        QDomElement block = blocks.at(i).toElement();
        block.removeChild(block.firstChildElement(QStringLiteral("images")));
    }
    QVERIFY2(writeDocument(file_path, document, error), qPrintable(error));
    VDomDocument::ValidateXML(QStringLiteral("://schema/pattern/v0.7.7.xsd"), file_path);
    QVERIFY2(convertAndCheck(file_path, error), qPrintable(error));
}

void TST_VPatternConverter::currentSchemaRequiresImages()
{
    QTemporaryDir test_files;
    QVERIFY(test_files.isValid());
    QDomDocument document;
    PatternDocumentWriter::createEmptyPattern(document, Unit::Cm, QStringLiteral("New pattern"));
    QDomElement block = document.createElement(QStringLiteral("draftBlock"));
    block.setAttribute(QStringLiteral("name"), QStringLiteral("New block"));
    block.appendChild(document.createElement(QStringLiteral("calculation")));
    block.appendChild(document.createElement(QStringLiteral("modeling")));
    block.appendChild(document.createElement(QStringLiteral("pieces")));
    block.appendChild(document.createElement(QStringLiteral("groups")));
    document.documentElement().appendChild(block);
    const QString file_path = QDir(test_files.path()).filePath(QStringLiteral("missing_current_images.sm2d"));
    QString error;
    QVERIFY2(writeDocument(file_path, document, error), qPrintable(error));
    QVERIFY_EXCEPTION_THROWN(VDomDocument::ValidateXML(VPatternConverter::CurrentSchema, file_path), VException);
}

void TST_VPatternConverter::repairsGroupsAfterImages()
{
    QTemporaryDir test_files;
    QVERIFY(test_files.isValid());

    const QString sample_path = QDir(samplePatternsDirectory()).filePath(QStringLiteral("male_shirt.sm2d"));
    const QString input_path = QDir(test_files.path()).filePath(QStringLiteral("groups_after_images.sm2d"));
    QVERIFY(QFile::copy(sample_path, input_path));

    QDomDocument document;
    QString error;
    QVERIFY2(readDocument(input_path, document, error), qPrintable(error));
    QDomElement block = document.elementsByTagName(QStringLiteral("draftBlock")).at(0).toElement();
    const QDomElement groups = block.firstChildElement(QStringLiteral("groups"));
    const QDomElement images = block.firstChildElement(QStringLiteral("images"));
    QVERIFY(!groups.isNull());
    QVERIFY(!images.isNull());
    block.insertBefore(images, groups);
    QVERIFY2(writeDocument(input_path, document, error), qPrintable(error));

    QVERIFY2(convertAndCheck(input_path, error), qPrintable(error));
}

void TST_VPatternConverter::addsMissingGroupsAndImages()
{
    QTemporaryDir test_files;
    QVERIFY(test_files.isValid());

    const QString sample_path = QDir(samplePatternsDirectory()).filePath(QStringLiteral("male_shirt.sm2d"));
    const QString input_path = QDir(test_files.path()).filePath(QStringLiteral("missing_groups_images.sm2d"));
    QVERIFY(QFile::copy(sample_path, input_path));

    QDomDocument document;
    QString error;
    QVERIFY2(readDocument(input_path, document, error), qPrintable(error));
    const QDomNodeList blocks = document.elementsByTagName(QStringLiteral("draftBlock"));
    for (int i = 0; i < blocks.size(); ++i)
    {
        QDomElement block = blocks.at(i).toElement();
        const QDomElement groups = block.firstChildElement(QStringLiteral("groups"));
        const QDomElement images = block.firstChildElement(QStringLiteral("images"));
        if (!groups.isNull())
        {
            block.removeChild(groups);
        }
        if (!images.isNull())
        {
            block.removeChild(images);
        }
    }
    QVERIFY2(writeDocument(input_path, document, error), qPrintable(error));

    QVERIFY2(convertAndCheck(input_path, error), qPrintable(error));
}
