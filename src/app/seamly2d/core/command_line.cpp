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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
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
/// @brief Connects the standard streams when an argument selects test mode, layout export, help or version.
///
/// A normal GUI launch from a console stays detached from that console.
///
/// @param arguments The application arguments, program name first.
//---------------------------------------------------------------------------------------------------------------------
void attachParentConsoleForCommandLine(const QStringList &arguments)
{
#ifdef Q_OS_WIN
    bool is_command_line_mode = false;
    for (const QString &argument : arguments)
    {
        is_command_line_mode = is_command_line_mode
            || argument.startsWith(QStringLiteral("--layout-"))
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
            || argument == QStringLiteral("--version");
    }

    if (is_command_line_mode)
    {
        connectStandardStreams();
    }
#else
    Q_UNUSED(arguments);
#endif
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
        translate("VCommandLine", "SeamlyLayout output format: svg, pdf, pdf-tiled, png, jpg, jpeg, dxf, hpgl, ps, eps, gcode, or 3mf."),
        translate("VCommandLine", "format")));

    // --layout-output: Destination file for SeamlyLayout export.
    options_index.insert(LONG_OPTION_LAYOUT_OUTPUT, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_OUTPUT},
        translate("VCommandLine", "Output file path for --layout-export."),
        translate("VCommandLine", "path")));

    // --layout-settings: Layout settings JSON file passed to SeamlyLayout.
    options_index.insert(LONG_OPTION_LAYOUT_SETTINGS, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_SETTINGS},
        translate("VCommandLine", "Layout settings JSON file for SeamlyLayout."),
        translate("VCommandLine", "path")));

    // --layout-export-options: Format-specific JSON options passed to SeamlyLayout.
    options_index.insert(LONG_OPTION_LAYOUT_EXPORT_OPTIONS, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_EXPORT_OPTIONS},
        translate("VCommandLine", "Format-specific export options JSON file for SeamlyLayout."),
        translate("VCommandLine", "path")));

    // --svg-text-mode: SVG label rendering mode.
    options_index.insert(LONG_OPTION_SVG_TEXT_MODE, index++);
    options.append(new QCommandLineOption({LONG_OPTION_SVG_TEXT_MODE},
        translate("VCommandLine", "SVG text mode: designerFont, singleLineFont, hersheyStrokes, or asSupplied."),
        translate("VCommandLine", "mode")));

    // --layout-scale: PNG scale factor passed to SeamlyLayout.
    options_index.insert(LONG_OPTION_LAYOUT_SCALE, index++);
    options.append(new QCommandLineOption({LONG_OPTION_LAYOUT_SCALE},
        translate("VCommandLine", "PNG export scale factor (default 1.0)."),
        translate("VCommandLine", "factor")));

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
        qCritical() << command_line->parser.errorText();
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
    if (test_mode && layout_export_mode)
    {
        qCritical() << translate("VCommandLine", "--test and --layout-export cannot be used together.");
        command_line->showHelp(V_EX_USAGE);
    }
    command_line->m_is_gui_enabled = !test_mode && !layout_export_mode;

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
        qCritical() << translate("VCommandLine", "Test option can be used with single input file only.") << "\n";
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
/// Returns a measurement override only for test or layout-export mode.
///
/// @return QString containing the measurement file path, or empty if not applicable.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::measurementPathOption() const
{
    QString measure;
    if (parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_MEASUREFILE)))
        && (isTestModeEnabled() || isLayoutExportEnabled()))
    {
        measure = parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_MEASUREFILE)));
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
        qCritical() << translate("VCommandLine", "Invalid gradation size value.") << "\n";
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
        qCritical() << translate("VCommandLine", "Invalid gradation height value.") << "\n";
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
        qCritical() << error;
        showHelp(V_EX_USAGE);
    }

    return parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_EXPORT)));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns a validation error for an incomplete or invalid layout export request.
///
/// @return Empty when export options are valid or no export was requested.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::layoutExportError() const
{
    const bool export_requested =
        parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_EXPORT)));
    const bool has_export_option =
        parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_FORMAT))) ||
        parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_OUTPUT))) ||
        parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_SETTINGS))) ||
        parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_EXPORT_OPTIONS))) ||
        parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_SVG_TEXT_MODE))) ||
        parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_SCALE)));

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

    if (parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_SCALE))))
    {
        bool scale_ok = false;
        const double scale = layoutScaleOption().toDouble(&scale_ok);
        if (!scale_ok || !std::isfinite(scale) || scale <= 0.0 ||
            scale > static_cast<double>((std::numeric_limits<float>::max)()) ||
            static_cast<float>(scale) <= 0.0F || format != QStringLiteral("png"))
        {
            return translate("VCommandLine", "--layout-scale requires a positive PNG export scale.");
        }
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
/// @brief Returns the requested PNG scale, defaulting to 1.0.
//---------------------------------------------------------------------------------------------------------------------
QString VCommandLine::layoutScaleOption() const
{
    const QString scale =
        parser.value(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_SCALE))).trimmed();
    return scale.isEmpty() ? QStringLiteral("1.0") : scale;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns whether an explicit SVG text mode was supplied.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isSvgTextModeSet() const
{
    return parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_SVG_TEXT_MODE)));
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns whether an explicit PNG scale was supplied.
//---------------------------------------------------------------------------------------------------------------------
bool VCommandLine::isLayoutScaleSet() const
{
    return parser.isSet(*m_options_used.value(m_options_index.value(LONG_OPTION_LAYOUT_SCALE)));
}

#undef translate
