// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file LoggerPaths.h
// @brief Shared runtime resolution of the Logger output directory.

#pragma once

#include "Platform.h"

#include <QCoreApplication>
#include <QStandardPaths>

namespace LoggerPaths
{
inline QString appConfigOrApplicationDirectory()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (dir.isEmpty()) {
        dir = QCoreApplication::applicationDirPath();
    }
    return dir;
}

inline QString resolveLogsDirectoryForCurrentPlatform()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    return appConfigOrApplicationDirectory() + QStringLiteral("/logs");
#else
    if (Platform::isAppImage() || Platform::isFlatpak()) {
        return appConfigOrApplicationDirectory() + QStringLiteral("/logs");
    }
    return QCoreApplication::applicationDirPath() + QStringLiteral("/logs");
#endif
}
}
