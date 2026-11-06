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
#include "vcmdexport.h"
#include "../vmisc/commandoptions.h"
#include "../vmisc/logging.h"

#include <QCommandLineParser>
#include <QFileInfo>
#include <QTemporaryDir>
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
    QVERIFY(command_line.IsLayoutExportEnabled());
    QCOMPARE(command_line.OptLayoutFormat(), QStringLiteral("svg"));
    QCOMPARE(command_line.OptLayoutOutput(), QFileInfo(output_path).absoluteFilePath());
    QCOMPARE(command_line.OptLayoutSettings(), QFileInfo(settings_path).absoluteFilePath());
    QCOMPARE(command_line.OptLayoutExportOptions(), QFileInfo(export_options_path).absoluteFilePath());
    QCOMPARE(command_line.OptSvgTextMode(), QStringLiteral("hersheyStrokes"));
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
    QVERIFY(command_line.IsLayoutExportEnabled());
    QCOMPARE(command_line.OptLayoutFormat(), QStringLiteral("svg"));
    QCOMPARE(command_line.OptSvgTextMode(), QStringLiteral("designerFont"));
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
    QVERIFY(command_line.OptLayoutOutput().isEmpty());
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
void TST_VCommandLine::LayoutExportScale()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    VCommandLine command_line;
    const QStringList arguments{
        QStringLiteral("seamly2d"),
        QStringLiteral("--layout-export"),
        QStringLiteral("--layout-format"), QStringLiteral("png"),
        QStringLiteral("--layout-output"), directory.filePath(QStringLiteral("layout.png")),
        QStringLiteral("--layout-scale"), QStringLiteral("2.0"),
        QStringLiteral("pattern.sm2d")
    };

    QVERIFY(command_line.parser.parse(arguments));
    QVERIFY(command_line.IsLayoutExportEnabled());
    QCOMPARE(command_line.OptLayoutScale(), QStringLiteral("2.0"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VCommandLine::LayoutExportRejectsInvalidScale()
{
    for (const QString &scale : {QStringLiteral("0"), QStringLiteral("1e100")}) {
        VCommandLine command_line;
        const QStringList arguments{
            QStringLiteral("seamly2d"),
            QStringLiteral("--layout-export"),
            QStringLiteral("--layout-format"), QStringLiteral("png"),
            QStringLiteral("--layout-output"), QStringLiteral("layout.png"),
            QStringLiteral("--layout-scale"), scale,
            QStringLiteral("pattern.sm2d")
        };

        QVERIFY(command_line.parser.parse(arguments));
        QCOMPARE(command_line.layoutExportError(),
                 QStringLiteral("--layout-scale requires a positive PNG export scale."));
    }
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
