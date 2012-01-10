/****************************************************************************
**
**    This file is part of Lorris.
**    Copyright (C) 2012 Vojtěch Boček
**
**    Contact: <vbocek@gmail.com>
**             https://github.com/Tasssadar
**
**    Lorris is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    Lorris is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with Lorris.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef COLORWIDGET_H
#define COLORWIDGET_H

#include "datawidget.h"

class ColorWidget : public DataWidget
{
    Q_OBJECT
public:
    ColorWidget(QWidget *parent = 0);
    ~ColorWidget();

    void setUp();
    void saveWidgetInfo(AnalyzerDataFile *file);
    void loadWidgetInfo(AnalyzerDataFile *file);

protected:
     void processData(analyzer_data *data);

private slots:

private:
     QWidget *m_widget;
};

class ColorWidgetAddBtn : public DataWidgetAddBtn
{
    Q_OBJECT
public:
    ColorWidgetAddBtn(QWidget *parent = 0);

protected:
    QPixmap getRender();
};
#endif // COLORWIDGET_H