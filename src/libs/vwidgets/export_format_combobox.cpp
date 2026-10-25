 /******************************************************************************
  *   @file   export_format_combobox.cpp
  **  @author DS Caskey
  **  @date   @date   17 Sep, 2023
  **
  **  @brief
  **  @copyright
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

#include <QAbstractItemView>
#include <QPen>
#include <Qt>
#include <QPainter>
#include <QPixmap>
#include <QDebug>
#include <QVariant>

#include "export_format_combobox.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"

/**
 * Constructor with name.
 */
ExportFormatCombobox::ExportFormatCombobox(QWidget *parent, const char *name)
		: QComboBox(parent)
      	, m_currentFormat()
{
		 setObjectName(name);
		 setEditable ( false );
		 init();
}

/**
 * Destructor
 */
ExportFormatCombobox::~ExportFormatCombobox() {}

/**
 * Initialisation called from constructor or manually but only once.
 */
void ExportFormatCombobox::init()
{
    this->blockSignals(true);

    int count = 0;
    foreach (auto& v, initFormats())
    {
        addItem(v.first, QVariant(static_cast<int>(v.second)));
        count++;
    }
    setMaxVisibleItems(count);

    this->view()->setTextElideMode(Qt::ElideNone);

    this->blockSignals(false);
		connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ExportFormatCombobox::updateExportFormat);

    setCurrentToDefault();
    updateExportFormat(currentIndex());
}

LayoutExportFormat ExportFormatCombobox::getExportFormat() const
{
    return m_currentFormat;
}

/**
 * Sets the currently selected format item to the given format.
 */
void ExportFormatCombobox::setExportFormat(LayoutExportFormat &format)
{
    m_currentFormat = format;

    setCurrentIndex(findData(QVariant(static_cast<int>(format))));

    if (currentIndex()!= count() -1 )
    {
        updateExportFormat(currentIndex());
    }
}

//---------------------------------------------------------------------------------------------------------------------
QVector<std::pair<QString, LayoutExportFormat>> ExportFormatCombobox::initFormats()
{
    QVector<std::pair<QString, LayoutExportFormat>> list;

    auto InitFormat = [&list](LayoutExportFormat format)
    {
        list.append(std::make_pair(exportFormatDescription(format), format));
    };

    InitFormat(LayoutExportFormat::SVG);
    InitFormat(LayoutExportFormat::PDF);
    InitFormat(LayoutExportFormat::PNG);
    InitFormat(LayoutExportFormat::JPG);
    InitFormat(LayoutExportFormat::BMP);
    InitFormat(LayoutExportFormat::TIF);
    InitFormat(LayoutExportFormat::PPM);

    return list;
}

//---------------------------------------------------------------------------------------------------------------------
QString ExportFormatCombobox::exportFormatDescription(LayoutExportFormat format)
{
    const QString filesStr = tr("files");

    switch(format)
    {
        case LayoutExportFormat::SVG:
            return QString("SVG %1 (*.svg)").arg(filesStr);
        case LayoutExportFormat::PDF:
            return QString("PDF %1 (*.pdf)").arg(filesStr);
        case LayoutExportFormat::PNG:
            return QString("PNG %1 (*.png)").arg(filesStr);
        case LayoutExportFormat::JPG:
            return QString("JPG %1 (*.jpg)").arg(filesStr);
        case LayoutExportFormat::BMP:
            return QString("BMP %1 (*.bmp)").arg(filesStr);
        case LayoutExportFormat::TIF:
            return QString("TIF %1 (*.tif)").arg(filesStr);
        case LayoutExportFormat::PPM:
            return QString("PPM %1 (*.ppm)").arg(filesStr);
        default:
            return QString();
    }
}

//---------------------------------------------------------------------------------------------------------------------
QString ExportFormatCombobox::makeHelpFormatList()
{
   QString out("\n");
   foreach(auto& v, initFormats())
   {
       out += QLatin1String("\t") + v.first + QLatin1String(" = ") + QString::number(static_cast<int>(v.second))
               + QLatin1String("\n");
   }
   return out;
}

/**
 * Called when the width has changed. This method sets the current width to the value chosen or even
 * offers a dialog to the user that allows him/ her to choose an individual width.
 */
void ExportFormatCombobox::updateExportFormat(int index)
{
    QVariant format = itemData(index);
    if(format != QVariant::Invalid )
    {
       m_currentFormat = static_cast<LayoutExportFormat>(this->currentData().toInt());
    }

    emit exportFormatChanged(m_currentFormat);
}

void ExportFormatCombobox::setCurrentToDefault()
{
    QString format = qApp->Settings()->getExportFormat();

    int index = findText(format);
    if (index != -1)
    {
        setCurrentIndex(index);
    }
    else
    {
        setCurrentIndex(0);
    }
}
