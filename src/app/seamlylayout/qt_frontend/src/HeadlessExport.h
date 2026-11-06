// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file HeadlessExport.h
// @brief Runs the existing layout and export pipeline without loading QML.

#pragma once

#include <QString>

class StartupOptions;

// @brief Runs a parsed command-line export request through AppController.
class HeadlessExport
{
public:
    // @brief Import, lay out, and export one SVG without creating a QML window.
    // @param options Validated command-line export request.
    // @param errorMessage Receives the actionable failure message, if any.
    // @return 0 on success; a sysexits-style non-zero code on failure.
    static int run(const StartupOptions &options, QString &errorMessage);
}; // class HeadlessExport
