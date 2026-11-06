/**********************************************************************
 **
 **  @file   vcmdexport.cpp
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

#include "vcmdexport.h"
#include "../vformat/measurements.h"
#include "../vmisc/commandoptions.h"
#include "../vmisc/def.h"

#include <QDebug>
#include <QFileInfo>

#include <cmath>
#include <limits>
#include <cstdio>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace
{
void attachParentConsoleForCommandLine(const QStringList &arguments)
{
#ifdef Q_OS_WIN
    const HANDLE standard_output = GetStdHandle(STD_OUTPUT_HANDLE);
    const HANDLE standard_error = GetStdHandle(STD_ERROR_HANDLE);
    if ((standard_output != nullptr && standard_output != INVALID_HANDLE_VALUE) ||
        (standard_error != nullptr && standard_error != INVALID_HANDLE_VALUE))
    {
        return;
    }
    for (const QString &argument : arguments)
    {
        if (argument.startsWith(QStringLiteral("--layout-"))
            || argument == QStringLiteral("--svg-text-mode")
            || argument == QStringLiteral("--test")
            || argument == QStringLiteral("--mfile")
            || argument == QStringLiteral("--gsize")
            || argument == QStringLiteral("--gheight")
            || argument == QStringLiteral("-t")
            || argument == QStringLiteral("-m")
            || argument == QStringLiteral("-x")
            || argument == QStringLiteral("-e")
            || argument == QStringLiteral("-h")
            || argument == QStringLiteral("--help")
            || argument == QStringLiteral("-v")
            || argument == QStringLiteral("--version"))
        {
            if (AttachConsole(ATTACH_PARENT_PROCESS) || GetConsoleWindow() != nullptr)
            {
                FILE *console_stream = nullptr;
                freopen_s(&console_stream, "CONOUT$", "w", stdout);
                freopen_s(&console_stream, "CONOUT$", "w", stderr);
            }
            return;
        }
    }
#else
    Q_UNUSED(arguments);
#endif
}
} // namespace

VCommandLinePtr VCommandLine::commandLine = nullptr;

#define translate(context, source) QCoreApplication::translate((context), (source))

//  --------------------
/*
 * @brief Constructor for the VCommandLine class.
 *
 * Initializes the command-line parser with application description, standard help/version
 * options, a positional argument for the pattern file, and all custom options defined
 * in the application.
 */
VCommandLine::VCommandLine() : parser(), optionsUsed(), optionsIndex(), isGuiEnabled(false)
{
    // Sets a brief description of the application shown in the help output.
    parser.setApplicationDescription(translate("VCommandLine", "Seamly2D open source patternmaking program."));

    // Adds the standard help option (-h, --help) to the parser.
    parser.addHelpOption();

    // Adds the standard version option (-v, --version) to the parser.
    parser.addVersionOption();

    // Add a positional argument (not prefixed with a flag), typically the filename
    parser.addPositionalArgument("filename", translate("VCommandLine", "Seamly2D pattern file (.sm2d)"));

    // Initialize the list of custom command-line options and their index
    initOptions(optionsUsed, optionsIndex);

    // Iterate over each command-line option in optionsUsed and add it to the parser
    VCommandLineOptions::const_iterator i = optionsUsed.constBegin();
    while (i != optionsUsed.constEnd())
    {
        parser.addOption(*(*i));  // Dereference the pointer to QCommandLineOption and add it
        ++i;
    }
}

//  --------------------
/*
 * @brief Initializes the list of command-line options and their index.
 *
 * This method defines the supported test-mode, display, and layout-export options.
 * Options are added in a grouped, user-friendly order for consistency in help output.
 *
 * @param options A reference to the list where all VCommandLineOptions will be stored.
 * @param optionsIndex A map that associates each long-form option string with its index
 *in the options list.
 */
void VCommandLine::initOptions(VCommandLineOptions &options, QMap<QString, int> &optionsIndex)
{
    int index = 0;

    // Options are grouped for usability; order matters for help display
    // ================================================================================================================

    // --mfile: Path to custom measurement file
    optionsIndex.insert(LONG_OPTION_MEASUREFILE, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_MEASUREFILE, LONG_OPTION_MEASUREFILE},
        translate("VCommandLine", "Path to a measurement file for test mode or CLI export."),
        translate("VCommandLine", "The measure file")));

    // --gsize: Size value for multi-size patterns
    optionsIndex.insert(LONG_OPTION_GRADATIONSIZE, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_GRADATIONSIZE, LONG_OPTION_GRADATIONSIZE},
        translate("VCommandLine", "Set the size for a multisize pattern in test mode or CLI export. "
                                  "Valid values: %1cm.")
                                 .arg(MeasurementVariable::WholeListSizes(Unit::Cm).join(", ")),
        translate("VCommandLine", "The size value")));

    // --gheight: Height value for multi-size patterns
    optionsIndex.insert(LONG_OPTION_GRADATIONHEIGHT, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_GRADATIONHEIGHT, LONG_OPTION_GRADATIONHEIGHT},
        translate("VCommandLine", "Set the height for a multisize pattern in test mode or CLI export. "
                                  "Valid values: %1cm.")
                                  .arg(MeasurementVariable::WholeListHeights(Unit::Cm).join(", ")),
        translate("VCommandLine", "The height value")));

    // --test: Run program in test mode with single pattern file without showing mainwindow
    optionsIndex.insert(LONG_OPTION_TEST, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_TEST, LONG_OPTION_TEST},
        translate("VCommandLine", "Run the program in a test mode. The program in "
                                  "this mode loads a single pattern file and silently quit without "
                                  "showing the main window.")));

    // --layout-export: Generate a layout through SeamlyLayout without either UI.
    optionsIndex.insert(LONG_OPTION_LAYOUT_EXPORT, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_EXPORT},
        translate("VCommandLine", "Generate a layout through SeamlyLayout without showing either application.")));

    // --layout-format: SeamlyLayout output format.
    optionsIndex.insert(LONG_OPTION_LAYOUT_FORMAT, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_FORMAT},
        translate("VCommandLine", "SeamlyLayout output format: svg, pdf, pdf-tiled, png, jpg, jpeg, dxf, hpgl, ps, eps, gcode, or 3mf."),
        translate("VCommandLine", "format")));

    // --layout-output: Destination file for SeamlyLayout export.
    optionsIndex.insert(LONG_OPTION_LAYOUT_OUTPUT, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_OUTPUT},
        translate("VCommandLine", "Output file path for --layout-export."),
        translate("VCommandLine", "path")));

    // --layout-settings: Layout settings JSON file passed to SeamlyLayout.
    optionsIndex.insert(LONG_OPTION_LAYOUT_SETTINGS, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_SETTINGS},
        translate("VCommandLine", "Layout settings JSON file for SeamlyLayout."),
        translate("VCommandLine", "path")));

    // --layout-export-options: Format-specific JSON options passed to SeamlyLayout.
    optionsIndex.insert(LONG_OPTION_LAYOUT_EXPORT_OPTIONS, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_EXPORT_OPTIONS},
        translate("VCommandLine", "Format-specific export options JSON file for SeamlyLayout."),
        translate("VCommandLine", "path")));

    // --svg-text-mode: SVG label rendering mode.
    optionsIndex.insert(LONG_OPTION_SVG_TEXT_MODE, index++);
    options.append(new QCommandLineOption({LONG_OPTION_SVG_TEXT_MODE},
        translate("VCommandLine", "SVG text mode: designerFont, singleLineFont, hersheyStrokes, or asSupplied."),
        translate("VCommandLine", "mode")));

    // --layout-scale: PNG scale factor passed to SeamlyLayout.
    optionsIndex.insert(LONG_OPTION_LAYOUT_SCALE, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_SCALE},
        translate("VCommandLine", "PNG export scale factor (default 1.0)."),
        translate("VCommandLine", "factor")));

    // --no-scaling: Disable high DPI auto-scaling (useful on some systems)
    optionsIndex.insert(LONG_OPTION_NO_HDPI_SCALING, index++);
    options.append(new QCommandLineOption({LONG_OPTION_NO_HDPI_SCALING},
        translate("VCommandLine", "Disable high dpi scaling. Call this option if has problem "
                                  "with scaling (by default scaling enabled). Alternatively you "
                                  "can use the %1 environment variable.")
                                  .arg("QT_AUTO_SCREEN_SCALE_FACTOR=0")));
}


//  --------------------
/*
 * @brief Retrieves the singleton instance of VCommandLine and processes CLI arguments.
 *
 * If no VCommandLine exists, a new one is created. It then processes the arguments provided
 * to the application and determines whether the GUI should be enabled based on the CLI mode.
 *
 * @param app The current application instance containing CLI arguments.
 * @return VCommandLinePtr A shared pointer to the global VCommandLine instance.
 */
VCommandLinePtr VCommandLine::Get(const QCoreApplication &app)
{
    if (commandLine == nullptr)
    {
        commandLine.reset(new VCommandLine()); // Replace pointer with new VCommandLine
    }
    attachParentConsoleForCommandLine(app.arguments());
    commandLine->parser.process(app);          // Process CLI arguments with the parser

    // FIXME: Add more conditions here if new CLI modes are introduced that should disable the GUI
    const bool testMode = commandLine->IsTestModeEnabled();
    const bool layoutExportMode = commandLine->IsLayoutExportEnabled();
    if (testMode && layoutExportMode)
    {
        qCritical() << translate("VCommandLine", "--test and --layout-export cannot be used together.");
        commandLine->parser.showHelp(V_EX_USAGE);
    }
    commandLine->isGuiEnabled = !testMode && !layoutExportMode;

    return commandLine;
}

//  --------------------
/*
 * @brief Destructor for VCommandLine.
 *
 * Cleans up dynamically allocated list of VCommandLineOptions stored in optionsUsed,
 * ensuring no memory leaks. Clears the list after deleting.
 */
VCommandLine::~VCommandLine()
{
    qDeleteAll(optionsUsed.begin(), optionsUsed.end()); // Delete all dynamically allocated options
    optionsUsed.clear();                                // Clear the list to reset state
}

//  --------------------
/*
 * @brief Resets the singleton instance of VCommandLine.
 *
 * This destroys the current VCommandLine, allowing a fresh one to be created next time Get() is called.
 * Useful for tests or reinitializing command-line parsing.
 */
void VCommandLine::Reset()
{
    commandLine.reset(); // Calls the VCommandLine destructor.
}

//  --------------------
/*
 * @brief Checks whether the application is running in test mode.
 *
 * Determines if the `--test` command-line option is set. If it is, the function
 * verifies that exactly one positional argument (input file) is provided. If not,
 * it prints an error and terminates execution.
 *
 * @return true if test mode is enabled; otherwise false.
 */
bool VCommandLine::IsTestModeEnabled() const
{
    const bool r = parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_TEST)));

    // Ensure exactly one positional argument is provided when in test mode
    if (r && parser.positionalArguments().size() != 1)
    {
        qCritical() << translate("VCommandLine", "Test option can be used with single input file only.") << "/n";
        const_cast<VCommandLine*>(this)->parser.showHelp(V_EX_USAGE);
    }

    return r;
}

//  --------------------
/*
 * @brief Checks whether high DPI scaling is disabled.
 *
 * Determines if the --no-hdpi-scaling option is set via the command-line.
 *
 * @return true if scaling is disabled; otherwise false.
 */
bool VCommandLine::IsNoScalingEnabled() const
{
    return parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_NO_HDPI_SCALING)));
}

//  --------------------
/*
 * @brief Retrieves the path to the measurement file if applicable.
 *
 * Returns a measurement override only for test or layout-export mode.
 *
 * @return QString containing the measurement file path, or empty if not applicable.
 */
QString VCommandLine::OptMeasurePath() const
{
    QString measure;
    if (parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_MEASUREFILE)))
        && (IsTestModeEnabled() || IsLayoutExportEnabled()))
    {
        measure = parser.value(*optionsUsed.value(optionsIndex.value(LONG_OPTION_MEASUREFILE)));
    }

    return measure;
}

//  --------------------
/*
 * @brief Retrieves the list of input filenames passed as positional arguments.
 *
 * @return QStringList of input file names.
 */
QStringList VCommandLine::OptInputFileNames() const
{
    return parser.positionalArguments();
}

//  --------------------
/*
 * @brief Checks whether the GUI should be enabled based on current CLI mode.
 *
 * @return true if GUI mode is enabled; otherwise false.
 */
bool VCommandLine::IsGuiEnabled() const
{
    return isGuiEnabled;
}

//  --------------------
/*
 * @brief Checks if a specific gradation size is set via the CLI.
 *
 * @return true if --gradationsize option is set; otherwise false.
 */
bool VCommandLine::IsSetGradationSize() const
{
    return parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_GRADATIONSIZE)));
}

//  --------------------
/*
 * @brief Checks if a specific gradation height is set via the CLI.
 *
 * @return true if --gradationheight option is set; otherwise false.
 */
bool VCommandLine::IsSetGradationHeight() const
{
    return parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_GRADATIONHEIGHT)));
}

//  --------------------
/*
 * @brief Retrieves the gradation size value from the command-line.
 *
 * This function fetches the value of the --gradationsize option and validates it
 * using MeasurementVariable::IsGradationSizeValid(). If invalid, it prints an error
 * and exits the application via showHelp().
 *
 * @return QString containing a valid gradation size.
 */
QString VCommandLine::OptGradationSize() const
{
    const QString size = parser.value(*optionsUsed.value(optionsIndex.value(LONG_OPTION_GRADATIONSIZE)));
    if (MeasurementVariable::IsGradationSizeValid(size))
    {
        return size;
    }
    else
    {
        qCritical() << translate("VCommandLine", "Invalid gradation size value.") << "\n";
        const_cast<VCommandLine*>(this)->parser.showHelp(V_EX_USAGE);
    }
}

//  --------------------
/*
 * @brief Retrieves the gradation height value from the command-line.
 *
 * This function fetches the value of the --gradationheight option and validates it
 * using MeasurementVariable::IsGradationHeightValid(). If invalid, it prints an error
 * and exits the application via showHelp().
 *
 * @return QString containing a valid gradation height.
 */
QString VCommandLine::OptGradationHeight() const
{
    const QString height = parser.value(*optionsUsed.value(optionsIndex.value(LONG_OPTION_GRADATIONHEIGHT)));
    if (MeasurementVariable::IsGradationHeightValid(height))
    {
        return height;
    }
    else
    {
        qCritical() << translate("VCommandLine", "Invalid gradation height value.") << "\n";
        const_cast<VCommandLine*>(this)->parser.showHelp(V_EX_USAGE);
    }
}

// --------------------
/*
 * @brief Checks whether the CLI export options form a valid headless invocation.
 *
 * @return true when --layout-export is set and its companions are valid.
 */
bool VCommandLine::IsLayoutExportEnabled() const
{
    const QString error = layoutExportError();
    if (!error.isEmpty())
    {
        qCritical() << error;
        const_cast<VCommandLine*>(this)->parser.showHelp(V_EX_USAGE);
    }

    return parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_EXPORT)));
}

// --------------------
/*
 * @brief Returns a validation error for an incomplete or invalid layout export request.
 *
 * @return Empty when export options are valid or no export was requested.
 */
QString VCommandLine::layoutExportError() const
{
    const bool export_requested =
        parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_EXPORT)));
    const bool has_export_option =
        parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_FORMAT))) ||
        parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_OUTPUT))) ||
        parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_SETTINGS))) ||
        parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_EXPORT_OPTIONS))) ||
        parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_SVG_TEXT_MODE))) ||
        parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_SCALE)));

    if (!export_requested && has_export_option)
    {
        return translate("VCommandLine", "SeamlyLayout export options require --layout-export.");
    }
    if (!export_requested)
    {
        return QString();
    }

    if (parser.positionalArguments().size() != 1)
    {
        return translate("VCommandLine", "--layout-export requires one input pattern file.");
    }

    if (parser.value(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_OUTPUT))).trimmed().isEmpty())
    {
        return translate("VCommandLine", "--layout-export requires --layout-output <path>.");
    }

    const QString format = OptLayoutFormat();
    const QStringList supported_formats{
        QStringLiteral("svg"), QStringLiteral("pdf"), QStringLiteral("pdf-tiled"),
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("dxf"), QStringLiteral("hpgl"), QStringLiteral("ps"),
        QStringLiteral("eps"), QStringLiteral("gcode"), QStringLiteral("3mf")
    };
    if (!supported_formats.contains(format))
    {
        return translate("VCommandLine", "Unsupported SeamlyLayout export format: %1.").arg(format);
    }

    const QString text_mode = OptSvgTextMode();
    const QStringList supported_text_modes{
        QStringLiteral("designerFont"), QStringLiteral("singleLineFont"),
        QStringLiteral("hersheyStrokes"), QStringLiteral("asSupplied")
    };
    if (parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_SVG_TEXT_MODE)))
        && (format != QStringLiteral("svg") || !supported_text_modes.contains(text_mode)))
    {
        return translate("VCommandLine", "Invalid --svg-text-mode for the selected export format.");
    }

    if (parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_SCALE))))
    {
        bool scale_ok = false;
        const double scale = OptLayoutScale().toDouble(&scale_ok);
        if (!scale_ok || !std::isfinite(scale) || scale <= 0.0 ||
            scale > static_cast<double>((std::numeric_limits<float>::max)()) ||
            static_cast<float>(scale) <= 0.0F || format != QStringLiteral("png"))
        {
            return translate("VCommandLine", "--layout-scale requires a positive PNG export scale.");
        }
    }

    return QString();
}

// --------------------
/*
 * @brief Returns the requested output format, defaulting to SVG.
 */
QString VCommandLine::OptLayoutFormat() const
{
    const QString format =
        parser.value(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_FORMAT))).trimmed();
    return format.isEmpty() ? QStringLiteral("svg") : format.toLower();
}

// --------------------
/*
 * @brief Returns the absolute SeamlyLayout output path.
 */
QString VCommandLine::OptLayoutOutput() const
{
    const QString path =
        parser.value(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_OUTPUT))).trimmed();
    return path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
}

// --------------------
/*
 * @brief Returns the absolute optional layout settings path.
 */
QString VCommandLine::OptLayoutSettings() const
{
    const QString path =
        parser.value(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_SETTINGS))).trimmed();
    return path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
}

// --------------------
/*
 * @brief Returns the absolute optional export options path.
 */
QString VCommandLine::OptLayoutExportOptions() const
{
    const QString path =
        parser.value(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_EXPORT_OPTIONS))).trimmed();
    return path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
}

// --------------------
/*
 * @brief Returns the requested SVG text mode, defaulting to designerFont.
 */
QString VCommandLine::OptSvgTextMode() const
{
    const QString mode =
        parser.value(*optionsUsed.value(optionsIndex.value(LONG_OPTION_SVG_TEXT_MODE))).trimmed();
    if (mode.isEmpty())
    {
        return QStringLiteral("designerFont");
    }

    const QStringList supported_modes{
        QStringLiteral("designerFont"), QStringLiteral("singleLineFont"),
        QStringLiteral("hersheyStrokes"), QStringLiteral("asSupplied")
    };
    for (const QString &supported_mode : supported_modes)
    {
        if (supported_mode.compare(mode, Qt::CaseInsensitive) == 0)
        {
            return supported_mode;
        }
    }

    return mode;
}

// --------------------
/*
 * @brief Returns the requested PNG scale, defaulting to 1.0.
 */
QString VCommandLine::OptLayoutScale() const
{
    const QString scale =
        parser.value(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_SCALE))).trimmed();
    return scale.isEmpty() ? QStringLiteral("1.0") : scale;
}

// --------------------
/*
 * @brief Returns whether an explicit SVG text mode was supplied.
 */
bool VCommandLine::IsSetSvgTextMode() const
{
    return parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_SVG_TEXT_MODE)));
}

// --------------------
/*
 * @brief Returns whether an explicit PNG scale was supplied.
 */
bool VCommandLine::IsSetLayoutScale() const
{
    return parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_LAYOUT_SCALE)));
}

#undef translate
