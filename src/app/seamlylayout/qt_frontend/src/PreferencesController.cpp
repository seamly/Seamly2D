// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file PreferencesController.cpp
// @brief Implementation of PreferencesController — QML-to-QtWidgets bridge.

#include "PreferencesController.h"
#include "PreferencesModel.h"
#include "PreferencesWindow.h"
#include "Logger.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

/// @brief Construct the controller; no window is created until openPreferences().
PreferencesController::PreferencesController(QObject *parent)
    : QObject(parent)
{
} // PreferencesController

// ---------------------------------------------------------------------------
// Property setter
// ---------------------------------------------------------------------------

/// @brief Set the PreferencesModel; emits preferencesModelChanged if changed.
void PreferencesController::setPreferencesModel(PreferencesModel *model)
{
    if (m_model == model) return;
    m_model = model;
    emit preferencesModelChanged();
} // setPreferencesModel

// ---------------------------------------------------------------------------
// Q_INVOKABLE methods
// ---------------------------------------------------------------------------

/// @brief Open or raise the preferences dialog.
void PreferencesController::openPreferences()
{
    if (!m_model) {
        Logger::log(QStringLiteral("PreferencesController::openPreferences(): no model set"));
        return;
    } // if no model

    if (!m_window) {
        // First open — create the window and wire signals.
        Logger::log(QStringLiteral("PreferencesController::openPreferences(): creating PreferencesWindow"));

        m_window = new PreferencesWindow(m_model);

        // Forward PreferencesWindow::saved() as saved().
        connect(m_window, &PreferencesWindow::saved,
                this,     &PreferencesController::saved);

        // Forward PreferencesWindow::defaultsReset() as defaultsReset().
        connect(m_window, &PreferencesWindow::defaultsReset,
            this,     &PreferencesController::defaultsReset);

        // Forward PreferencesWindow::discarded() as discarded().
        connect(m_window, &PreferencesWindow::discarded,
                this,     &PreferencesController::discarded);

        // Clear our pointer when the dialog is destroyed,
        // so the next openPreferences() call creates a fresh dialog.
        connect(m_window, &QObject::destroyed,
                this, [this]() {
                    m_window = nullptr;
                    Logger::log(QStringLiteral("PreferencesController: PreferencesWindow destroyed"));
                }); // destroyed lambda
    } else {
        // Subsequent call — reload fields from the model in case they changed.
        Logger::log(QStringLiteral("PreferencesController::openPreferences(): raising PreferencesWindow"));
        m_window->reloadFromModel();
    } // if no window yet

    m_window->show();
    m_window->raise();
    m_window->activateWindow();
} // openPreferences

/// @brief Close the preferences window on demand.
void PreferencesController::closePreferences()
{
    if (m_window) {
        m_window->close();
    } // if window exists
} // closePreferences
