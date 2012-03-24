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

#ifndef TABVIEW_H
#define TABVIEW_H

#include <QWidget>
#include <set>
#include <QHash>

#include "tabwidget.h"

class TabView : public QWidget
{
    Q_OBJECT
public:
    explicit TabView(QWidget *parent = 0);

private slots:
    void focusChanged(QWidget *prev, QWidget *now);

private:
    TabWidget *newTabWidget(QLayout *l);

    QHash<quint32, TabWidget*> m_tab_widgets;
    std::set<QLayout*> m_layouts;

    TabWidget *m_active_widget;
};

#endif // TABVIEW_H
