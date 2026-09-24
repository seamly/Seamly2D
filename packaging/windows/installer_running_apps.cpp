/******************************************************************************
 **  @file   installer_running_apps.cpp
 **  @author slspencer
 **
 **  @brief
 **  MSI custom actions that find and close running Seamly apps before the
 **  installer wizard starts.
 **
 **  Windows Installer finds running apps itself during InstallValidate, but
 **  only after Restart Manager checks every packaged file against every
 **  process. With the Qt runtime that takes long enough to look like a hang.
 **  A process-name lookup answers the same question at once.
 **
 **  @copyright
 **  This source code is part of the Seamly project, a suite of apparel CAD
 **  software.
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
 **
 **  @license
 **  Seamly2D/SeamlyMe is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Seamly2D/SeamlyMe is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Seamly2D/SeamlyMe.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <tlhelp32.h>

#include <vector>

namespace
{
// Set when at least one Seamly app runs. The dialog is shown on it.
constexpr wchar_t anyAppRunningProperty[] = L"SEAMLYRUNNINGAPPS";

// Time the close action waits for the apps to exit. An app that asks the
// user to save its work stays running, and the dialog then lists it again.
constexpr DWORD closeWaitMilliseconds = 5000;

struct SeamlyApp
{
    const wchar_t *executableName;
    const wchar_t *runningProperty;
};

// The dialog shows one line for each app, conditioned on its property.
constexpr SeamlyApp seamlyApps[] = {
    {L"seamly2d.exe", L"SEAMLYRUNNING2D"},
    {L"seamlyme.exe", L"SEAMLYRUNNINGME"},
    {L"seamlylayout.exe", L"SEAMLYRUNNINGLAYOUT"},
};

struct RunningApp
{
    DWORD processId;
    size_t appIndex;
};

/**
 * @brief Lists the Seamly app processes in the installer's own Windows session.
 *
 * An app in another user's session can not be closed from here. Restart
 * Manager still finds it during InstallValidate.
 */
std::vector<RunningApp> findRunningApps()
{
    std::vector<RunningApp> runningApps;

    DWORD installerSession = 0;
    if (!ProcessIdToSessionId(GetCurrentProcessId(), &installerSession))
    {
        return runningApps;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return runningApps;
    }

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    for (BOOL found = Process32FirstW(snapshot, &entry); found; found = Process32NextW(snapshot, &entry))
    {
        DWORD processSession = 0;
        if (!ProcessIdToSessionId(entry.th32ProcessID, &processSession) || processSession != installerSession)
        {
            continue;
        }
        for (size_t index = 0; index < ARRAYSIZE(seamlyApps); ++index)
        {
            if (lstrcmpiW(entry.szExeFile, seamlyApps[index].executableName) == 0)
            {
                runningApps.push_back({entry.th32ProcessID, index});
            }
        }
    }

    CloseHandle(snapshot);
    return runningApps;
}

/**
 * @brief Sets one property for each running app and one for any app.
 *
 * An empty value deletes the property, so a condition on it reads false.
 */
void publishRunningApps(MSIHANDLE install, const std::vector<RunningApp> &runningApps)
{
    bool running[ARRAYSIZE(seamlyApps)] = {};
    for (const RunningApp &app : runningApps)
    {
        running[app.appIndex] = true;
    }

    for (size_t index = 0; index < ARRAYSIZE(seamlyApps); ++index)
    {
        MsiSetPropertyW(install, seamlyApps[index].runningProperty, running[index] ? L"1" : L"");
    }
    MsiSetPropertyW(install, anyAppRunningProperty, runningApps.empty() ? L"" : L"1");
}

/**
 * @brief Posts WM_CLOSE to each visible top-level window of a running app.
 *
 * WM_CLOSE is the same request as the window's close button, so an app with
 * unsaved work asks the user to save it.
 */
BOOL CALLBACK closeAppWindow(HWND window, LPARAM parameter)
{
    const auto *runningApps = reinterpret_cast<const std::vector<RunningApp> *>(parameter);

    DWORD windowProcessId = 0;
    GetWindowThreadProcessId(window, &windowProcessId);
    if (!IsWindowVisible(window) || GetWindow(window, GW_OWNER) != nullptr)
    {
        return TRUE;
    }

    for (const RunningApp &app : *runningApps)
    {
        if (app.processId == windowProcessId)
        {
            PostMessageW(window, WM_CLOSE, 0, 0);
            break;
        }
    }
    return TRUE;
}

/**
 * @brief Waits until every listed process exits, or until the shared timeout ends.
 */
void waitForExit(const std::vector<RunningApp> &runningApps)
{
    const ULONGLONG deadline = GetTickCount64() + closeWaitMilliseconds;
    for (const RunningApp &app : runningApps)
    {
        HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, app.processId);
        if (process == nullptr)
        {
            continue;
        }
        const ULONGLONG now = GetTickCount64();
        WaitForSingleObject(process, now < deadline ? static_cast<DWORD>(deadline - now) : 0);
        CloseHandle(process);
    }
}
} // namespace

/**
 * @brief Sets SEAMLYRUNNINGAPPS and the per-app properties from the running processes.
 * @return ERROR_SUCCESS always: a failed lookup must not stop the install,
 *         because Restart Manager still checks during InstallValidate.
 */
extern "C" __declspec(dllexport) UINT __stdcall SeamlyFindRunningApps(MSIHANDLE install)
{
    publishRunningApps(install, findRunningApps());
    return ERROR_SUCCESS;
}

/**
 * @brief Asks each running Seamly app to close, then refreshes the running-app properties.
 * @return ERROR_SUCCESS always, for the same reason as SeamlyFindRunningApps.
 */
extern "C" __declspec(dllexport) UINT __stdcall SeamlyCloseRunningApps(MSIHANDLE install)
{
    const std::vector<RunningApp> runningApps = findRunningApps();
    if (!runningApps.empty())
    {
        EnumWindows(closeAppWindow, reinterpret_cast<LPARAM>(&runningApps));
        waitForExit(runningApps);
    }
    publishRunningApps(install, findRunningApps());
    return ERROR_SUCCESS;
}
