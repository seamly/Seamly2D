// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file Platform.h
// @brief Detect the host operating system once at startup.
//
// Usage:
//   Platform::init();                         // call once from main()
//   if (Platform::os == Platform::Windows)    // use throughout the application
//   QString p = Platform::toNativePath(path); // convert path separators for OS

#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// @brief Static-only platform detection — detected once, used everywhere.
class Platform
{
public:

    // @brief Supported operating systems.
    enum OS { Windows, macOS, Linux };

    // @brief The detected operating system. Set by init(), read everywhere.
    static OS os;

    // @brief Detect the host OS. Call once from main() at startup.
    static void init()
    {
#if defined(Q_OS_WIN)
        os = Windows;
#elif defined(Q_OS_MACOS)
        os = macOS;
#else
        os = Linux;
#endif
    } // init

    // @brief Convert a file path to the native OS separator format.
    // Windows: forward slashes → backslashes.
    // macOS/Linux: backslashes → forward slashes.
    static QString toNativePath(const QString &path)
    {
        return QDir::toNativeSeparators(path);
    } // toNativePath

    // @brief Check if an executable is a Windows Store (UWP/MSIX) app.
    // Windows Store apps are registered as App Execution Aliases — NTFS reparse
    // points that QProcess::startDetached cannot launch directly. Detecting them
    // lets us skip the slow QProcess failure and go straight to QDesktopServices.
    // On non-Windows platforms, always returns false.
    static bool isStoreApp(const QString &exePath)
    {
#ifdef Q_OS_WIN
        // App Execution Aliases are NTFS reparse points; real .exe files are not.
        QString native = toNativePath(exePath);
        DWORD attrs = GetFileAttributesW(
            reinterpret_cast<const wchar_t *>(native.utf16()));
        if (attrs != INVALID_FILE_ATTRIBUTES
            && (attrs & FILE_ATTRIBUTE_REPARSE_POINT)) {
            return true; // if reparse point — UWP/Store app alias
        } // if reparse point
        return false; // traditional desktop executable
#else
        Q_UNUSED(exePath);
        return false; // non-Windows — no Store apps
#endif
    } // isStoreApp

    // @brief Detect whether this process is running from within a mounted AppImage.
    //
    // Task 17: an AppImage mounts its payload read-only (a FUSE-mounted squashfs), so any
    // exe-relative writable path (settings, input/output folders, debug logs) that works for
    // a normal Linux install fails silently inside one — the same problem Task 16 found for
    // a signed, notarized macOS .app bundle. The AppImage runtime sets the APPIMAGE
    // environment variable (absolute path to the .AppImage file) in every process it execs
    // (see https://docs.appimage.org/packaging-guide/environment-variables.html), so checking
    // for it is enough to tell the two cases apart.
    //
    // The check itself is a plain environment-variable read with no OS-specific API, so it is
    // safe to call on every platform — it is simply always false on Windows/macOS, where the
    // variable is never set. This also lets unit tests exercise the AppImage fallback path on
    // any host by setting the variable directly, unlike the compile-time Q_OS_MACOS branches.
    static bool isAppImage()
    {
        return qEnvironmentVariableIsSet("APPIMAGE");
    } // isAppImage

private:
    // Non-instantiable — all members are static.
    Platform() = delete;

}; // class Platform
