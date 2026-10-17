/******************************************************************************
 **  @file   windows_foreground_window.h
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

#ifndef WINDOWS_FOREGROUND_WINDOW_H
#define WINDOWS_FOREGROUND_WINDOW_H

#include <QtGlobal>

/**
 * @brief Windows lets only the foreground process bring a window to the front.
 *
 * When another process tries, its window stays behind and only its taskbar
 * button flashes. Call these functions while Seamly2D is the foreground
 * process, that is, from a user action.
 */
namespace WindowsForegroundWindow
{
/**
 * @brief allowForeground gives a started process the right to bring its window to the front.
 * @param processId the child process, from QProcess::processId().
 */
void allowForeground(qint64 processId);

/**
 * @brief activateProcessWindow brings the front-most visible top-level window of a process to the front.
 * @param processId the process that owns the window.
 * @return false when the process has no visible top-level window or Windows refused.
 */
bool activateProcessWindow(qint64 processId);
} // namespace WindowsForegroundWindow

#endif // WINDOWS_FOREGROUND_WINDOW_H
