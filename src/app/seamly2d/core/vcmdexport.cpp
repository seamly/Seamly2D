/**********************************************************************
 **
 **  @file   vcmdexport.cpp
 **  @author Alex Zaharov <alexzkhr@gmail.com>
 **  @date   25 8, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentine project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Seamly2D project
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
 * This method defines all supported CLI options for Seamly2D's test mode and display features.
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
        translate("VCommandLine", "Path to custom measure file (test mode)."),
        translate("VCommandLine", "The measure file")));

    // --gsize: Size value for multi-size patterns
    optionsIndex.insert(LONG_OPTION_GRADATIONSIZE, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_GRADATIONSIZE, LONG_OPTION_GRADATIONSIZE},
        translate("VCommandLine", "Set size value a pattern file, that was opened with multisize "
                                  "measurements (test mode). Valid values: %1cm.")
                                 .arg(MeasurementVariable::WholeListSizes(Unit::Cm).join(", ")),
        translate("VCommandLine", "The size value")));

    // --gheight: Height value for multi-size patterns
    optionsIndex.insert(LONG_OPTION_GRADATIONHEIGHT, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_GRADATIONHEIGHT, LONG_OPTION_GRADATIONHEIGHT},
        translate("VCommandLine", "Set height value a pattern file, that was opened with multisize "
                                  "measurements (test mode). Valid values: %1cm.")
                                  .arg(MeasurementVariable::WholeListHeights(Unit::Cm).join(", ")),
        translate("VCommandLine", "The height value")));

    // --test: Run program in test mode with single pattern file without showing mainwindow
    optionsIndex.insert(LONG_OPTION_TEST, index++);
    options.append(new QCommandLineOption({SINGLE_OPTION_TEST, LONG_OPTION_TEST},
        translate("VCommandLine", "Run the program in a test mode. The program in "
                                  "this mode loads a single pattern file and silently quit without "
                                  "showing the main window.")));

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
    commandLine->parser.process(app);          // Process CLI arguments with the parser

    // FIXME: Add more conditions here if new CLI modes are introduced that should disable the GUI
    commandLine->isGuiEnabled = !commandLine->IsTestModeEnabled();

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
 * Only returns the measurement file path if the --measurefile option is set
 * and the app is running in test mode.
 *
 * @return QString containing the measurement file path, or empty if not applicable.
 */
QString VCommandLine::OptMeasurePath() const
{
    QString measure;
    if (parser.isSet(*optionsUsed.value(optionsIndex.value(LONG_OPTION_MEASUREFILE)))
        && IsTestModeEnabled())
        // TODO: Currently restricting general use of measure file until window handling is improved
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

#undef translate
