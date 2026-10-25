//-----------------------------------------------------------------------------
//  @file   export_layout_dialog.cpp
//  @author Douglas S Caskey
//  @date   Apr 1, 2026
//
//  @brief
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2013-2026 Seamly2D project
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
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  @file   export_layout_dialog.cpp
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date   22 1, 2015
//
//  @brief
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program to create and model patterns of clothing.
//  Copyright (C) 2017-2026 Seamly2D project
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
//-----------------------------------------------------------------------------

#include "export_layout_dialog.h"
#include "ui_export_layout_dialog.h"
#include "../options.h"
#include "../core/application_2d.h"
#include "../vmisc/def.h"
#include "../vmisc/vsettings.h"
#include "../ifc/exception/vexception.h"
#include "../vwidgets/export_format_combobox.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QtDebug>
#include <QRegularExpression>
#include <QSpinBox>
#include <QtDebug>

const QString fileNameRegExp = QStringLiteral("^[\\p{L}\\p{Nd}\\-. _]+$");

//---------------------------------------------------------------------------------------------------------------------
ExportLayoutDialog::ExportLayoutDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExportLayoutDialog)
    , m_isInitialized(false)
    , m_saveButton(nullptr)
    , m_foreGroundColor()
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->path_LineEdit->setClearButtonEnabled(true);
    ui->filename_LineEdit->setClearButtonEnabled(true);

    // Need to save foreground textcolor.
    QPalette palette = ui->path_LineEdit->palette();
    m_foreGroundColor = palette.text().color();

    qApp->Seamly2DSettings()->getOsSeparator() ? setLocale(QLocale()) : setLocale(QLocale::c());

    m_saveButton = ui->buttonBox->button(QDialogButtonBox::Save);
    SCASSERT(m_saveButton != nullptr)
    m_saveButton->setEnabled(false);
    m_saveButton->setText(tr("Export"));

    ui->filename_LineEdit->setValidator( new QRegularExpressionValidator(QRegularExpression(fileNameRegExp), this));

    const QString mask = fileName + modeString();
    if (Application2D::isGUIMode())
    {
        ui->filename_LineEdit->setText(mask);
    }
    else
    {
        if (QRegularExpression(fileNameRegExp).match(mask).hasMatch())
        {
            ui->filename_LineEdit->setText(mask);
        }
        else
        {
            VException e(tr("The base filename does not match a regular expression."));
            throw e;
        }
    }

    connect(m_saveButton, &QPushButton::clicked, this, &ExportLayoutDialog::save);
    connect(ui->browse_PushButton, &QPushButton::clicked, this, [this]()
    {
        const QString dirPath = qApp->Seamly2DSettings()->getLayoutPath();
        bool usedNotExistedDir = false;
        QDir directory(dirPath);
        if (!directory.exists())
        {
            usedNotExistedDir = directory.mkpath(".");
        }

        const QString dir = QFileDialog::getExistingDirectory(this, tr("Select folder"), dirPath,
                                                              QFileDialog::ShowDirsOnly
                                                              | QFileDialog::DontResolveSymlinks
                                                              | qApp->Seamly2DSettings()->getUseNativeFileDialogs());
        if (!dir.isEmpty())
        {// If the paths are equal the slot will not be called, we will do this manually
            dir == ui->path_LineEdit->text() ? pathChanged(dir) : ui->path_LineEdit->setText(dir);
        }

        if (usedNotExistedDir)
        {
            QDir directory(dirPath);
            directory.rmpath(".");
        }
    });
    connect(ui->path_LineEdit, &QLineEdit::textChanged, this, &ExportLayoutDialog::pathChanged);
    connect(ui->quality_Slider, &QSlider::valueChanged, this, [this]()
    {
        ui->percent_SpinBox->setValue(ui->quality_Slider->value());
    });
    connect(ui->percent_SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int i)
    {
        ui->quality_Slider->setValue(i);
    });

    ui->path_LineEdit->setText(qApp->Seamly2DSettings()->getLayoutPath());

    readSettings();

    adjustSize();
    setFixedHeight(this->height());
}

//---------------------------------------------------------------------------------------------------------------------
QString ExportLayoutDialog::exportFormatSuffix(LayoutExportFormat format)
{
    switch(format)
    {
        case LayoutExportFormat::SVG:
            return ".svg";
        case LayoutExportFormat::PDF:
            return ".pdf";
        case LayoutExportFormat::PNG:
            return ".png";
        case LayoutExportFormat::JPG:
            return ".jpg";
        case LayoutExportFormat::BMP:
            return ".bmp";
        case LayoutExportFormat::PPM:
            return ".ppm";
        case LayoutExportFormat::TIF:
            return ".tif";
        default:
            return QString();
    }
}

//---------------------------------------------------------------------------------------------------------------------
ExportLayoutDialog::~ExportLayoutDialog()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
QString ExportLayoutDialog::path() const
{
    return ui->path_LineEdit->text();
}

//---------------------------------------------------------------------------------------------------------------------
QString ExportLayoutDialog::fileName() const
{
    return ui->filename_LineEdit->text();
}

//---------------------------------------------------------------------------------------------------------------------
void ExportLayoutDialog::setFileName(const QString &name)
{
    ui->filename_LineEdit->setText(name);
}

//---------------------------------------------------------------------------------------------------------------------
LayoutExportFormat ExportLayoutDialog::format() const
{
    return static_cast<LayoutExportFormat>(ui->format_ComboBox->currentData().toInt());
}

//---------------------------------------------------------------------------------------------------------------------
QString ExportLayoutDialog::formatText() const
{
    return ui->format_ComboBox->currentText();
}

//---------------------------------------------------------------------------------------------------------------------
void ExportLayoutDialog::save()
{
    const QString name = QString("%1/%2%3")
    .arg(path())                        //1
    .arg(fileName())                    //2
    .arg(exportFormatSuffix(format())); //3

    if (QFile::exists(name))
    {
        QMessageBox messageBox(this);
        messageBox.setWindowTitle(tr("Confirm Export"));
        messageBox.setIcon(QMessageBox::Question);
        messageBox.setText(tr("%1 already exists.").arg(fileName() + exportFormatSuffix(format())));
        messageBox.setInformativeText(QStringLiteral("Do you want to replace it?"));
        messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        messageBox.setDefaultButton(QMessageBox::Yes);

        if (messageBox.exec() == QMessageBox::No)
        {
            reject();
            return;
        }
    }
    writeSettings();
    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void ExportLayoutDialog::pathChanged(const QString &text)
{
    QPalette palette = ui->path_LineEdit->palette();

    QDir dir(text);
    dir.setPath(text);
    if (dir.exists(text))
    {
        m_saveButton->setEnabled(true);
        palette.setColor(ui->path_LineEdit->foregroundRole(), m_foreGroundColor);
    }
    else
    {
        m_saveButton->setEnabled(false);
        palette.setColor(ui->path_LineEdit->foregroundRole(), Qt::red);
    }

    ui->path_LineEdit->setPalette(palette);
}

//---------------------------------------------------------------------------------------------------------------------
void ExportLayoutDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent( event );
    if ( event->spontaneous() )
    {
        return;
    }

    if (m_isInitialized)
    {
        return;
    }

    m_isInitialized = true;//first show windows are held
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief readSettings sets the image quality control from the settings.
 */
void ExportLayoutDialog::readSettings()
{
    VSettings *settings = qApp->Seamly2DSettings();
    ui->quality_Slider->setValue(settings->getExportQuality());
    ui->percent_SpinBox->setValue(ui->quality_Slider->value());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief writeSettings saves the image quality and, when the user wants it, the export format.
 */
void ExportLayoutDialog::writeSettings() const
{
    qApp->Seamly2DSettings()->setExportQuality(ui->quality_Slider->value());

    //Export Format
    if (qApp->Settings()->useLastExportFormat())
    {
        qApp->Settings()->setExportFormat(formatText());
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief modeString returns the file name suffix of a draft block export, empty when the user turned it off.
 */
QString ExportLayoutDialog::modeString() const
{
    QString modeStr = QStringLiteral("");
    if (qApp->Seamly2DSettings()->useModeType())
    {
        modeStr = QStringLiteral("_block");
    }
    return modeStr;
}
