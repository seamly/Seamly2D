// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file StartupOptions.h
// @brief Command-line parsing for SeamlyLayout's two startup transports — an
//        optional positional <svg-file>, or an SVG document read from standard
//        input — plus the validation of each.
//
// This is the SeamlyLayout half of the Seamly2D Layout Mode handoff.
//
// Transport 1 — a file path (Task 49, still supported):
//
//     SeamlyLayout <absolute path to some.svg>
//
// Anyone can use it: a shell, a desktop file association, or a script. It is
// the only way to open an SVG that already exists on disk.
//
// Transport 2 — a document on standard input (Seamly2D.5, the handoff):
//
//     SeamlyLayout --svg-stdin [--document-name <name>]
//
// Seamly2D serialises piece mode to one stringified SVG, launches SeamlyLayout
// with --svg-stdin, writes that string to the child's standard input and closes
// the channel. Nothing is written to disk, so no ".pieces.svg" is left beside
// the pattern and a read-only pattern directory no longer blocks Layout Mode.
// The seamly2d side builds the argument vector through
// SeamlySuitePaths::seamlyLayoutLaunchArguments().
//
// --document-name carries the pattern base name, because a document with no
// file has no name to derive default export file names from.
//
// The class is a plain value type — no QObject, no Qt application instance
// required — so the whole contract can be unit tested (see
// src/test/SeamlyLayoutTest/StartupOptionsTests.cpp) without starting the GUI.
// The standard-input read is a separate parse() overload taking a QIODevice, so
// a test can feed it a QBuffer.
//
// Accepted argument forms:
//   SeamlyLayout                       start with an empty canvas
//   SeamlyLayout <file.svg>            open that file
//   SeamlyLayout --svg-stdin           open the SVG document on standard input
//   SeamlyLayout -h | --help           show usage, exit 0
//   SeamlyLayout -v | --version        show the version, exit 0
//
// Usage:
//   QFile standardInput;
//   standardInput.open(stdin, QIODevice::ReadOnly);
//   const StartupOptions options =
//       StartupOptions::parse(QCoreApplication::arguments(), &standardInput);
//   switch (options.status()) { ... }

#pragma once

#include <QString>
#include <QStringList>

class QIODevice;

// @brief Parsed and validated SeamlyLayout command line.
//
// Produced by the static parse() factories; the private members are set once
// there and only read afterwards.  status() says what the caller must do;
// filePath(), svgDocument() and message() are the payload for the respective
// statuses.
class StartupOptions
{
public:
    // @brief What main() should do with the parsed command line.
    enum class Status
    {
        // No file and no document were given — start with an empty canvas.
        NoFile,
        // filePath() holds an existing, readable SVG file to open at startup.
        OpenFile,
        // svgDocument() holds an SVG document read from standard input.
        OpenDocument,
        // message() holds --help / --version text; show it and exit 0.
        ShowInformation,
        // message() holds the reason the command line could not be honoured
        // (missing file, unreadable file, not an SVG, too many arguments,
        // empty or unreadable standard input).
        // The application still starts, with an empty canvas, after showing it.
        Failed
    }; // enum class Status

    // @brief Parse and validate a command line that cannot use standard input.
    // Equivalent to parse(arguments, nullptr): --svg-stdin fails, because no
    // channel was offered to read the document from.
    // @param arguments Full argument list including the program name at index 0,
    //        exactly as QCoreApplication::arguments() returns it.
    // @return A fully populated StartupOptions; never throws, never exits.
    static StartupOptions parse(const QStringList &arguments);

    // @brief Parse and validate a command line, reading standard input when
    //        --svg-stdin asks for it.
    // @param arguments Full argument list including the program name at index 0.
    // @param standardInput Open, readable device holding the SVG document. Read
    //        to end only when --svg-stdin is set; may be nullptr otherwise.
    // @return A fully populated StartupOptions; never throws, never exits.
    static StartupOptions parse(const QStringList &arguments, QIODevice *standardInput);

    // @brief What the caller must do with this command line.
    Status status() const { return m_status; }

    // @brief Absolute path of the SVG to open. Empty unless status() is OpenFile.
    QString filePath() const { return m_filePath; }

    // @brief The SVG document read from standard input. Empty unless status()
    //        is OpenDocument.
    QString svgDocument() const { return m_svgDocument; }

    // @brief Name to use for default export file names. Set only for
    //        OpenDocument, and only when --document-name was given.
    QString documentName() const { return m_documentName; }

    // @brief Text to show the user. Empty unless status() is ShowInformation or Failed.
    QString message() const { return m_message; }

    // @brief True when an SVG file should be opened at startup.
    bool hasFile() const { return m_status == Status::OpenFile; }

    // @brief True when an SVG document from standard input should be opened.
    bool hasDocument() const { return m_status == Status::OpenDocument; }

    // @brief True when the command line could not be honoured.
    bool hasError() const { return m_status == Status::Failed; }

private:
    // @brief Read and validate the SVG document offered on standard input.
    // @param documentName Value of --document-name; empty when not given.
    // @param standardInput Device read to end; nullptr is a failure, because
    //        --svg-stdin asked for a channel the caller did not supply.
    // @return OpenDocument with svgDocument() set, or Failed with a message.
    static StartupOptions parseStandardInput(const QString &documentName,
                                             QIODevice *standardInput);

    // Result of the parse; NoFile is the "nothing was asked of us" default.
    Status m_status = Status::NoFile;

    // Absolute path of the file to open; set only for Status::OpenFile.
    QString m_filePath;

    // Stringified SVG document; set only for Status::OpenDocument.
    QString m_svgDocument;

    // Pattern base name for default export names; set only for OpenDocument.
    QString m_documentName;

    // User-visible text; set only for Status::ShowInformation and Status::Failed.
    QString m_message;
}; // class StartupOptions
