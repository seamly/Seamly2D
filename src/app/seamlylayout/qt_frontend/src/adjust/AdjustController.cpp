// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file AdjustController.cpp
// @brief Implementation of AdjustController — QML-to-QtWidgets bridge.

#include "AdjustController.h"
#include "AdjustWindow.h"
#include "Logger.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

/// @brief Construct the controller; no window is created until launchAdjustWindow().
AdjustController::AdjustController(QObject* parent)
    : QObject(parent)
    , m_window(nullptr)
{
}

// ---------------------------------------------------------------------------
// Q_INVOKABLE methods
// ---------------------------------------------------------------------------

/// @brief Close the AdjustWindow on demand (called from QML after canvas reload).
void AdjustController::closeAdjustWindow()
{
    if (m_window) {
        m_window->closeWindow();
    } // if window exists
} // closeAdjustWindow

/// @brief Open or reload the AdjustWindow with the provided output/adjust_dom.svg file.
//    - On first call, creates the window, connects its signals, and shows it.
//    - On subsequent calls, reloads the scene with the new SVG and bbox JSON
//      so the window reflects the latest layout state.

void AdjustController::launchAdjustWindow(const QString& svgPath,
                                          const QString& bboxJson)
{
    if (!m_window) {
        // First launch — create the window and wire signals.
        Logger::log(QStringLiteral("===========ENTER ADJUST MODE=========="));
        Logger::log(QStringLiteral("AdjustController::launchAdjustWindow(): creating AdjustWindow"));

        m_window = new AdjustWindow(svgPath, bboxJson);

        // When user applies/enters, Forward AdjustWindow::accepted() as applyRequested().
        connect(m_window, &AdjustWindow::accepted,
                this,     &AdjustController::applyRequested);

        // When user saves, Forward AdjustWindow::saveRequested() as saveRequested().
        connect(m_window, &AdjustWindow::saveRequested,
                this,     &AdjustController::saveRequested);

        // Forward AdjustWindow::cancelled() as cancelRequested().
        connect(m_window, &AdjustWindow::cancelled,
                this,     &AdjustController::cancelRequested);

        // Forward title-bar X closes as immediate abandon/discard.
        connect(m_window, &AdjustWindow::abandoned,
                this,     &AdjustController::abandonRequested);

        // Clear our pointer when the user closes the window via the OS button,
        // so the next launchAdjustWindow() call creates a fresh window.
        connect(m_window, &QObject::destroyed,
                this, [this]() {
                    m_window = nullptr;
                    Logger::log(QStringLiteral("AdjustController: AdjustWindow destroyed"));
                }); // destroyed lambda
    } else {
        // Subsequent call — reload scene without destroying the window.
        Logger::log(QStringLiteral("AdjustController::launchAdjustWindow(): reloading AdjustWindow"));
        // Note: AdjustWindow::reload() clears all existing items and creates new ones, so the window reflects the latest layout state.
        m_window->reload(svgPath, bboxJson);
    } // if no window yet

    m_window->show();
    m_window->raise();
    m_window->activateWindow();
} // launchAdjustWindow
