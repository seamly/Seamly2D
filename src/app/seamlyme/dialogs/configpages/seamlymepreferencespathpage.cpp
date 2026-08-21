/******************************************************************************
*   @file   seamlymepreferencespathpage.cpp
**  @author Douglas S Caskey
**  @date   14 Jul, 2023
**
**  @brief
**  @copyright
**  This source code is part of the Seamly2D project, a pattern making
**  program to create and model patterns of clothing.
**  Copyright (C) 2017-2023 Seamly2D project
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
*************************************************************************/

/************************************************************************
 **
 **  @file   seamlymepreferencespathpage.cpp
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

#include "seamlymepreferencespathpage.h"
#include "ui_seamlymepreferencespathpage.h"
#include "../../application_me.h"
#include "../vmisc/vseamlymesettings.h"

#include <QDir>
#include <QFileDialog>

//---------------------------------------------------------------------------------------------------------------------
SeamlyMePreferencesPathPage::SeamlyMePreferencesPathPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SeamlyMePreferencesPathPage)
{
    ui->setupUi(this);

    initializeTable();

    connect(ui->defaultButton, &QPushButton::clicked, this, &SeamlyMePreferencesPathPage::defaultPath);
    connect(ui->editButton,    &QPushButton::clicked, this, &SeamlyMePreferencesPathPage::editPath);
}

//---------------------------------------------------------------------------------------------------------------------
SeamlyMePreferencesPathPage::~SeamlyMePreferencesPathPage()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void SeamlyMePreferencesPathPage::changeEvent(QEvent *event)
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
 * The shared data root (row 0) is applied first, and every subfolder still living inside
 * the previous root follows it to the new location, so repointing the root at another
 * drive or a cloud-synced folder relocates the whole tree in one edit (Task 34).
 */
void SeamlyMePreferencesPathPage::Apply()
{
    VSeamlyMeSettings *settings = qApp->seamlyMeSettings();

    const QString previousRoot = settings->getDataRoot();
    const QString dataRoot     = ui->pathTable->item(0, 1)->text();
    settings->setDataRoot(dataRoot);

    settings->setIndividualSizePath(
        VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(1, 1)->text(), previousRoot, dataRoot));
    settings->setMultisizePath(
        VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(2, 1)->text(), previousRoot, dataRoot));
    settings->setTemplatePath(
        VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(3, 1)->text(), previousRoot, dataRoot));
    settings->setBodyScansPath(
        VCommonSettings::rebaseOntoDataRoot(ui->pathTable->item(4, 1)->text(), previousRoot, dataRoot));
}

//---------------------------------------------------------------------------------------------------------------------
void SeamlyMePreferencesPathPage::defaultPath()
{
    const int row = ui->pathTable->currentRow();
    QTableWidgetItem *item = ui->pathTable->item(row, 1);
    SCASSERT(item != nullptr)

    QString path;
    switch (row)
    {
        case 0: // shared user data root
            path = VCommonSettings::getDefaultDataRoot();
            break;
        case 1: // individual measurements
            path = VCommonSettings::getDefaultIndividualSizePath();
            break;
        case 2: // multisize measurements
            path = VCommonSettings::getDefaultMultisizePath();
            break;
        case 3: // templates
            path = VCommonSettings::getDefaultTemplatePath();
            break;
        case 4: // body scans
            path = VCommonSettings::getDefaultBodyScansPath();
            break;
        default:
            break;
    }

    item->setText(path);
    item->setToolTip(path);
}

//---------------------------------------------------------------------------------------------------------------------
void SeamlyMePreferencesPathPage::editPath()
{
    const int row = ui->pathTable->currentRow();
    QTableWidgetItem *item = ui->pathTable->item(row, 1);
    SCASSERT(item != nullptr)

    QString path;
    switch (row)
    {
        case 0: // shared user data root
            // Any drive, volume or path is accepted here, including an external disk or a
            // cloud-synced folder such as G:/My Drive/seamly (Task 34).
            path = qApp->seamlyMeSettings()->getDataRoot();
            break;
        case 1: // individual measurements
            path = qApp->seamlyMeSettings()->getIndividualSizePath();
            break;
        case 2: // multisize measurements
            path = qApp->seamlyMeSettings()->getMultisizePath();
            path = VCommonSettings::prepareMultisizeTables(path);
            break;
        case 3: // templates
            path = qApp->seamlyMeSettings()->getTemplatePath();
            break;
        case 4: // body scans
            path = qApp->seamlyMeSettings()->getBodyScansPath();
            break;
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
                                                              qApp->seamlyMeSettings()->getUseNativeFileDialogs(),
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
void SeamlyMePreferencesPathPage::initializeTable()
{
    ui->pathTable->setRowCount(5);
    ui->pathTable->setColumnCount(2);

    const VSeamlyMeSettings *settings = qApp->seamlyMeSettings();

    {
        // Task 34: the shared user-data root, the same setting seamly2d edits — both apps
        // read it from the common settings file, so a change here moves both apps' data.
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Seamly Data"));
        item->setIcon(QIcon("://icon/32x32/template_size_file.png"));
        ui->pathTable->setItem(0, 0, item);
        item = new QTableWidgetItem(settings->getDataRoot());
        item->setToolTip(settings->getDataRoot());
        ui->pathTable->setItem(0, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Individual Measurements"));
        item->setIcon(QIcon("://icon/32x32/individual_size_file.png"));
        ui->pathTable->setItem(1, 0, item);
        item = new QTableWidgetItem(settings->getIndividualSizePath());
        item->setToolTip(settings->getIndividualSizePath());
        ui->pathTable->setItem(1, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Multisize Measurements"));
        item->setIcon(QIcon("://icon/32x32/multisize_size_file.png"));
        ui->pathTable->setItem(2, 0, item);
        item = new QTableWidgetItem(settings->getMultisizePath());
        item->setToolTip(settings->getMultisizePath());
        ui->pathTable->setItem(2, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Templates"));
        item->setIcon(QIcon("://icon/32x32/template_size_file.png"));
        ui->pathTable->setItem(3, 0, item);
        item = new QTableWidgetItem(settings->getTemplatePath());
        item->setToolTip(settings->getTemplatePath());
        ui->pathTable->setItem(3, 1, item);
    }

    {
        QTableWidgetItem *item = new QTableWidgetItem(tr("My Body Scans"));
        item->setIcon(QIcon("://icon/32x32/body_scan.png"));
        ui->pathTable->setItem(4, 0, item);
        item = new QTableWidgetItem(settings->getBodyScansPath());
        item->setToolTip(settings->getBodyScansPath());
        ui->pathTable->setItem(4, 1, item);
    }

    ui->pathTable->verticalHeader()->setDefaultSectionSize(20);
    ui->pathTable->resizeColumnsToContents();
    ui->pathTable->resizeRowsToContents();
    ui->pathTable->horizontalHeader()->setStretchLastSection(true);

    connect(ui->pathTable, &QTableWidget::itemSelectionChanged, this, [this]()
    {
        ui->defaultButton->setEnabled(true);
        ui->defaultButton->setDefault(false);

        ui->editButton->setEnabled(true);
        ui->editButton->setDefault(true);
    });
}
