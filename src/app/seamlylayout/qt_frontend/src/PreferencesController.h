// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file PreferencesController.h
// @brief QObject bridge between QML and the QtWidgets PreferencesWindow.
//
// Registered with the QML engine as "PreferencesController" (SeamlyLayout 1.0).
// QML calls openPreferences() to show (or raise) the preferences dialog.
// Signals from PreferencesWindow are forwarded as QML-visible signals.

#pragma once

#include <QObject>

// Full include required — Qt 6.10 moc needs the complete type definition
// for Q_PROPERTY pointer types (PreferencesModel*).
#include "PreferencesModel.h"

// Forward declaration — avoids pulling widget headers into QML-visible code.
class PreferencesWindow;

/// @class PreferencesController
/// @brief QML-accessible controller that owns and manages PreferencesWindow.
///
/// Lifetime: one PreferencesWindow is created on the first call to
/// openPreferences() and reused on subsequent calls.  The pointer is cleared
/// when the dialog is destroyed (e.g. via the OS close button).
///
/// @par QML usage:
/// @code
///   PreferencesController {
///       id: preferencesController
///       preferencesModel: preferencesModel
///       onSaved: { /* preferences persisted */ }
///       onDiscarded: { /* preferences reloaded */ }
///   }
///   preferencesController.openPreferences()
/// @endcode
class PreferencesController : public QObject
{
    Q_OBJECT

    // @brief The PreferencesModel instance to pass to the window.
    Q_PROPERTY(PreferencesModel* preferencesModel
               READ preferencesModel
               WRITE setPreferencesModel
               NOTIFY preferencesModelChanged)

public:
    /// @brief Construct the controller (no window created yet).
    /// @param parent Optional QObject parent.
    explicit PreferencesController(QObject *parent = nullptr);

    /// @brief Open or raise the PreferencesWindow.
    Q_INVOKABLE void openPreferences();

    /// @brief Close the preferences window on demand.
    Q_INVOKABLE void closePreferences();

    // Property accessors
    PreferencesModel *preferencesModel() const { return m_model; }
    void setPreferencesModel(PreferencesModel *model);

signals:
    /// @brief Forwarded from PreferencesWindow::saved().
    void saved();

    /// @brief Forwarded from PreferencesWindow::defaultsReset().
    void defaultsReset();

    /// @brief Forwarded from PreferencesWindow::discarded().
    void discarded();

    /// @brief Emitted when the preferencesModel property changes.
    void preferencesModelChanged();

private:
    /// @brief The managed preferences window (nullptr until first open).
    PreferencesWindow *m_window = nullptr;

    /// @brief The PreferencesModel to bind (set from QML).
    PreferencesModel *m_model = nullptr;
}; // PreferencesController
