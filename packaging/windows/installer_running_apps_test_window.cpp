/******************************************************************************
 **  @file   installer_running_apps_test_window.cpp
 **  @author slspencer
 **
 **  @brief
 **  Stand-in for a running Seamly app in installer_running_apps_test.ps1.
 **
 **  The test copies this program to a Seamly executable name. It shows one
 **  top-level window and exits when that window gets WM_CLOSE, as a Seamly
 **  app without unsaved work does.
 **
 **  @copyright
 **  This source code is part of the Seamly project, a suite of apparel CAD
 **  software.
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
 **
 **  @license
 **  GPL-3.0-or-later
 *****************************************************************************/

#include <windows.h>

namespace
{
LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"SeamlyRunningAppsTestWindow";
    RegisterClassW(&windowClass);

    // Visible but off screen, so a build run does not flash a window at the user.
    HWND window = CreateWindowExW(WS_EX_TOOLWINDOW, windowClass.lpszClassName, L"Seamly running-apps test",
                                  WS_POPUP, -32000, -32000, 1, 1, nullptr, nullptr, instance, nullptr);
    ShowWindow(window, SW_SHOWNOACTIVATE);

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return 0;
}
