// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file StartupOptions.h
// @brief Command-line parsing for SeamlyLayout's single positional <svg-file>
//        argument, plus the validation of that file.
//
// This is the SeamlyLayout half of the Seamly2D Layout Mode handoff (Task 49).
// Seamly2D writes "<pattern>.pieces.svg" beside the pattern file and launches
//
//     SeamlyLayout <absolute path to .pieces.svg>
//
// detached (MainWindow::exportPiecesToSeamlyLayout(), which builds the argument
// vector through SeamlyFamilyPaths::seamlyLayoutLaunchArguments()).  Before this
// class existed, main.cpp handed argc/argv to QApplication and read nothing
// else, so the handoff opened an empty canvas and the user had to re-find the
// file Seamly2D had just written.
//
// The class is a plain value type — no QObject, no Qt application instance
// required — so the whole contract can be unit tested (see
// src/test/SeamlyLayoutTest/StartupOptionsTests.cpp) without starting the GUI.
//
// Accepted argument forms:
//   SeamlyLayout                       start with an empty canvas
//   SeamlyLayout <file.svg>            open that file
//   SeamlyLayout -h | --help           show usage, exit 0
//   SeamlyLayout -v | --version        show the version, exit 0
//
// Usage:
//   const StartupOptions options = StartupOptions::parse(QCoreApplication::arguments());
//   switch (options.status()) { ... }

#pragma once

#include <QString>
#include <QStringList>

// @brief Parsed and validated SeamlyLayout command line.
//
// Produced by the static parse() factory; the four private members are set
// once there and only read afterwards.  status() says what the caller must do;
// filePath() and message() are the payload for the respective statuses.
class StartupOptions
{
public:
    // @brief What main() should do with the parsed command line.
    enum class Status
    {
        // No file was given — start normally with an empty canvas.
        NoFile,
        // filePath() holds an existing, readable SVG file to open at startup.
        OpenFile,
        // message() holds --help / --version text; show it and exit 0.
        ShowInformation,
        // message() holds the reason the command line could not be honoured
        // (missing file, unreadable file, not an SVG, too many arguments).
        // The application still starts, with an empty canvas, after showing it.
        Failed
    }; // enum class Status

    // @brief Parse and validate a command line.
    // @param arguments Full argument list including the program name at index 0,
    //        exactly as QCoreApplication::arguments() returns it.
    // @return A fully populated StartupOptions; never throws, never exits.
    static StartupOptions parse(const QStringList &arguments);

    // @brief What the caller must do with this command line.
    Status status() const { return m_status; }

    // @brief Absolute path of the SVG to open. Empty unless status() is OpenFile.
    QString filePath() const { return m_filePath; }

    // @brief Text to show the user. Empty unless status() is ShowInformation or Failed.
    QString message() const { return m_message; }

    // @brief True when an SVG file should be opened at startup.
    bool hasFile() const { return m_status == Status::OpenFile; }

    // @brief True when the command line could not be honoured.
    bool hasError() const { return m_status == Status::Failed; }

private:
    // Result of the parse; NoFile is the "nothing was asked of us" default.
    Status m_status = Status::NoFile;

    // Absolute path of the file to open; set only for Status::OpenFile.
    QString m_filePath;

    // User-visible text; set only for Status::ShowInformation and Status::Failed.
    QString m_message;
}; // class StartupOptions
