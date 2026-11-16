//---------------------------------------------------------------------------------------------------------------------
/// @file   command_line.cpp
/// @author Douglas S Caskey
/// @date   14 Nov, 2026
///
/// @brief
/// @copyright
/// This source code is part of the Seamly2D project, a pattern making
/// program to create and model patterns of clothing.
/// Copyright (C) 2017-2026 Seamly2D project
/// <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
///
/// Seamly2D is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
//
/// Seamly2D is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
/// @file   vcmdexport.cpp
/// @author Alex Zaharov <alexzkhr@gmail.com>
/// @date   25 8, 2015
///
/// @brief
/// @copyright
/// This source code is part of the Valentine project, a pattern making
/// program, whose allow create and modeling patterns of clothing.
/// Copyright (C) 2015 Seamly2D project
/// <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
///
/// Seamly2D is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// Seamly2D is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

#include "command_line.h"
#include "../vformat/measurements.h"
#include "../vmisc/commandoptions.h"
#include "../vmisc/def.h"

#include <QDebug>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// @brief Connects stdout and stderr to the parent console when the process has no standard handles.
///
/// A Windows GUI process starts without a console. A redirect (`> file`, a pipe) gives it valid handles,
/// and those stay untouched.
///
/// @return true when stdout or stderr reaches a console, a file or a pipe.
//---------------------------------------------------------------------------------------------------------------------
bool connectStandardStreams()
{
    bool connected = true;
#ifdef Q_OS_WIN
    const HANDLE standard_output = GetStdHandle(STD_OUTPUT_HANDLE);
    const HANDLE standard_error = GetStdHandle(STD_ERROR_HANDLE);
    connected = (standard_output != nullptr && standard_output != INVALID_HANDLE_VALUE) ||
                (standard_error != nullptr && standard_error != INVALID_HANDLE_VALUE);

    // GetConsoleWindow() covers a second call: AttachConsole() fails once the process has a console.
    if (!connected && (AttachConsole(ATTACH_PARENT_PROCESS) || GetConsoleWindow() != nullptr))
    {
        FILE *console_stream = nullptr;
        freopen_s(&console_stream, "CONOUT$", "w", stdout);
        freopen_s(&console_stream, "CONOUT$", "w", stderr);
        connected = true;
    }
#endif
    return connected;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Checks whether an argument selects a mode that writes to the console.
///
/// Matches `--name`, `--name=value` and the single-letter forms of test mode, layout export, layout info,
/// measurement and gradation overrides, help and version.
///
/// @param argument One application argument.
/// @return true for a console-mode argument.
//---------------------------------------------------------------------------------------------------------------------
bool isConsoleModeArgument(const QString &argument)
{
    if (argument.startsWith(QStringLiteral("--layout-")) || argument.startsWith(QStringLiteral("--svg-text-mode")))
    {
        return true;
    }

    const QString name = argument.section(QLatin1Char('='), 0, 0);
    static const QStringList console_names{
        QStringLiteral("--test"), QStringLiteral("--mfile"), QStringLiteral("--gsize"), QStringLiteral("--gheight"),
        QStringLiteral("--text2paths"),
        QStringLiteral("--help"), QStringLiteral("--version"), QStringLiteral("-t"), QStringLiteral("-m"),
        QStringLiteral("-x"), QStringLiteral("-e"), QStringLiteral("-h"), QStringLiteral("-v")
    };
    return console_names.contains(name);
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Connects the standard streams when an argument selects a console mode.
///
/// A normal GUI launch from a console stays detached from that console.
///
/// @param arguments The application arguments, program name first.
//---------------------------------------------------------------------------------------------------------------------
void attachParentConsoleForCommandLine(const QStringList &arguments)
{
#ifdef Q_OS_WIN
    if (std::any_of(arguments.cbegin() + (arguments.isEmpty() ? 0 : 1), arguments.cend(), isConsoleModeArgument))
    {
        connectStandardStreams();
    }
#else
    Q_UNUSED(arguments);
#endif
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Writes one command-line error line to standard error.
///
/// Connects the console first: an unknown option is not a console-mode argument, so the console
/// may still be detached. Writes plain text: qCritical() would quote the string and, before the log
/// handler exists, send it to the debugger instead of the console.
///
/// @param message Error text without a line end.
//---------------------------------------------------------------------------------------------------------------------
void printCommandLineError(const QString &message)
{
    connectStandardStreams();
    vStdErr() << message << Qt::endl;
}
} // namespace

VCommandLinePtr VCommandLine::command_line = nullptr;

#define translate(context, source) QCoreApplication::translate((context), (source))

//---------------------------------------------------------------------------------------------------------------------
/// @brief Constructor for the VCommandLine class.
///
/// Initializes the command-line parser with application description, standard help/version
/// options, a positional argument for the pattern file, and all custom options defined
/// in the application.
//---------------------------------------------------------------------------------------------------------------------
VCommandLine::VCommandLine()
    : parser()
    , m_options_used()
    , m_options_index()
    , m_is_gui_enabled(false)
    , m_reserves_stdout(false)
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
    initOptions(m_options_used, m_options_index);

    // Iterate over each command-line option in m_options_used and add it to the parser
    VCommandLineOptions::const_iterator i = m_options_used.constBegin();
    while (i != m_options_used.constEnd())
    {
        parser.addOption(*(*i));  // Dereference the pointer to QCommandLineOption and add it
        ++i;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Displays command-line help text and terminates the application.
///
/// Prints the help text to stdout when it reaches a console, a file or a pipe.
/// Otherwise (Windows launch without a console) shows the text in a scrollable dialog.
///
/// @param exit_code Process exit code passed to std::exit().
//---------------------------------------------------------------------------------------------------------------------
[[noreturn]] void VCommandLine::showHelp(int exit_code) const
{
    const QString text = parser.helpText();

    if (connectStandardStreams())
    {
        QTextStream(stdout) << text << Qt::endl;
    }
    else
    {
        QDialog dialog;
        dialog.setWindowTitle(QCoreApplication::applicationName());
        dialog.resize(750, 550);

        auto *layout = new QVBoxLayout(&dialog);
        auto *text_edit = new QPlainTextEdit(&dialog);
        text_edit->setReadOnly(true);
        text_edit->setLineWrapMode(QPlainTextEdit::NoWrap);
        text_edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        text_edit->setPlainText(text);

        auto *button_box = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
        QObject::connect(button_box, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

        layout->addWidget(text_edit);
        layout->addWidget(button_box);
        dialog.exec();
    }

    std::exit(exit_code);
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Initializes the list of command-line options and their index.
///
/// This method defines the supported test-mode, display, and layout-export options.
/// Options are added in a grouped, user-friendly order for consistency in help output.
///
/// @param options A reference to the list where all VCommandLineOptions will be stored.
/// @param options_index A map that associates each long-form option string with its index
///        in the options list.
//---------------------------------------------------------------------------------------------------------------------
void VCommandLine::initOptions(VCommandLineOptions &options, QMap<QString, int> &options_index)
{
    int index = 0;

    // Options are grouped for usability; order matters for help display
    // ================================================================================================================

    // --mfile: Path to custom measurement file
    options_index.insert(LONG_OPTION_MEASUREFILE, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_MEASUREFILE, LONG_OPTION_MEASUREFILE},
        translate("VCommandLine", "Path to a measurement file for test mode or CLI export."),
        translate("VCommandLine", "The measure file")));

    // --gsize: Size value for multi-size patterns
    options_index.insert(LONG_OPTION_GRADATIONSIZE, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_GRADATIONSIZE, LONG_OPTION_GRADATIONSIZE},
        translate("VCommandLine", "Set the size for a multisize pattern in test mode or CLI export. "
                                  "Valid values: %1cm.")
                                 .arg(MeasurementVariable::WholeListSizes(Unit::Cm).join(", ")),
        translate("VCommandLine", "The size value")));

    // --gheight: Height value for multi-size patterns
    options_index.insert(LONG_OPTION_GRADATIONHEIGHT, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_GRADATIONHEIGHT, LONG_OPTION_GRADATIONHEIGHT},
        translate("VCommandLine", "Set the height for a multisize pattern in test mode or CLI export. "
                                  "Valid values: %1cm.")
                                  .arg(MeasurementVariable::WholeListHeights(Unit::Cm).join(", ")),
        translate("VCommandLine", "The height value")));

    // --test: Run program in test mode with single pattern file without showing mainwindow
    options_index.insert(LONG_OPTION_TEST, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_TEST, LONG_OPTION_TEST},
        translate("VCommandLine", "Run the program in a test mode. The program in "
                                  "this mode loads a single pattern file and silently quit without "
                                  "showing the main window.")));

    // --layout-export: Generate a layout through SeamlyLayout without either UI.
    options_index.insert(LONG_OPTION_LAYOUT_EXPORT, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_EXPORT},
        translate("VCommandLine", "Generate a layout through SeamlyLayout without showing either application.")));

    // --layout-format: SeamlyLayout output format.
    options_index.insert(LONG_OPTION_LAYOUT_FORMAT, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_FORMAT},
        translate("VCommandLine", "SeamlyLayout output format: svg, pdf, pdf-tiled, png, jpg, jpeg, dxf, hpgl, ps, eps, "
                                  "gcode, or 3mf. Default: svg."),
        translate("VCommandLine", "format")));

    // --layout-output: Destination file for SeamlyLayout export.
    options_index.insert(LONG_OPTION_LAYOUT_OUTPUT, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_OUTPUT},
        translate("VCommandLine", "Output file path for --layout-export."),
        translate("VCommandLine", "path")));

    // --layout-settings: Layout settings JSON file passed to SeamlyLayout.
    options_index.insert(LONG_OPTION_LAYOUT_SETTINGS, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_SETTINGS},
        translate("VCommandLine", "Layout settings JSON file for SeamlyLayout. "
                                  "Default: the settings SeamlyLayout saved last."),
        translate("VCommandLine", "path")));

    // --layout-export-options: Format-specific JSON options passed to SeamlyLayout.
    options_index.insert(LONG_OPTION_LAYOUT_EXPORT_OPTIONS, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_EXPORT_OPTIONS},
        translate("VCommandLine", "Format-specific export options JSON file for SeamlyLayout."),
        translate("VCommandLine", "path")));

    // --svg-text-mode: SVG label rendering mode.
    options_index.insert(LONG_OPTION_SVG_TEXT_MODE, index++);
    options.append(new QCommandLineOption({LONG_OPTION_SVG_TEXT_MODE},
        translate("VCommandLine", "SVG text mode: designerFont, singleLineFont, hersheyStrokes, or asSupplied. "
                                  "SVG only."),
        translate("VCommandLine", "mode")));

    // --layout-material: material of a multi-material pattern.
    options_index.insert(LONG_OPTION_LAYOUT_MATERIAL, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_MATERIAL},
        translate("VCommandLine", "Material to export, such as Fabric or Lining. "
                                  "Required when the pattern has several materials."),
        translate("VCommandLine", "name")));

    // --layout-view: layout of a multisize pattern.
    options_index.insert(LONG_OPTION_LAYOUT_VIEW, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_VIEW},
        translate("VCommandLine", "Multisize layout to export: marker, nested, base, or a size as its layout tab "
                                  "shows it, such as 500 for Size 500. Default: the first layout."),
        translate("VCommandLine", "name")));

    // --layout-oversize: pieces wider than a roll or fabric.
    options_index.insert(LONG_OPTION_LAYOUT_OVERSIZE, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_OVERSIZE},
        translate("VCommandLine", "Pieces wider than a roll or fabric: split (split the pieces) or extend "
                                  "(make the media longer). Without it such pieces stop the export."),
        translate("VCommandLine", "choice")));

    // --layout-info: JSON report of materials, layouts, oversized pieces and formats.
    options_index.insert(LONG_OPTION_LAYOUT_INFO, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_INFO},
        translate("VCommandLine", "Print a JSON report of the materials, layouts, oversized pieces and export "
                                  "formats of the pattern with the layout settings. Exports nothing.")));

    // --text2paths: label text as paths in vector exports.
    options_index.insert(LONG_OPTION_TEXT2PATHS, index++);
    options.append(new QCommandLineOption({LONG_OPTION_TEXT2PATHS},
        translate("VCommandLine", "Write label text as paths: SVG labels as single-stroke paths, PDF text as glyph "
                                  "outlines. PS and EPS draw paths already. For svg, pdf, pdf-tiled, ps and eps.")));

    // --no-scaling: Disable high DPI auto-scaling (useful on some systems)
    options_index.insert(LONG_OPTION_NO_HDPI_SCALING, index++);
    options.append(new QCommandLineOption({LONG_OPTION_NO_HDPI_SCALING},
        translate("VCommandLine", "Disable high dpi scaling. Call this option if has problem "
                                  "with scaling (by default scaling enabled). Alternatively you "
                                  "can use the %1 environment variable.")
                                  .arg("QT_AUTO_SCREEN_SCALE_FACTOR=0")));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Retrieves the singleton instance of VCommandLine and processes CLI arguments.
///
/// If no VCommandLine exists, a new one is created. It then parses the application arguments,
/// handles help and version, and decides whether the GUI is enabled.
///
/// @param app The current application instance containing CLI arguments.
/// @return VCommandLinePtr A shared pointer to the global VCommandLine instance.
//---------------------------------------------------------------------------------------------------------------------
VCommandLinePtr VCommandLine::getCommandLine(const QCoreApplication &app)
{
    if (command_line == nullptr)
    {
        command_line.reset(new VCommandLine()); // Replace pointer with new VCommandLine
    }
    attachParentConsoleForCommandLine(app.arguments());

    if (!command_line->parser.parse(app.arguments()))
    {
        printCommandLineError(command_line->parser.errorText());
        command_line->showHelp(V_EX_USAGE);
    }

    if (command_line->parser.isSet(QStringLiteral("help")))
    {
        command_line->showHelp(0);
    }

    if (command_line->parser.isSet(QStringLiteral("version")))
    {
        command_line->parser.showVersion();
    }

    // Each mode that runs without the GUI must be added here.
    const bool test_mode = command_line->isTestModeEnabled();
    const bool layout_export_mode = command_line->isLayoutExportEnabled();
    const bool layout_info_mode = command_line->isLayoutInfoEnabled();
    if (test_mode && (layout_export_mode || layout_info_mode))
    {
        printCommandLineError(translate("VCommandLine", "--test cannot be used with --layout-export or --layout-info."));
        command_line->showHelp(V_EX_USAGE);
    }
    command_line->m_is_gui_enabled = !test_mode && !layout_export_mode && !layout_info_mode;
    command_line->m_reserves_stdout = layout_export_mode || layout_info_mode;

    return command_line;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Destructor for VCommandLine.
///
/// Cleans up dynamically allocated list of VCommandLineOptions stored in m_options_used,
/// ensuring no memory leaks. Clears the list after deleting.
//---------------------------------------------------------------------------------------------------------------------
VCommandLine::~VCommandLine()
{
    qDeleteAll(m_options_used.begin(), m_options_used.end()); // Delete all dynamically allocated options
    m_options_used.clear();                                // Clear the list to reset state
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Resets the singleton instance of VCommandLine.
///
/// This destroys the current VCommandLine, allowing a fresh one to be created next time getCommandLine() is called.
/// Useful for tests or reinitializing command-line parsing.
//---------------------------------------------------------------------------------------------------------------------
void VCommandLine::reset()
{
    command_line.reset(); // Calls the VCommandLine destructor.
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Checks whether the application is running in test mode.
///
/// Determines if the `--test` command-line option is set. If it is, the function
/// verifies that exactly one positional argument (input file) is provided. If not,
/// it prints an error and terminates execution.
///
/// @return true if test mode is enabled; otherwise false.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isTestModeEnabled() const
{
    const bool r = parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_TEST)));

    // Ensure exactly one positional argument is provided when in test mode
    if (r && parser.positionalArguments().size() != 1)
    {
        printCommandLineError(translate("VCommandLine", "Test option can be used with single input file only."));
        showHelp(V_EX_USAGE);
    }

    return r;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Checks whether high DPI scaling is disabled.
///
/// Determines if the --no-hdpi-scaling option is set via the command-line.
///
/// @return true if scaling is disabled; otherwise false.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isNoScalingEnabled() const
{
    return parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_NO_HDPI_SCALING)));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Retrieves the path to the measurement file if applicable.
///
/// Returns a measurement override only for test, layout-export or layout-info mode.
/// A relative path is resolved against the launch directory, as the pattern file is. The pattern
/// loader resolves a relative path against the pattern folder, so it must not see one.
///
/// @return Absolute measurement file path, or empty if not applicable.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::measurementPathOption() const
{
    QString measure;
    if (parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_MEASUREFILE)))
        && (isTestModeEnabled() || isLayoutExportEnabled() || isLayoutInfoEnabled()))
    {
        const QString path = parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_MEASUREFILE))).trimmed();
        measure = path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
    }

    return measure;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Retrieves the list of input filenames passed as positional arguments.
///
/// @return QStringList of input file names.
//---------------------------------------------------------------------------------------------------------------------
QStringList VCommandLine::inputFileNamesOption() const
{
    return parser.positionalArguments();
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Checks whether the GUI should be enabled based on current CLI mode.
///
/// @return true if GUI mode is enabled; otherwise false.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isGuiEnabled() const
{
    return m_is_gui_enabled;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Checks if a specific gradation size is set via the CLI.
///
/// @return true if --gradationsize option is set; otherwise false.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isGradationSizeSet() const
{
    return parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_GRADATIONSIZE)));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Checks if a specific gradation height is set via the CLI.
///
/// @return true if --gradationheight option is set; otherwise false.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isGradationHeightSet() const
{
    return parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_GRADATIONHEIGHT)));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Retrieves the gradation size value from the command-line.
///
/// This function fetches the value of the --gradationsize option and validates it
/// using MeasurementVariable::IsGradationSizeValid(). If invalid, it prints an error
/// and exits the application via showHelp().
///
/// @return QString containing a valid gradation size.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::gradationSizeOption() const
{
    const QString size = parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_GRADATIONSIZE)));
    if (!MeasurementVariable::IsGradationSizeValid(size))
    {
        printCommandLineError(translate("VCommandLine", "Invalid gradation size value: %1.").arg(size));
        showHelp(V_EX_USAGE);
    }

    return size;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Retrieves the gradation height value from the command-line.
///
/// This function fetches the value of the --gradationheight option and validates it
/// using MeasurementVariable::IsGradationHeightValid(). If invalid, it prints an error
/// and exits the application via showHelp().
///
/// @return QString containing a valid gradation height.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::gradationHeightOption() const
{
    const QString height = parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_GRADATIONHEIGHT)));
    if (!MeasurementVariable::IsGradationHeightValid(height))
    {
        printCommandLineError(translate("VCommandLine", "Invalid gradation height value: %1.").arg(height));
        showHelp(V_EX_USAGE);
    }

    return height;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Checks whether the CLI export options form a valid headless invocation.
///
/// Prints the validation error and exits through showHelp() when the options are invalid.
///
/// @return true when --layout-export is set and its companions are valid.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isLayoutExportEnabled() const
{
    const QString error = layoutExportError();
    if (!error.isEmpty())
    {
        printCommandLineError(error);
        showHelp(V_EX_USAGE);
    }

    return parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_EXPORT)));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Checks whether --layout-info asks for the SeamlyLayout JSON report.
///
/// Validation is shared with isLayoutExportEnabled(): an invalid combination prints the error and exits.
///
/// @return true when --layout-info is set and its companions are valid.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isLayoutInfoEnabled() const
{
    const QString error = layoutExportError();
    if (!error.isEmpty())
    {
        printCommandLineError(error);
        showHelp(V_EX_USAGE);
    }

    return parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_INFO)));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns a validation error for an incomplete or invalid layout export request.
///
/// @return Empty when export options are valid or no export was requested.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::layoutExportError() const
{
    const auto is_set = [this](const QString &name)
    {
        return parser.isSet(*m_options_used.value(m_options_index.value(name)));
    };

    const bool export_requested = is_set(LONG_OPTION_LAYOUT_EXPORT);
    const bool info_requested = is_set(LONG_OPTION_LAYOUT_INFO);
    // Options that only an export uses; --layout-settings also serves --layout-info.
    const bool has_export_choice =
        is_set(LONG_OPTION_LAYOUT_FORMAT) || is_set(LONG_OPTION_LAYOUT_OUTPUT) ||
        is_set(LONG_OPTION_LAYOUT_EXPORT_OPTIONS) || is_set(LONG_OPTION_SVG_TEXT_MODE) ||
        is_set(LONG_OPTION_LAYOUT_MATERIAL) ||
        is_set(LONG_OPTION_LAYOUT_VIEW) || is_set(LONG_OPTION_LAYOUT_OVERSIZE) || is_set(LONG_OPTION_TEXT2PATHS);

    if (export_requested && info_requested)
    {
        return translate("VCommandLine", "--layout-export and --layout-info cannot be used together.");
    }
    if (info_requested)
    {
        if (has_export_choice)
        {
            return translate("VCommandLine", "--layout-info prints a report and exports nothing; "
                                             "it takes only --layout-settings, --mfile, --gsize and --gheight.");
        }
        if (parser.positionalArguments().size() != 1)
        {
            return translate("VCommandLine", "--layout-info requires one input pattern file.");
        }
        return QString();
    }
    if (!export_requested && (has_export_choice || is_set(LONG_OPTION_LAYOUT_SETTINGS)))
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

    if (parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_OUTPUT))).trimmed().isEmpty())
    {
        return translate("VCommandLine", "--layout-export requires --layout-output <path>.");
    }

    const QString format = layoutFormatOption();
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

    const QString text_mode = svgTextModeOption();
    const QStringList supported_text_modes{
        QStringLiteral("designerFont"), QStringLiteral("singleLineFont"),
        QStringLiteral("hersheyStrokes"), QStringLiteral("asSupplied")
    };
    if (parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_SVG_TEXT_MODE)))
        && (format != QStringLiteral("svg") || !supported_text_modes.contains(text_mode)))
    {
        return translate("VCommandLine", "Invalid --svg-text-mode for the selected export format.");
    }

    if (is_set(LONG_OPTION_TEXT2PATHS))
    {
        if (is_set(LONG_OPTION_SVG_TEXT_MODE))
        {
            return translate("VCommandLine", "--text2paths and --svg-text-mode cannot be used together.");
        }
        const QStringList text_formats{QStringLiteral("svg"), QStringLiteral("pdf"), QStringLiteral("pdf-tiled"),
                                       QStringLiteral("ps"), QStringLiteral("eps")};
        if (!text_formats.contains(format))
        {
            return translate("VCommandLine", "--text2paths applies to svg, pdf, pdf-tiled, ps and eps; "
                                             "%1 has no text to convert.").arg(format);
        }
    }

    if (is_set(LONG_OPTION_LAYOUT_OVERSIZE))
    {
        const QString oversize = layoutOversizeOption();
        if (oversize != QStringLiteral("split") && oversize != QStringLiteral("extend"))
        {
            return translate("VCommandLine", "--layout-oversize must be split or extend.");
        }
    }

    for (const QString &name : {LONG_OPTION_LAYOUT_MATERIAL, LONG_OPTION_LAYOUT_VIEW})
    {
        if (is_set(name) && parser.value(*m_options_used.value(m_options_index.value(name))).trimmed().isEmpty())
        {
            return translate("VCommandLine", "--%1 needs a name.").arg(name);
        }
    }

    if (is_set(LONG_OPTION_LAYOUT_VIEW) && (isGradationSizeSet() || isGradationHeightSet()))
    {
        return translate("VCommandLine", "--layout-view selects a layout of every size; "
                                         "--gsize and --gheight export one size without views.");
    }

    return QString();
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns the requested output format, defaulting to SVG.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::layoutFormatOption() const
{
    const QString format =
        parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_FORMAT))).trimmed();
    return format.isEmpty() ? QStringLiteral("svg") : format.toLower();
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns the absolute SeamlyLayout output path.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::layoutOutputOption() const
{
    const QString path =
        parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_OUTPUT))).trimmed();
    return path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns the absolute optional layout settings path.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::layoutSettingsOption() const
{
    const QString path =
        parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_SETTINGS))).trimmed();
    return path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns the absolute optional export options path.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::layoutExportOptionsOption() const
{
    const QString path =
        parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_EXPORT_OPTIONS))).trimmed();
    return path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns the requested SVG text mode, defaulting to designerFont.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::svgTextModeOption() const
{
    const QString mode =
        parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_SVG_TEXT_MODE))).trimmed();
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

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns the --layout-material name; empty when not set.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::layoutMaterialOption() const
{
    return parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_MATERIAL))).trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns the --layout-view name; empty when not set.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::layoutViewOption() const
{
    return parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_VIEW))).trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns the --layout-oversize choice in lower case; empty when not set.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::layoutOversizeOption() const
{
    return parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_OVERSIZE))).trimmed().toLower();
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns whether --text2paths asks for label text as paths.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isText2PathsSet() const
{
    return parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_TEXT2PATHS)));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns whether an explicit SVG text mode was supplied.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isSvgTextModeSet() const
{
    return parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_SVG_TEXT_MODE)));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns whether standard output carries only the result of --layout-export or --layout-info.
///
/// The log handler then writes debug and info lines to the log file only, so a caller can parse the output.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::reservesStandardOutput() const
{
    return m_reserves_stdout;
}


#undef translate
