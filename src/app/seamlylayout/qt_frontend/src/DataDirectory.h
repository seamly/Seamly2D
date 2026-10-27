// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file DataDirectory.h
// @brief SEAMLY_DATA_DIR support: one folder that holds the settings and logs of a job.

#pragma once

#include <QString>

/// @brief Reads SEAMLY_DATA_DIR and gives the settings and log folders under it.
///
/// Matches Seamly2D and SeamlyMe: settings in <dir>/settings, logs in <dir>/logs/seamlylayout.
/// Call configure() before QApplication. The variable is unset or empty for a normal run.
class DataDirectory
{
public:
    /// Exit codes. They equal V_EX_CONFIG and V_EX_CANTCREAT in vsysexits.h.
    static constexpr int exitOk = 0;
    static constexpr int exitConfig = 78;
    static constexpr int exitCannotCreate = 73;

    /// @brief Read SEAMLY_DATA_DIR from the environment and apply it.
    /// @return exitOk, exitConfig for a relative path, or exitCannotCreate for an unwritable folder.
    static int configure();

    /// @brief Apply a requested directory. An empty string clears the setting.
    /// @param requested Value of SEAMLY_DATA_DIR.
    /// @return Same codes as configure(). A message goes to stderr on failure.
    static int configureFrom(const QString &requested);

    /// @brief Absolute data directory, or an empty string when SEAMLY_DATA_DIR is not set.
    static QString path();

    /// @brief Folder that replaces the app config root: <dir>/settings. Empty when not set.
    static QString settingsRoot();

    /// @brief Log folder: <dir>/logs/seamlylayout. Empty when not set.
    static QString logsDirectory();
};
