/******************************************************************************
 **  @file   legacy_data_migration.cpp
 **  @author slspencer
 **  @date   August 24, 2026
 **
 **  @brief
 **  Runs the first-run move out of ~/seamly2d, and tells the user it happened.
 **
 **  @copyright
 **  This source code is part of the Seamly2D project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
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
 *****************************************************************************/

#include "legacy_data_migration.h"

#include "legacy_data_archive.h"
#include "vcommonsettings.h"

#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QRect>
#include <QSplashScreen>
#include <Qt>

namespace
{
    // Long enough to read two paths, short enough not to feel like a stall.
    const int finalMessageHoldMs = 6000;

    //-----------------------------------------------------------------------------------------------------------------
    /**
     * @brief canShowSplash reports whether this process has a screen to put a window on.
     *
     * openSettings() runs from the application constructor, before VCommandLine has parsed
     * anything, so Application2D::isGUIMode() is not yet answerable — it returns false for
     * every run at this point, GUI or not. The platform plugin is the one thing that is
     * already settled, and it is what actually decides whether a window can appear: the test
     * suites and the CI jobs run with QT_QPA_PLATFORM=offscreen.
     *
     * A console export therefore does get the splash. That is accepted: it is not modal, it
     * closes itself, and it only ever appears on the single run that migrates the tree.
     */
    bool canShowSplash()
    {
        if (qobject_cast<QApplication *>(QCoreApplication::instance()) == nullptr)
        {
            return false;
        }

        const QString platform = QGuiApplication::platformName();
        return !platform.isEmpty()
               && platform != QLatin1String("offscreen")
               && platform != QLatin1String("minimal");
    }

    //-----------------------------------------------------------------------------------------------------------------
    /**
     * @brief MigrationSplash is the "please wait" window, or nothing at all.
     *
     * Constructing it on a headless run gives an object whose every method does nothing, so
     * the caller needs no branches of its own.
     *
     * The colours are fixed rather than taken from the palette. The splash is created before
     * Application2D::setTheme() runs, so there is no application palette to read yet, and a
     * light panel with dark text is legible under either theme.
     */
    class MigrationSplash
    {
    public:
                 MigrationSplash();
                ~MigrationSplash();

        void     show(const QString &message);
        void     hold(int milliseconds);

    private:
        Q_DISABLE_COPY_MOVE(MigrationSplash)

        QSplashScreen *m_splash;
    };

    //-----------------------------------------------------------------------------------------------------------------
    MigrationSplash::MigrationSplash()
        : m_splash(nullptr)
    {
        if (!canShowSplash())
        {
            return;
        }

        const QSize logicalSize(560, 220);
        const qreal ratio = qApp->devicePixelRatio();

        QPixmap panel(logicalSize * ratio);
        panel.setDevicePixelRatio(ratio);
        panel.fill(QColor(0xFA, 0xFA, 0xFA));

        QPainter painter(&panel);
        painter.setPen(QPen(QColor(0x60, 0x60, 0x60), 1));
        painter.drawRect(QRect(QPoint(0, 0), logicalSize - QSize(1, 1)));
        painter.end();

        m_splash = new QSplashScreen(panel);
        m_splash->setFont(QFont(m_splash->font().family(), 10));
    }

    //-----------------------------------------------------------------------------------------------------------------
    MigrationSplash::~MigrationSplash()
    {
        if (m_splash != nullptr)
        {
            m_splash->close();
            delete m_splash;
        }
    }

    //-----------------------------------------------------------------------------------------------------------------
    /**
     * @brief show puts a message on the splash and paints it now.
     *
     * The event loop has not started yet — this all runs from the application constructor —
     * so without the explicit processEvents() the window would be created and never drawn.
     */
    void MigrationSplash::show(const QString &message)
    {
        if (m_splash == nullptr)
        {
            return;
        }

        m_splash->show();
        m_splash->showMessage(message, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                              QColor(0x20, 0x20, 0x20));
        QCoreApplication::processEvents();
    }

    //-----------------------------------------------------------------------------------------------------------------
    /**
     * @brief hold keeps the last message on screen long enough to read.
     *
     * processEvents() in a loop rather than a nested QEventLoop: this runs during application
     * construction, and a nested event loop there can re-enter code that is not built yet.
     */
    void MigrationSplash::hold(int milliseconds)
    {
        if (m_splash == nullptr)
        {
            return;
        }

        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < milliseconds)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
    }
}   // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief run copies the legacy tree out, then archives what it left behind.
 *
 * Called only when initializeDataRoot() has just adopted a legacy tree, so it is the first
 * run of the first app after an upgrade. Every step is fail-safe: a failure at any point
 * leaves the user's files where the previous step put them, and the worst case is that the
 * app carries on from ~/seamly2d exactly as the old build did.
 *
 * The legacy tree is never deleted here. migrateAdoptedLegacyTree() already leaves it in
 * place with a MIGRATED-TO-SEAMLY.txt marker; the .zip this function adds is a second,
 * portable backup beside it, not a replacement for it.
 *
 * @param legacyRoot the adopted tree, normally ~/seamly2d.
 * @param newRoot where it should live now, normally <Documents>/Seamly.
 * @return the data root actually in force afterwards.
 */
QString LegacyDataMigration::run(const QString &legacyRoot, const QString &newRoot)
{
    if (legacyRoot.isEmpty() || newRoot.isEmpty() || !QFileInfo(legacyRoot).isDir())
    {
        return legacyRoot;
    }

    MigrationSplash splash;
    splash.show(QStringLiteral("Moving your work to\n\n    %1\n\nPlease wait. A large collection of "
                               "patterns can take a few minutes.")
                    .arg(QDir::toNativeSeparators(newRoot)));

    const QString root = VCommonSettings::migrateAdoptedLegacyTree(legacyRoot, newRoot);
    if (root != newRoot)
    {
        // migrateAdoptedLegacyTree() has already said why. Nothing was moved, so there is
        // nothing to back up yet.
        return root;
    }

    splash.show(QStringLiteral("Backing up your old folder\n\n    %1\n\nChecking every file as it is "
                               "packed into a backup.")
                    .arg(QDir::toNativeSeparators(legacyRoot)));

    QString errorMessage;
    const QString archive = LegacyDataArchive::archive(legacyRoot, root, &errorMessage);

    if (archive.isEmpty())
    {
        qWarning() << "Could not back up the old data folder" << QDir::toNativeSeparators(legacyRoot)
                   << ':' << errorMessage;
        splash.show(QStringLiteral("Your work is now in\n\n    %1\n\nYour old folder could not be backed "
                                   "up as a .zip, but nothing was changed there. It is still at\n\n    %2")
                        .arg(QDir::toNativeSeparators(root), QDir::toNativeSeparators(legacyRoot)));
    }
    else
    {
        qInfo() << "Backed up the old data folder to" << QDir::toNativeSeparators(archive) << "- it remains at"
                << QDir::toNativeSeparators(legacyRoot);
        splash.show(QStringLiteral("Your work is now in\n\n    %1\n\nYour old folder is kept at\n\n    %2\n\n"
                                   "and backed up in\n\n    %3")
                        .arg(QDir::toNativeSeparators(root), QDir::toNativeSeparators(legacyRoot),
                             QDir::toNativeSeparators(archive)));
    }

    splash.hold(finalMessageHoldMs);
    return root;
}
