// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file PreferencesWindow.h
// @brief QtWidgets dialog for application preferences — directory and viewer paths.
//
// Mirrors the QML PreferencesPanel.qml layout and Seamly violet branding.
// Uses SeamlyTheme::makeSeamlyPalette() + Fusion style for visual consistency.
//
// Two sections:
//   1. Directories — Input SVG Directory, Layout Output Directory,
//      Settings Directory, Default Settings File
//   2. Viewer Applications — DXF Viewer, PDF Viewer, PNG Viewer executable paths
//
// Each field: read-only QLineEdit + Browse button opening a native file/folder dialog.
// Save persists via PreferencesModel::save(); Discard reloads via PreferencesModel::load().

#pragma once

#include <QDialog>

class QLineEdit;
class PreferencesModel;

/// @class PreferencesWindow
/// @brief QtWidgets preferences dialog with Seamly violet branding.
///
/// Launched by PreferencesController from QML.  Reads/writes PreferencesModel
/// properties directly and calls save()/load() on accept/discard.
class PreferencesWindow : public QDialog
{
    Q_OBJECT

public:
    /// @brief Construct the preferences dialog.
    /// @param model PreferencesModel to read/write (not owned; must outlive the dialog).
    /// @param parent Optional parent widget.
    explicit PreferencesWindow(PreferencesModel *model, QWidget *parent = nullptr);

    /// @brief Reload all fields from the model (e.g. after external changes).
    void reloadFromModel();

signals:
    /// @brief Emitted after Save is clicked and preferences are persisted.
    void saved();

    /// @brief Emitted after Reset to Defaults applies defaults and persists them.
    void defaultsReset();

    /// @brief Emitted after Discard is clicked and preferences are reloaded.
    void discarded();

private:
    /// @brief Populate field widgets from the current model state.
    void populateFields();

    /// @brief Browse for a directory and set the result into model + field.
    /// @param title Dialog title string.
    /// @param field QLineEdit to update.
    /// @param setter Member function pointer on PreferencesModel.
    void browseFolder(const QString &title, QLineEdit *field,
                      void (PreferencesModel::*setter)(const QString &));

    /// @brief Browse for a file and set the result into model + field.
    /// @param title Dialog title string.
    /// @param filter Name filter string (e.g. "JSON Files (*.json);;All Files (*)").
    /// @param field QLineEdit to update.
    /// @param setter Member function pointer on PreferencesModel.
    void browseFile(const QString &title, const QString &filter,
                    QLineEdit *field,
                    void (PreferencesModel::*setter)(const QString &));

    // Model — not owned
    PreferencesModel *m_model = nullptr;

    // Field widgets
    QLineEdit *m_inputDirField    = nullptr;
    QLineEdit *m_layoutDirField   = nullptr;
    QLineEdit *m_settingsDirField = nullptr;
    QLineEdit *m_settingsFileField = nullptr;
    QLineEdit *m_preferencesFileField = nullptr;
    QLineEdit *m_dxfViewerField   = nullptr;
    QLineEdit *m_pdfViewerField   = nullptr;
    QLineEdit *m_pngViewerField   = nullptr;
    QLineEdit *m_projectorField   = nullptr;

}; // PreferencesWindow
