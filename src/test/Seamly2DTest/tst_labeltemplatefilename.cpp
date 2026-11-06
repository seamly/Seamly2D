//---------------------------------------------------------------------------------------------------------------------
// @file tst_labeltemplatefilename.cpp
// @author slspencer
// @brief Tests label template basenames and full-path tooltips.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "tst_labeltemplatefilename.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QTemporaryDir>
#include <QtTest>

#include "tools/new_piece_defaults.h"

void TST_LabelTemplateFileName::selectedFile()
{
    QTemporaryDir files;
    QVERIFY(files.isValid());
    const QString selected_file = QDir(files.path()).filePath(QStringLiteral("selected.label"));
    QFile file(selected_file);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    QLabel label;
    NewPieceDefaults::showLabelTemplateFile(&label, NewPieceDefaults::labelTemplateFile(selected_file, QString()));
    QCOMPARE(label.text(), QStringLiteral("selected.label"));
    QCOMPARE(label.toolTip(), QDir::toNativeSeparators(selected_file));
}

void TST_LabelTemplateFileName::defaultFile()
{
    QTemporaryDir files;
    QVERIFY(files.isValid());
    const QString default_file = QDir(files.path()).filePath(QStringLiteral("default.label"));
    QFile file(default_file);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    QLabel label;
    const QString missing_file = QDir(files.path()).filePath(QStringLiteral("missing.label"));
    NewPieceDefaults::showLabelTemplateFile(&label, NewPieceDefaults::labelTemplateFile(missing_file, default_file));
    QCOMPARE(label.text(), QStringLiteral("default.label"));
    QCOMPARE(label.toolTip(), QDir::toNativeSeparators(default_file));
}

void TST_LabelTemplateFileName::noFile()
{
    QLabel label;
    NewPieceDefaults::showLabelTemplateFile(&label, QStringLiteral("selected.label"));
    NewPieceDefaults::showLabelTemplateFile(&label, QString());
    QCOMPARE(label.text(), QStringLiteral("none"));
    QVERIFY(label.toolTip().isEmpty());
}

void TST_LabelTemplateFileName::builtInFile()
{
    QLabel label;
    const QString resource_file = QStringLiteral(":/label_templates/piece.label");
    NewPieceDefaults::showLabelTemplateFile(&label, resource_file);
    QCOMPARE(label.text(), QStringLiteral("piece.label"));
    QCOMPARE(label.toolTip(), QDir::toNativeSeparators(resource_file));
}
