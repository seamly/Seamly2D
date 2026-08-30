//  @file   preferencespathpage.cpp
//  @author slspencer
//  @date   30 Aug, 2026
//
//  @brief
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program to create and model patterns of clothing.
//  Copyright (C) 2026 Seamly2D Project
//  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
//
//  Seamly2D is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Seamly2D is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.

/************************************************************************
 **
 **  @file   preferencespathpage.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://bitbucket.org/dismine/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "preferencespathpage.h"
#include "ui_preferencespathpage.h"
#include "../vmisc/vsettings.h"
#include "../../options.h"
#include "../../core/application_2d.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>

//---------------------------------------------------------------------------------------------------------------------
PreferencesPathPage::PreferencesPathPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PreferencesPathPage)
{
    ui->setupUi(this);

    initializeTable();

    connect(ui->defaultButton, &QPushButton::clicked, this, &PreferencesPathPage::defaultPath);
    connect(ui->editButton, &QPushButton::clicked, this, &PreferencesPathPage::editPath);
}

//---------------------------------------------------------------------------------------------------------------------
PreferencesPathPage::~PreferencesPathPage()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesPathPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Apply stores every path shown in the table back into the application settings.
 *
 * The data root (row 0) is applied first, and every subfolder still living inside the
 * previous root follows it to the new location, so repointing the root at another drive or
 * a cloud-synced folder relocates the whole tree in one edit (Task 34).
 */
void PreferencesPathPage::Apply()
{
    VSettings *settings = qApp->Seamly2DSettings();

    const QString previousRoot = settings->getDataRoot();
    const QString dataRoot     = ui->pathTable->item(0, 1)->text();
    settings->setDataRoot(dataRoot);

    settings->SetPathPattern(VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(1, 1)->text(), previousRoot, dataRoot));
    settings->setTemplatePath(VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(2, 1)->text(), previousRoot, dataRoot));
    settings->setIndividualSizePath(VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(3, 1)->text(), previousRoot, dataRoot));
    settings->setMultisizePath(VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(4, 1)->text(), previousRoot, dataRoot));
    settings->SetPathLayout(VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(5, 1)->text(), previousRoot, dataRoot));
    settings->SetPathLabelTemplate(VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(6, 1)->text(), previousRoot, dataRoot));
    settings->setImageFilePath(VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(7, 1)->text(), previousRoot, dataRoot));
    settings->setBackupFilePath(VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(8, 1)->text(), previousRoot, dataRoot));
    settings->setBodyScansPath(VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(9, 1)->text(), previousRoot, dataRoot));

    // Not a data path: the SeamlyLayout executable is never rebased onto the data root.
    settings->setSeamlyLayoutAppPath(ui->pathTable->item(10, 1)->text());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief defaultPath resets the currently selected table row to its built-in default path.
 */
void PreferencesPathPage::defaultPath()
{
    const int row = ui->pathTable->currentRow();
    QTableWidgetItem *item = ui->pathTable->item(row, 1);
    SCASSERT(item != nullptr)

    QString path;

    switch (row)
    {
        case 0: // user data root
            path = VCommonSettings::getDefaultDataRoot();
            break;
        case 1: // pattern path
            path = VSettings::getDefaultPatternPath();
            break;
        case 2: // templates
            path = VCommonSettings::getDefaultTemplatePath();
            break;
        case 3: // individual measurements
            path = VCommonSettings::getDefaultIndividualSizePath();
            break;
        case 4: // multisize measurements
            path = VCommonSettings::getDefaultMultisizePath();
            break;
        case 5: // layout path
            path = VSettings::getDefaultLayoutPath();
            break;
        case 6: // label templates
            path = VSettings::getDefaultLabelTemplatePath();
            break;
        case 7: // images
            path = VSettings::getDefaultImageFilePath();
            break;
        case 8: // backups
            path = VSettings::getDefaultBackupFilePath();
            break;
        case 9: // body scans
            path = VCommonSettings::getDefaultBodyScansPath();
            break;
        case 10: // SeamlyLayout application
            // Empty means "not configured": the executable is then looked up
            // next to the Seamly2D executable (see Application2D::seamlyLayoutFilePath()).
            path = QString();
            break;
        default:
            break;
    }

    item->setText(path);
    item->setToolTip(path);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief editPath opens a picker dialog for the currently selected table row.
 *
 * Directory rows open a directory picker; the SeamlyLayout application row
 * opens a file picker because it points at an executable, not a folder.
 */
void PreferencesPathPage::editPath()
{
    const int row = ui->pathTable->currentRow();
    QTableWidgetItem *item = ui->pathTable->item(row, 1);
    SCASSERT(item != nullptr)

    QString path;
    switch (row)
    {
        case 0: // user data root
            // Any drive, volume or path is accepted here, including an external disk or a
            // cloud-synced folder such as G:/My Drive/seamly (Task 34).
            path = qApp->Seamly2DSettings()->getDataRoot();
            break;
        case 1: // pattern path
            path = qApp->Seamly2DSettings()->getPatternPath();
            break;
        case 2: // templates
            path = qApp->Seamly2DSettings()->getTemplatePath();
            break;
        case 3: // individual measurements
            path = qApp->Seamly2DSettings()->getIndividualSizePath();
            break;
        case 4: // multisize measurements
            path = qApp->Seamly2DSettings()->getMultisizePath();
            path = VCommonSettings::prepareMultisizeTables(path);
            break;
        case 5: // layout path
            path = qApp->Seamly2DSettings()->getLayoutPath();
            break;
        case 6: // label templates
            path = qApp->Seamly2DSettings()->getLabelTemplatePath();
            break;
        case 7: // images
                path = qApp->Seamly2DSettings()->getImageFilePath();
                break;
        case 8: // backups
                path = qApp->Seamly2DSettings()->getBackupFilePath();
                break;
        case 9: // body scans
                path = qApp->Seamly2DSettings()->getBodyScansPath();
                break;
        case 10: // SeamlyLayout application
        {
            // Executable file, not a directory: use a file picker and skip the
            // directory handling below.
            const QString appPath = qApp->Seamly2DSettings()->getSeamlyLayoutAppPath();
#ifdef Q_OS_WIN
            const QString filter = tr("Applications (*.exe);;All files (*.*)");
#else
            const QString filter = tr("All files (*.*)");
#endif
            const QString filename = fileDialog(this, tr("Select SeamlyLayout Application"),
                                                QFileInfo(appPath).absolutePath(), filter, nullptr,
                                                qApp->Seamly2DSettings()->getUseNativeFileDialogs(),
                                                QFileDialog::ExistingFile,
                                                QFileDialog::AcceptOpen);
            if (!filename.isEmpty())
            {
                item->setText(filename);
                item->setToolTip(filename);
            }
            return;
        }
        default:
            break;
    }

    bool usedNotExistedDir = false;
    QDir directory(path);
    if (not directory.exists())
    {
        usedNotExistedDir = directory.mkpath(".");
    }

    QString filename = fileDialog(this, tr("Open Directory"), path, QString(""), nullptr,
                                                              QFileDialog::ShowDirsOnly |
                                                              QFileDialog::DontResolveSymlinks |
                                                              qApp->Seamly2DSettings()->getUseNativeFileDialogs(),
                                                              QFileDialog::Directory, QFileDialog::AcceptOpen);

    const QString dir = QFileInfo(filename).filePath();

    if (usedNotExistedDir)
    {
        QDir directory(path);
        directory.rmpath(".");
    }

    if (dir.isEmpty())
    {
        return;
    }

    item->setText(dir);
    item->setToolTip(dir);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief initializeTable fills the paths table with the configured paths from the settings.
 */
void PreferencesPathPage::initializeTable()
{
    ui->pathTable->setRowCount(11);
    ui->pathTable->setColumnCount(2);

    const VSettings *settings = qApp->Seamly2DSettings();

    {
        // Task 34: the root every data folder below sits under. Changing it moves the
        // whole tree, since Apply() rebases the rows that live inside it.
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Seamly Data"));
        item->setIcon(QIcon("://icon/32x32/backup_files.png"));
        ui->pathTable->setItem(0, 0, item);
        item = new QTableWidgetItem(settings->getDataRoot());
        item->setToolTip(settings->getDataRoot());
        ui->pathTable->setItem(0, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Patterns"));
        item->setIcon(QIcon("://icon/32x32/seamly2d_file.png"));
        ui->pathTable->setItem(1, 0, item);
        item = new QTableWidgetItem(settings->getPatternPath());
        item->setToolTip(settings->getPatternPath());
        ui->pathTable->setItem(1, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Templates"));
        item->setIcon(QIcon("://icon/32x32/template_size_file.png"));
        ui->pathTable->setItem(2, 0, item);
        item = new QTableWidgetItem(settings->getTemplatePath());
        item->setToolTip(settings->getTemplatePath());
        ui->pathTable->setItem(2, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Individual Measurements"));
        item->setIcon(QIcon("://icon/32x32/individual_size_file.png"));
        ui->pathTable->setItem(3, 0, item);
        item = new QTableWidgetItem(settings->getIndividualSizePath());
        item->setToolTip(settings->getIndividualSizePath());
        ui->pathTable->setItem(3, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Multisize Measurements"));
        item->setIcon(QIcon("://icon/32x32/multisize_size_file.png"));
        ui->pathTable->setItem(4, 0, item);
        item = new QTableWidgetItem(settings->getMultisizePath());
        item->setToolTip(settings->getMultisizePath());
        ui->pathTable->setItem(4, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Layouts"));
        item->setIcon(QIcon("://icon/32x32/layout.png"));
        ui->pathTable->setItem(5, 0, item);
        item = new QTableWidgetItem(settings->getLayoutPath());
        item->setToolTip(settings->getLayoutPath());
        ui->pathTable->setItem(5, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Label Templates"));
        item->setIcon(QIcon("://icon/32x32/labels.png"));
        ui->pathTable->setItem(6, 0, item);
        item = new QTableWidgetItem(settings->getLabelTemplatePath());
        item->setToolTip(settings->getLabelTemplatePath());
        ui->pathTable->setItem(6, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Images"));
        item->setIcon(QIcon("://icon/32x32/add_image.png"));
        ui->pathTable->setItem(7, 0, item);
        item = new QTableWidgetItem(settings->getImageFilePath());
        item->setToolTip(settings->getImageFilePath());
        ui->pathTable->setItem(7, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Backups"));
        item->setIcon(QIcon("://icon/32x32/backup_files.png"));
        ui->pathTable->setItem(8, 0, item);
        item = new QTableWidgetItem(settings->getBackupFilePath());
        item->setToolTip(settings->getBackupFilePath());
        ui->pathTable->setItem(8, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Body Scans"));
        item->setIcon(QIcon("://icon/32x32/body_scan.png"));
        ui->pathTable->setItem(9, 0, item);
        item = new QTableWidgetItem(settings->getBodyScansPath());
        item->setToolTip(settings->getBodyScansPath());
        ui->pathTable->setItem(9, 1, item);
    }

    {
        // Path of the SeamlyLayout executable used by the Layout Mode handoff.
        // Empty means "auto-detect next to the Seamly2D executable".
        QTableWidgetItem *item = new QTableWidgetItem(tr("SeamlyLayout Application"));
        item->setIcon(QIcon("://icon/32x32/layout.png"));
        ui->pathTable->setItem(10, 0, item);
        item = new QTableWidgetItem(settings->getSeamlyLayoutAppPath());
        item->setToolTip(settings->getSeamlyLayoutAppPath());
        ui->pathTable->setItem(10, 1, item);
    }

    ui->pathTable->verticalHeader()->setDefaultSectionSize(20);
    ui->pathTable->resizeColumnsToContents();
    ui->pathTable->resizeRowsToContents();

    connect(ui->pathTable, &QTableWidget::itemSelectionChanged, this, [this]()
    {
        ui->defaultButton->setEnabled(true);
        ui->defaultButton->setDefault(false);

        ui->editButton->setEnabled(true);
        ui->editButton->setDefault(true);
    });
}
