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

#include "lorrisproxy.h"
#include "lorrisproxyinfo.h"

static const LorrisProxyInfo info;

LorrisProxyInfo::LorrisProxyInfo()
{

}

LorrisProxyInfo::~LorrisProxyInfo()
{

}

WorkTab *LorrisProxyInfo::GetNewTab()
{
    return new LorrisProxy();
}

QString LorrisProxyInfo::GetName()
{
    return QObject::tr("Serial port proxy");
}

QString LorrisProxyInfo::GetDescription()
{
    return QObject::tr("This module acts as proxy between serial port and TCP socket");
}

