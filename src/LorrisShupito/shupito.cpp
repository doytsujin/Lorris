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

#include <stdarg.h>
#include <stdio.h>
#include <exception>
#include <stdexcept>

#include "shupito.h"
#include "lorrisshupito.h"
#include "shupitodesc.h"
#include "connection/connectionmgr.h"

ShupitoPacket::ShupitoPacket(quint8 cmd, quint8 size, ...)
{
    itr = 0;

    m_data = QByteArray(size + 2, 0);
    m_data[itr++] = 0x80;
    m_data[itr++] = size | (cmd << 4);

    va_list args;
    va_start(args, size);
    for (quint8 i = 0; i < size; ++i)
        m_data[itr++] = (quint8)va_arg(args, int);
    va_end(args);
}

ShupitoPacket::ShupitoPacket()
{
    Clear();
}

void ShupitoPacket::set(bool resize, quint8 cmd, quint8 size)
{
    itr = 0;

    int target_size = resize ? size + 2 : 2;
    m_data = QByteArray(target_size, 0);

    m_data[itr++] = 0x80;
    m_data[itr++] = size | (cmd << 4);
}

void ShupitoPacket::Clear()
{
    m_data = QByteArray(2, 0);
    itr = 0;
}

bool ShupitoPacket::isValid()
{
    if((quint8)m_data.at(0) != 0x80 || itr < 2)
        return false;

    quint8 size = getLen();
    if((itr - 2) != size)
        return false;
    return true;
}

quint8 ShupitoPacket::addData(const QByteArray& data)
{
    quint8 read = 0;

    if(itr == 0)
    {
        if(quint8(data[read++]) == 0x80)
            m_data[itr++] = 0x80;
        else
            return 0;
    }
    if(itr == 1 && data.length() > 1)
        m_data[itr++] = data[read++];

    quint8 size = getLen();
    for(quint8 i = itr - 2; i < size && read < data.length(); ++i)
        m_data[itr++] = (char)data[read++];
    return read;
}

Shupito::Shupito(QObject *parent) :
    QObject(parent)
{
    m_con = NULL;
    m_desc = NULL;
    m_packet = ShupitoPacket();

    m_vdd_config = m_tunnel_config = NULL;
    m_tunnel_pipe = 0;
    m_tunnel_speed = 0;

    m_tunnel_timer.setInterval(50);
}

Shupito::~Shupito()
{
    sConMgr.RemoveShupito(this);
    emit tunnelStatus(false);
}

void Shupito::init(Connection *con, ShupitoDesc *desc)
{
    m_con = con;
    m_desc = desc;
    ShupitoPacket getInfo(MSG_INFO, 1, 0x00);
    sendPacket(getInfo);
}

void Shupito::readData(const QByteArray &data)
{
    mutex.lock();
    QByteArray dta = data;

    static bool first = true;

    std::vector<ShupitoPacket> packets;
    ShupitoPacket packet = m_packet;

    quint8 curRead = 1;
    while(dta.length() > 0)
    {
        if(first || curRead == 0)
        {
            int index = dta.indexOf(char(0x80));
            if(index == -1)
                break;
            dta.remove(0, index);
            first = false;
        }
        curRead = packet.addData(dta);
        dta.remove(0, curRead);
        if(packet.isValid())
        {
            packets.push_back(packet);
            packet = ShupitoPacket();
        }
    }
    m_packet = packet;
    mutex.unlock();

    for(quint32 i = 0; i < packets.size(); ++i)
        handlePacket(packets[i]);

}

void Shupito::sendPacket(ShupitoPacket& packet)
{
    QByteArray data = packet.getData(false);
    m_con->SendData(packet.getData(false));
}

void Shupito::handlePacket(ShupitoPacket& p)
{
    switch(p.getOpcode())
    {
        case MSG_INFO:
        {
            if(p.getLen() == 1)
                emit responseReceived(p[0]);
            else
            {
                m_desc->AddData(p.getData(), p.getLen() < 15);
                if(p.getLen() < 15)
                    emit descRead();
            }
            break;
        }
        case MSG_VCC:
        {
            handleVccPacket(p);
            break;
        }
        case MSG_TUNNEL:
        {
            handleTunnelPacket(p);
            break;
        }
    }
}

void Shupito::handleVccPacket(ShupitoPacket &p)
{
    //void handle_packet(yb_packet const & p)
    //vcc value
    if(p.getLen() == 4 && p[0] == 0x01 && p[1] == 0x03)
    {
        ShupitoDesc::config *vddCfg = m_desc->getConfig("1d4738a0-fc34-4f71-aa73-57881b278cb1");
        if(!vddCfg)
            return;

        double millivolts_per_unit = 1.0;
        if (vddCfg->data.size() == 5 && vddCfg->data[0] == 1)
        {
            qint32 mpu_16_16 = vddCfg->data[1] | (vddCfg->data[2] << 8) | (vddCfg->data[3] << 16) | (vddCfg->data[4] << 24);
            millivolts_per_unit = mpu_16_16 / 65536.0;
        }

        qint32 vdd = (qint8)p[p.getLen()-1];
        for (int i = p.getLen() - 1; i > 2; --i)
            vdd = (vdd << 8) | p[i-1];

        double value = vdd * (millivolts_per_unit / 1000.0);

        emit vccValueChanged(p[0] - 1, value);
    }

    // bool handle_vdd_desc(yb_packet const & p)
    else if(p.getLen() >= 3 && p[0] == 0 && p[1] == 0 && p[2] == 0)
    {
        m_vdd_setup.clear();
        for(quint8 i = 3; i < p.getLen(); ++i)
        {
            vdd_point vp;
            vp.current_drive = 0;
            quint8 len = p[i];
            if(i + 1 + len > p.getLen())
               return;
            vp.name.append(p.getData().mid(i+1, len));
            i += len + 1;
            m_vdd_setup.push_back(vp);
        }
        emit vddDesc(m_vdd_setup);
    }

    //bool handle_vdd_point_desc(yb_packet const & p)
    else if(p.getLen() >= 2 && p[0] == 0)
    {
        if(p[1] == 0 || p[1] > m_vdd_setup.size())
            return;

        vdd_point & vp = m_vdd_setup[p[1] - 1];
        vp.drives.clear();

        for(quint8 i = 2; i < p.getLen();)
        {
            if(i + 4 <= p.getLen())
            {
                double voltage = (p[i] | (p[i+1] << 8)) / 1000.0;
                quint16 amps = p[i+2]| (p[i+3] << 8);
                if(amps == 0)
                    vp.drives.push_back("<hiz>");
                else
                {
                    char buff[50];
                    sprintf(buff, "%.2fV, %dmA", voltage, amps);
                    vp.drives.push_back(QString(buff));
                }
                i += 4;
            }
        }
        emit vddDesc(m_vdd_setup);
    }

    //bool handle_vdd_drive_state(yb_packet const & p)
    else if(p.getLen() == 3 && p[1] == 1)
    {
        if(p[0]== 0 || p[0] > m_vdd_setup.size())
            return;

        vdd_point & vp = m_vdd_setup[p[1] - 1];
        vp.current_drive = p[2];

        emit vddDesc(m_vdd_setup);
    }
}

void Shupito::handleTunnelPacket(ShupitoPacket &p)
{
    if(!m_tunnel_config || p.getOpcode() != m_tunnel_config->cmd)
        return;

    if(p.getLen() >= 2 && p[0] == 0)
    {
        switch(p[1])
        {
            // Tunnel list
            case 0:
            {
                std::vector<QString> pipe_names;
                for(quint8 i = 2; i < p.getLen(); ++i)
                {
                    if (i + 1 + p[i] > p.getLen())
                        throw std::runtime_error("Invalid response while enumerating pipes.");

                    QString pipe_name;
                    for(quint8 j = 0; j < p[i]; ++j)
                        pipe_name.append(p[i+j+1]);
                    pipe_names.push_back(pipe_name);

                    i += 1 + p[i];
                }

                if(!pipe_names.empty())
                {
                    QString const & name = pipe_names[0];

                    QByteArray pkt_data;
                    pkt_data[0] = 0x80;
                    pkt_data[1] = (m_tunnel_config->cmd << 4) | (0x02 + name.length());
                    pkt_data[2] = 0x00;
                    pkt_data[3] = 0x01;
                    pkt_data.append(name);

                    m_con->SendData(pkt_data);
                }
                break;
            }
            // App tunnel activated
            case 1:
            {
                if(p.getLen() == 3)
                {
                    m_tunnel_pipe = p[2];
                    SendSetComSpeed();

                    sConMgr.AddShupito(m_con->GetIDString(), this);
                    emit tunnelStatus(true);

                    m_tunnel_data.clear();
                    connect(&m_tunnel_timer, SIGNAL(timeout()), SLOT(tunnelDataSend()));
                    m_tunnel_timer.start();
                    return;
                }
                break;
            }
            //App tunnel disabled
            case 2:
            {
                if(p.getLen() == 3 && p[2] == m_tunnel_pipe)
                {
                    m_tunnel_pipe = 0;

                    sConMgr.RemoveShupito(this);
                    emit tunnelStatus(false);

                    disconnect(&m_tunnel_timer, SIGNAL(timeout()), this, SLOT(tunnelDataSend()));
                    m_tunnel_timer.stop();
                    return;
                }
                break;
            }
        }
    }
    // Tunnel incoming data
    if (m_tunnel_pipe && p[0] == m_tunnel_pipe)
    {
        QByteArray data = p.getData();
        m_tunnel_data.append(data.remove(0, 1));
    }
}

//void send_set_comm_speed()
void Shupito::SendSetComSpeed()
{
    if(!m_tunnel_speed || !m_tunnel_config || m_tunnel_pipe == 0 ||
       m_tunnel_config->data.size() != 5 || m_tunnel_config->data[0] != 1)
        return;

    quint32 base_freq = m_tunnel_config->data[1] | (m_tunnel_config->data[2] << 8) |
            (m_tunnel_config->data[3] << 16) | (m_tunnel_config->data[4] << 24);

    double bsel = ((double)base_freq / m_tunnel_speed) - 1;
    qint8 bscale = 0;

    while(bscale > -6 && bsel < 2048)
    {
        bsel *= 2;
        --bscale;
    }

    quint16 res = (quint16)(bsel + 0.5);
    res |= bscale << 12;

    ShupitoPacket pkt = ShupitoPacket(m_tunnel_config->cmd, 5, 0, 3, m_tunnel_pipe,
                                      (quint8)res, (quint8)(res >> 8));
    sendPacket(pkt);
}

void Shupito::setTunnelSpeed(quint32 speed, bool send)
{
    m_tunnel_speed = speed;
    if(send)
        SendSetComSpeed();
}

qint16 Shupito::getTunnelCmd()
{
    if(m_tunnel_config)
        return m_tunnel_config->cmd;
    return -1;
}

void Shupito::tunnelDataSend()
{
    if(m_tunnel_data.size() > 0)
    {
        emit tunnelData(m_tunnel_data);
        m_tunnel_data.clear();
    }
    m_tunnel_timer.start();
}

void Shupito::setTunnelState(bool enable)
{
    if(!m_tunnel_config)
        return;

    if(enable && m_tunnel_pipe == 0)
    {
        ShupitoPacket packet(m_tunnel_config->cmd, 2, 0, 0);
        m_con->SendData(packet.getData(false));
    }
    else if(!enable && m_tunnel_pipe != 0)
    {
        ShupitoPacket packet(m_tunnel_config->cmd, 3, 0, 2, m_tunnel_pipe);
        m_con->SendData(packet.getData(false));
    }
}
