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
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

#include "../../libs/ifc/xml/vabstractpattern.h"
#include "../../libs/vtools/dialogs/support/editlabeltemplate_dialog.h"
#include "tools/new_piece_defaults.h"

namespace
{
class LabelTemplateTestDocument : public VAbstractPattern
{
public:
    void CreateEmptyFile() override {}
    void IncrementReferens(quint32) const override {}
    void DecrementReferens(quint32) const override {}
    QStringList GetCurrentAlphabet() const override { return {}; }
    QString GenerateLabel(const LabelType &, const QString &) const override { return {}; }
    QString generateSuffix(const QString &) const override { return {}; }
    void UpdateToolData(const quint32 &, VContainer *) override {}
    void LiteParseTree(const Document &) override {}
};
}

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

void TST_LabelTemplateFileName::insertedPlaceholderReplacesEmptyLine()
{
    LabelTemplateTestDocument document;
    EditLabelTemplateDialog dialog(&document);
    QVERIFY(QMetaObject::invokeMethod(&dialog, "AddLine"));

    auto *line_edit = dialog.findChild<QLineEdit *>(QStringLiteral("lineEditLine"));
    auto *lines = dialog.findChild<QListWidget *>(QStringLiteral("listWidgetEdit"));
    auto *insert_button = dialog.findChild<QPushButton *>(QStringLiteral("pushButtonInsert"));
    QVERIFY(line_edit != nullptr);
    QVERIFY(lines != nullptr);
    QVERIFY(insert_button != nullptr);
    QVERIFY(insert_button->menu() != nullptr);
    QVERIFY(lines->currentItem() != nullptr);
    QCOMPARE(line_edit->text(), QStringLiteral("<empty>"));

    const QList<QAction *> actions = insert_button->menu()->actions();
    QVERIFY(!actions.isEmpty());
    const QString placeholder = actions.first()->data().toString();
    QVERIFY(!placeholder.isEmpty());
    actions.first()->trigger();

    QCOMPARE(line_edit->text(), placeholder);
    QCOMPARE(lines->currentItem()->text(), placeholder);
    QVERIFY(dialog.templateFile().isEmpty());
}
