// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file HeadlessExport.h
// @brief Runs the existing layout and export pipeline without loading QML.

#pragma once

#include <QString>
#include <QStringList>

class StartupOptions;

// @brief Runs a parsed command-line export or --info request through AppController.
class HeadlessExport
{
public:
    // @brief Outcome of one headless run.
    struct Result
    {
        // 0 on success; a sysexits-style code on failure (64 usage, 65 data, 66 input, 69 unavailable,
        // 70 incomplete layout, 73 output).
        int exitCode = 0;
        // Actionable failure text; empty on success.
        QString error;
        // Layout notes that do not stop the export: longer media, rotation, fold lines.
        QStringList warnings;
        // --info JSON report; empty for an export.
        QString report;
    }; // struct Result

    // @brief Import, lay out, and export one SVG, or report on it, without creating a QML window.
    // @param options Validated command-line request (hasHeadlessExport() is true).
    // @return Exit code, error, warnings and report of the run.
    static Result run(const StartupOptions &options);
}; // class HeadlessExport
