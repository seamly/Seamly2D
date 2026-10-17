/******************************************************************************
 **  @file   windows_foreground_window.cpp
 **  @author slspencer
 **
 **  @brief
 **  Windows foreground rules for the windows of a child process
 **
 **  @copyright
 **  Copyright (C) 2026 Seamly2D Project
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
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **
 *****************************************************************************/

#include "windows_foreground_window.h"

// windows.h stays in this file: its macros clash with names in the rest of Seamly2D.
#include <qt_windows.h>

namespace
{
/**
 * @brief ProcessWindowSearch carries the process to find and the window found to the EnumWindows callback.
 */
struct ProcessWindowSearch
{
    DWORD processId;
    HWND  window;
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief findProcessWindow stops EnumWindows at the first visible top-level window without an owner.
 *
 * EnumWindows visits windows from front to back, so the first match is the window the user saw last.
 * Qt's hidden helper windows are not visible; dialogs have an owner.
 */
BOOL CALLBACK findProcessWindow(HWND window, LPARAM parameter)
{
    auto *search = reinterpret_cast<ProcessWindowSearch *>(parameter);

    DWORD windowProcessId = 0;
    GetWindowThreadProcessId(window, &windowProcessId);

    const bool found = windowProcessId == search->processId && IsWindowVisible(window) != FALSE
                       && GetWindow(window, GW_OWNER) == nullptr;
    if (found)
    {
        search->window = window;
    }
    return found ? FALSE : TRUE;
}
} // namespace

namespace WindowsForegroundWindow
{
//---------------------------------------------------------------------------------------------------------------------
void allowForeground(qint64 processId)
{
    AllowSetForegroundWindow(static_cast<DWORD>(processId));
}

//---------------------------------------------------------------------------------------------------------------------
bool activateProcessWindow(qint64 processId)
{
    ProcessWindowSearch search {static_cast<DWORD>(processId), nullptr};
    EnumWindows(findProcessWindow, reinterpret_cast<LPARAM>(&search));

    bool activated = false;
    if (search.window != nullptr)
    {
        // A minimized window stays on the taskbar after SetForegroundWindow; restore it first.
        if (IsIconic(search.window) != FALSE)
        {
            ShowWindow(search.window, SW_RESTORE);
        }
        activated = SetForegroundWindow(search.window) != FALSE;
    }
    return activated;
}
} // namespace WindowsForegroundWindow
