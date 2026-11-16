// Author: slspencer
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2017  Seamly, LLC                                       *
 *                                                                         *
 *   https://github.com/fashionfreedom/seamly2d                             *
 *                                                                         *
 ***************************************************************************
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
 **************************************************************************

 ************************************************************************
 **
 **  @file   tst_vcommandline.cpp
 **  @author slspencer
 **  @date   2026
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentine project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
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
 *************************************************************************/

#include "tst_vcommandline.h"
#include "command_line.h"
#include "../vmisc/commandoptions.h"
#include "../vmisc/logging.h"

#include <QCommandLineParser>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTableView>
#include <QHeaderView>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>
#include <QtTest>

#ifdef Q_CC_MSVC
    #include <ciso646>
#endif /* Q_CC_MSVC */
//---------------------------------------------------------------------------------------------------------------------
TST_VCommandLine::TST_VCommandLine(QObject *parent)
    :QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
// cppcheck-suppress unusedFunction
void TST_VCommandLine::UniqueKeys()
{
    const QStringList options = AllKeys();
    QSet<QString> unique;

    foreach(const QString &str, options)
    {
        const QString message = QString("Options '%1' is not unique!").arg(str);
        QVERIFY2(not unique.contains(str), qUtf8Printable(message));
        unique.insert(str);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutExportOptions()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString output_path = directory.filePath(QStringLiteral("layout.svg"));
    const QString settings_path = directory.filePath(QStringLiteral("layout.json"));
    const QString export_options_path = directory.filePath(QStringLiteral("export.json"));
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("--layout-format"), QStringLiteral("SVG"),
        QStringLiteral("--layout-output"), output_path,
        QStringLiteral("--layout-settings"), settings_path,
        QStringLiteral("--layout-export-options"), export_options_path,
        QStringLiteral("--svg-text-mode"), QStringLiteral("HERSHEYSTROKES"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QVERIFY(command_line.isLayoutExportEnabled());
    QCOMPARE(command_line.layoutFormatOption(), QStringLiteral("svg"));
    QCOMPARE(command_line.layoutOutputOption(), QFileInfo(output_path).absoluteFilePath());
    QCOMPARE(command_line.layoutSettingsOption(), QFileInfo(settings_path).absoluteFilePath());
    QCOMPARE(command_line.layoutExportOptionsOption(), QFileInfo(export_options_path).absoluteFilePath());
    QCOMPARE(command_line.svgTextModeOption(), QStringLiteral("hersheyStrokes"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutExportOptionsAppearInHelp()
{
    VCommandLine command_line;
    const QString help = command_line.parser.helpText();

    QVERIFY(help.contains(QStringLiteral("--layout-export")));
    QVERIFY(help.contains(QStringLiteral("--layout-format")));
    QVERIFY(help.contains(QStringLiteral("--layout-output")));
    QVERIFY(help.contains(QStringLiteral("--svg-text-mode")));
    QVERIFY(help.contains(QStringLiteral("--mfile")));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::HelpDescribesLimitsAndRemovedBasename()
{
    VCommandLine command_line;
    const QString help = command_line.helpText();

    QVERIFY(help.startsWith(command_line.parser.helpText()));
    QVERIFY(help.contains(QStringLiteral("--test")));
    QVERIFY(help.contains(QStringLiteral("--layout-export requires --layout-output <path>")));
    QVERIFY(help.contains(QStringLiteral("This version does not accept --basename")));
    QVERIFY(help.contains(QStringLiteral("64: Incorrect command line arguments")));
    QVERIFY(!command_line.parser.parse({QStringLiteral("seamly2d"), QStringLiteral("--basename"),
                                       QStringLiteral("layout"), QStringLiteral("pattern.sm2d")}));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::HelpDialogClosesWithoutExiting()
{
    VCommandLine command_line;
    QWidget parent;
    QFont parent_font = parent.font();
    parent_font.setPointSize(14);
    parent.setFont(parent_font);
    QPalette parent_palette = parent.palette();
    parent_palette.setColor(QPalette::Base, QColor(QStringLiteral("#f2f4f8")));
    parent.setPalette(parent_palette);
    bool has_options = false;
    bool is_read_only = false;
    bool can_scroll = false;
    bool inherits_style = false;
    bool has_close_button = false;

    QTimer::singleShot(0, [&]()
    {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (dialog != nullptr)
        {
            const auto* table = dialog->findChild<QTableView*>(QStringLiteral("helpOptions"));
            if (table != nullptr)
            {
                has_options = table->model()->rowCount()
                    == command_line.m_options_used.size() + command_line.m_standard_help_options.size();
                is_read_only = table->editTriggers() == QAbstractItemView::NoEditTriggers
                    && !(table->model()->index(0, 0).flags() & Qt::ItemIsEditable);
                can_scroll = table->verticalScrollBar()->maximum() > 0;
                inherits_style = table->font() == parent.font() && table->style() == parent.style()
                    && table->palette().color(QPalette::Base) == parent_palette.color(QPalette::Base);
            }
            auto* buttons = dialog->findChild<QDialogButtonBox*>();
            if (buttons != nullptr && buttons->button(QDialogButtonBox::Close) != nullptr)
            {
                has_close_button = true;
                buttons->button(QDialogButtonBox::Close)->click();
            }
            else
            {
                dialog->reject();
            }
        }
    });

    command_line.showHelpDialog(&parent);
    QVERIFY(has_options);
    QVERIFY(is_read_only);
    QVERIFY(can_scroll);
    QVERIFY(inherits_style);
    QVERIFY(has_close_button);
    QVERIFY(command_line.parser.parse({QStringLiteral("seamly2d"), QStringLiteral("pattern.sm2d")}));
    QVERIFY(!command_line.isTestModeEnabled());
    QVERIFY(!command_line.isLayoutExportEnabled());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::HelpDialogFiltersAndResizesColumns()
{
    VCommandLine command_line;
    bool lists_registered_options = false;
    bool filters_options = false;
    bool handles_no_matches = false;
    bool resizes_columns = false;
    bool lists_exit_codes = false;
    bool explains_removed_basename = false;

    QTimer::singleShot(0, [&]()
    {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (dialog != nullptr)
        {
            auto* table = dialog->findChild<QTableView*>(QStringLiteral("helpOptions"));
            auto* filter = dialog->findChild<QLineEdit*>(QStringLiteral("helpFilter"));
            auto* tabs = dialog->findChild<QTabWidget*>(QStringLiteral("helpTabs"));
            if (table != nullptr && filter != nullptr && tabs != nullptr)
            {
                const auto* model = table->model();
                lists_registered_options = true;
                for (const QCommandLineOption* option : command_line.m_options_used)
                {
                    bool found = false;
                    for (int row = 0; row < model->rowCount(); ++row)
                    {
                        const QString name = model->index(row, 0).data().toString().mid(2);
                        found |= option->names().contains(name)
                            && model->index(row, 3).data().toString() == option->description();
                    }
                    lists_registered_options &= found;
                }
                resizes_columns = true;
                for (int column = 0; column < model->columnCount(); ++column)
                {
                    const int width = table->columnWidth(column) + 40;
                    table->setColumnWidth(column, width);
                    resizes_columns &= table->columnWidth(column) == width
                        && table->horizontalHeader()->sectionResizeMode(column) == QHeaderView::Interactive;
                }
                filter->setText(QStringLiteral("LAYOUT-UNIT"));
                table->sortByColumn(0, Qt::DescendingOrder);
                int visible_rows = 0;
                QString visible_option;
                for (int row = 0; row < model->rowCount(); ++row)
                {
                    if (!table->isRowHidden(row))
                    {
                        ++visible_rows;
                        visible_option = model->index(row, 0).data().toString();
                    }
                }
                filters_options = visible_rows == 1 && visible_option == QStringLiteral("--layout-unit");
                filter->setText(QStringLiteral("no-such-command-line-option"));
                handles_no_matches = true;
                for (int row = 0; row < model->rowCount(); ++row)
                {
                    handles_no_matches &= table->isRowHidden(row);
                }
                tabs->setCurrentIndex(2);
                const auto* errors = dialog->findChild<QTableView*>(QStringLiteral("helpErrors"));
                if (errors != nullptr && filter->text().isEmpty())
                {
                    for (int row = 0; row < errors->model()->rowCount(); ++row)
                    {
                        lists_exit_codes |= errors->model()->index(row, 0).data().toString() == QStringLiteral("64")
                            && !errors->isRowHidden(row);
                    }
                }
                tabs->setCurrentIndex(3);
                const auto* removed = dialog->findChild<QTableView*>(QStringLiteral("helpRemoved"));
                if (removed != nullptr)
                {
                    for (int row = 0; row < removed->model()->rowCount(); ++row)
                    {
                        explains_removed_basename |= removed->model()->index(row, 0).data().toString()
                            .contains(QStringLiteral("--basename"))
                            && removed->model()->index(row, 1).data().toString()
                                .contains(QStringLiteral("--layout-export"));
                    }
                }
            }
            dialog->reject();
        }
    });

    command_line.showHelpDialog();
    QVERIFY(lists_registered_options);
    QVERIFY(filters_options);
    QVERIFY(handles_no_matches);
    QVERIFY(resizes_columns);
    QVERIFY(lists_exit_codes);
    QVERIFY(explains_removed_basename);
}

//---------------------------------------------------------------------------------------------------------------------

void TST_VCommandLine::LayoutExportUsesDefaultSvgFormat()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("--layout-output"), directory.filePath(QStringLiteral("layout.svg")),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QVERIFY(command_line.isLayoutExportEnabled());
    QCOMPARE(command_line.layoutFormatOption(), QStringLiteral("svg"));
    QCOMPARE(command_line.svgTextModeOption(), QStringLiteral("designerFont"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutExportRequiresOutput()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QVERIFY(command_line.layoutOutputOption().isEmpty());
    QCOMPARE(command_line.layoutExportError(),
             QStringLiteral("--layout-export requires --layout-output <path>."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutExportRequiresOneInput()
{
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("--layout-output"), QStringLiteral("layout.svg")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QCOMPARE(command_line.layoutExportError(),
             QStringLiteral("--layout-export requires one input pattern file."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutExportRejectsUnsupportedFormat()
{
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("--layout-format"), QStringLiteral("unknown"),
        QStringLiteral("--layout-output"), QStringLiteral("layout.unknown"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QCOMPARE(command_line.layoutExportError(),
             QStringLiteral("Unsupported SeamlyLayout export format: unknown."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutExportRejectsSvgTextModeForOtherFormats()
{
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("--layout-format"), QStringLiteral("pdf"),
        QStringLiteral("--layout-output"), QStringLiteral("layout.pdf"),
        QStringLiteral("--svg-text-mode"), QStringLiteral("designerFont"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QCOMPARE(command_line.layoutExportError(),
             QStringLiteral("Invalid --svg-text-mode for the selected export format."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutScaleOptionIsRemoved()
{
    // PNG export renders at 96 px per inch; a scale option would be ignored, so it is not offered.
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("--layout-format"), QStringLiteral("png"),
        QStringLiteral("--layout-output"), QStringLiteral("layout.png"),
        QStringLiteral("--layout-scale"), QStringLiteral("2.0"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(!command_line.parser.parse(arguments));
    QVERIFY(command_line.parser.errorText().contains(QStringLiteral("layout-scale")));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutExportOptionsRequireExportMode()
{
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-output"), QStringLiteral("layout.svg"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QCOMPARE(command_line.layoutExportError(),
             QStringLiteral("SeamlyLayout export options require --layout-export."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutChoiceOptions()
{
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("--layout-format"), QStringLiteral("pdf"),
        QStringLiteral("--layout-output"), QStringLiteral("layout.pdf"),
        QStringLiteral("--layout-material"), QStringLiteral("Lining"),
        QStringLiteral("--layout-view"), QStringLiteral("nested"),
        QStringLiteral("--layout-oversize"), QStringLiteral("Extend"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QVERIFY(command_line.layoutExportError().isEmpty());
    QCOMPARE(command_line.layoutMaterialOption(), QStringLiteral("Lining"));
    QCOMPARE(command_line.layoutViewOption(), QStringLiteral("nested"));
    QCOMPARE(command_line.layoutOversizeOption(), QStringLiteral("extend"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutChoiceOptionsAppearInHelp()
{
    VCommandLine command_line;
    const QString help = command_line.parser.helpText();

    for (const QString &option : {QStringLiteral("--layout-material"), QStringLiteral("--layout-view"),
                                  QStringLiteral("--layout-oversize"), QStringLiteral("--layout-info"),
                                  QStringLiteral("--layout-settings"), QStringLiteral("--layout-export-options"),
                                  QStringLiteral("--gsize"),
                                  QStringLiteral("--gheight"), QStringLiteral("--test"),
                                  QStringLiteral("--no-scaling")})
    {
        QVERIFY2(help.contains(option), qUtf8Printable(option));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutOversizeRejectsUnknownChoice()
{
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("--layout-output"), QStringLiteral("layout.svg"),
        QStringLiteral("--layout-oversize"), QStringLiteral("shrink"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QCOMPARE(command_line.layoutExportError(), QStringLiteral("--layout-oversize must be split or extend."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutNameOptionsNeedAName()
{
    for (const QString &option : {QStringLiteral("layout-material"), QStringLiteral("layout-view")})
    {
        VCommandLine command_line;
        const QStringList arguments{
            QStringLiteral("seamly2d"),
            QStringLiteral("--layout-export"),
            QStringLiteral("--layout-output"), QStringLiteral("layout.svg"),
            QStringLiteral("--") + option, QStringLiteral(" "),
            QStringLiteral("pattern.sm2d")
        };

        QVERIFY(command_line.parser.parse(arguments));
        QCOMPARE(command_line.layoutExportError(), QStringLiteral("--%1 needs a name.").arg(option));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutViewRejectsGradationOptions()
{
    for (const QString &option : {QStringLiteral("--gsize"), QStringLiteral("--gheight")})
    {
        VCommandLine command_line;
        const QStringList arguments{
            QStringLiteral("seamly2d"),
            QStringLiteral("--layout-export"),
            QStringLiteral("--layout-output"), QStringLiteral("layout.svg"),
            QStringLiteral("--layout-view"), QStringLiteral("marker"),
            option, QStringLiteral("40"),
            QStringLiteral("pattern.sm2d")
        };

        QVERIFY(command_line.parser.parse(arguments));
        QVERIFY(command_line.layoutExportError().startsWith(QStringLiteral("--layout-view selects")));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutInfoMode()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString settings_path = directory.filePath(QStringLiteral("layout.json"));
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-info"),
        QStringLiteral("--layout-settings"), settings_path,
        QStringLiteral("--gsize"), QStringLiteral("40"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QVERIFY(command_line.layoutExportError().isEmpty());
    QVERIFY(command_line.isLayoutInfoEnabled());
    QVERIFY(!command_line.isLayoutExportEnabled());
    QCOMPARE(command_line.layoutSettingsOption(), QFileInfo(settings_path).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutInfoRejectsExportChoices()
{
    const QList<QStringList> choices{
        {QStringLiteral("--layout-format"), QStringLiteral("pdf")},
        {QStringLiteral("--layout-output"), QStringLiteral("layout.pdf")},
        {QStringLiteral("--layout-material"), QStringLiteral("Fabric")},
        {QStringLiteral("--layout-view"), QStringLiteral("marker")},
        {QStringLiteral("--layout-oversize"), QStringLiteral("split")}
    };
    for (const QStringList &choice : choices)
    {
        VCommandLine command_line;
        const QStringList arguments = QStringList{QStringLiteral("seamly2d"), QStringLiteral("--layout-info")}
                                      + choice + QStringList{QStringLiteral("pattern.sm2d")};

        QVERIFY(command_line.parser.parse(arguments));
        QVERIFY2(command_line.layoutExportError().startsWith(QStringLiteral("--layout-info prints a report")),
                 qUtf8Printable(choice.join(QLatin1Char(' '))));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutInfoAndExportConflict()
{
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("--layout-info"),
        QStringLiteral("--layout-output"), QStringLiteral("layout.svg"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QCOMPARE(command_line.layoutExportError(),
             QStringLiteral("--layout-export and --layout-info cannot be used together."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutInfoRequiresOneInput()
{
    VCommandLine command_line;
    const QStringList arguments{QStringLiteral("seamly2d"), QStringLiteral("--layout-info")};

    QVERIFY(command_line.parser.parse(arguments));
    QCOMPARE(command_line.layoutExportError(), QStringLiteral("--layout-info requires one input pattern file."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutSettingsRequireExportOrInfo()
{
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-settings"), QStringLiteral("layout.json"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QCOMPARE(command_line.layoutExportError(),
             QStringLiteral("SeamlyLayout export options require --layout-export."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::RemovedLegacyOptionsFailToParse()
{
    // The old console export options stay removed; the parser names each one.
    for (const QString &option : {QStringLiteral("--basename"),
                                  QStringLiteral("--exportOnlyDetails"), QStringLiteral("--format"),
                                  QStringLiteral("--pageformat"), QStringLiteral("--rotate")})
    {
        VCommandLine command_line;
        QVERIFY2(!command_line.parser.parse({QStringLiteral("seamly2d"), option, QStringLiteral("pattern.sm2d")}),
                 qUtf8Printable(option));
        QVERIFY(command_line.parser.errorText().contains(option.mid(2)));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::MeasurementOverrideIsAbsolute()
{
    // The pattern loader resolves a relative path against the pattern folder; the CLI gives it the launch folder.
    const QString relative_path = QStringLiteral("measurements/shirt.smis");
    for (const QString &mode : {QStringLiteral("--test"), QStringLiteral("--layout-info")})
    {
        VCommandLine command_line;
        QVERIFY(command_line.parser.parse({QStringLiteral("seamly2d"), mode, QStringLiteral("--mfile"), relative_path,
                                           QStringLiteral("patterns/shirt.sm2d")}));
        QCOMPARE(command_line.measurementPathOption(), QFileInfo(relative_path).absoluteFilePath());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::Text2PathsForVectorFormats()
{
    for (const QString &format : {QStringLiteral("svg"), QStringLiteral("pdf"), QStringLiteral("pdf-tiled"),
                                  QStringLiteral("ps"), QStringLiteral("eps")})
    {
        VCommandLine command_line;
        QVERIFY(command_line.parser.parse({QStringLiteral("seamly2d"), QStringLiteral("--layout-export"),
                                           QStringLiteral("--layout-format"), format,
                                           QStringLiteral("--layout-output"), QStringLiteral("layout.out"),
                                           QStringLiteral("--text2paths"), QStringLiteral("pattern.sm2d")}));
        QVERIFY2(command_line.layoutExportError().isEmpty(), qUtf8Printable(format));
        QVERIFY(command_line.isText2PathsSet());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::Text2PathsRejectedCombinations()
{
    const QList<QPair<QStringList, QString>> cases{
        {{QStringLiteral("--layout-format"), QStringLiteral("dxf")}, QStringLiteral("--text2paths applies to")},
        {{QStringLiteral("--layout-format"), QStringLiteral("png")}, QStringLiteral("--text2paths applies to")},
        {{QStringLiteral("--svg-text-mode"), QStringLiteral("hersheyStrokes")},
         QStringLiteral("--text2paths and --svg-text-mode cannot be used together.")}
    };
    for (const auto &item : cases)
    {
        VCommandLine command_line;
        const QStringList arguments = QStringList{QStringLiteral("seamly2d"), QStringLiteral("--layout-export"),
                                                  QStringLiteral("--layout-output"), QStringLiteral("layout.out"),
                                                  QStringLiteral("--text2paths")}
                                      + item.first + QStringList{QStringLiteral("pattern.sm2d")};
        QVERIFY(command_line.parser.parse(arguments));
        QVERIFY2(command_line.layoutExportError().startsWith(item.second), qUtf8Printable(command_line.layoutExportError()));
    }

    VCommandLine without_export;
    QVERIFY(without_export.parser.parse({QStringLiteral("seamly2d"), QStringLiteral("--text2paths"),
                                         QStringLiteral("pattern.sm2d")}));
    QCOMPARE(without_export.layoutExportError(), QStringLiteral("SeamlyLayout export options require --layout-export."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutFlagsForwardedInOrder()
{
    VCommandLine command_line;
    QVERIFY(command_line.parser.parse({QStringLiteral("seamly2d"), QStringLiteral("--layout-export"),
                                       QStringLiteral("--layout-output"), QStringLiteral("layout.svg"),
                                       QStringLiteral("--layout-nap"), QStringLiteral("down"),
                                       QStringLiteral("--layout-gap"), QStringLiteral("0.1"),
                                       QStringLiteral("--layout-ignore-margins"),
                                       QStringLiteral("--layout-roll-width"), QStringLiteral("24"),
                                       QStringLiteral("pattern.sm2d")}));
    QVERIFY(command_line.layoutExportError().isEmpty());
    // Table order (LayoutFlagNames), so a specific flag always follows the general one.
    QCOMPARE(command_line.layoutFlagArguments(),
             QStringList({QStringLiteral("--roll-width"), QStringLiteral("24"), QStringLiteral("--ignore-margins"),
                          QStringLiteral("--gap"), QStringLiteral("0.1"), QStringLiteral("--nap"), QStringLiteral("down")}));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutFlagsNeedExportOrInfo()
{
    VCommandLine alone;
    QVERIFY(alone.parser.parse({QStringLiteral("seamly2d"), QStringLiteral("--layout-gap"), QStringLiteral("1"),
                                QStringLiteral("pattern.sm2d")}));
    QCOMPARE(alone.layoutExportError(), QStringLiteral("SeamlyLayout export options require --layout-export."));

    VCommandLine info;
    QVERIFY(info.parser.parse({QStringLiteral("seamly2d"), QStringLiteral("--layout-info"),
                               QStringLiteral("--layout-roll-width"), QStringLiteral("12"), QStringLiteral("pattern.sm2d")}));
    QVERIFY(info.layoutExportError().isEmpty());
    QCOMPARE(info.layoutFlagArguments(), QStringList({QStringLiteral("--roll-width"), QStringLiteral("12")}));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutNoPackingCombinations()
{
    VCommandLine plain;
    QVERIFY(plain.parser.parse({QStringLiteral("seamly2d"), QStringLiteral("--layout-export"),
                                QStringLiteral("--layout-output"), QStringLiteral("layout.svg"),
                                QStringLiteral("--layout-no-packing"), QStringLiteral("pattern.sm2d")}));
    QVERIFY(plain.layoutExportError().isEmpty());
    QVERIFY(plain.isLayoutNoPackingSet());

    for (const QStringList &extra : {QStringList{QStringLiteral("--layout-view"), QStringLiteral("marker")},
                                     QStringList{QStringLiteral("--layout-oversize"), QStringLiteral("split")},
                                     QStringList{QStringLiteral("--layout-format"), QStringLiteral("pdf-tiled")}})
    {
        VCommandLine command_line;
        QVERIFY(command_line.parser.parse(QStringList{QStringLiteral("seamly2d"), QStringLiteral("--layout-export"),
                                                      QStringLiteral("--layout-output"), QStringLiteral("layout.svg"),
                                                      QStringLiteral("--layout-no-packing")}
                                          + extra + QStringList{QStringLiteral("pattern.sm2d")}));
        QVERIFY2(command_line.layoutExportError().startsWith(QStringLiteral("--layout-no-packing exports")),
                 qUtf8Printable(extra.join(QLatin1Char(' '))));
    }
}
