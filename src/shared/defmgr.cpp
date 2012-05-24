/**********************************************
**    This file is part of Lorris
**    http://tasssadar.github.com/Lorris/
**
**    See README and COPYING
***********************************************/

#include <QDesktopServices>
#include <QFile>

#include "defmgr.h"

DefMgr::DefMgr(QObject *parent) :
    QObject(parent)
{
    loadChipdefs();
    loadFusedesc();
}

void DefMgr::loadChipdefs()
{
    m_chipdefs.clear();

    static const QString defFileLocations[] =
    {
        ":/definitions/chipdefs",
        "./shupito_chipdefs.txt",
        QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/shupito_chipdefs.txt",
    };

    QFile file;
    for(quint8 i = 0; i < sizeof(defFileLocations)/sizeof(QString); ++i)
    {
        file.setFileName(defFileLocations[i]);
        if(!file.open(QIODevice::ReadOnly))
            continue;

        QTextStream stream(&file);
        parseChipdefs(stream);
        file.close();
    }
}

void DefMgr::loadFusedesc()
{
    m_fusedesc.clear();

    static const QString defFileLocations[] =
    {
        ":/definitions/fusedesc",
        "./shupito_fusedesc.txt",
        QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/shupito_fusedesc.txt",
    };

    QFile file;
    for(quint8 i = 0; i < sizeof(defFileLocations)/sizeof(QString); ++i)
    {
        file.setFileName(defFileLocations[i]);
        if(!file.open(QIODevice::ReadOnly))
            continue;

        QTextStream stream(&file);
        parseFusedesc(stream);
        file.close();
    }
}

void DefMgr::parseChipdefs(QTextStream &ss)
{
    for(QString line = ss.readLine().trimmed(); !line.isNull(); line = ss.readLine().trimmed())
    {
        if(line.isEmpty() || line.startsWith("#"))
            continue;

        QStringList tokens = line.split(' ', QString::SkipEmptyParts);

        if(tokens.size() < 2)
            continue;

        chip_definition def;
        def.setName(tokens[0]);
        def.setSign(tokens[1]);

        // parse memories
        if(tokens.size() > 2)
        {
            QStringList memories = tokens[2].split(',', QString::SkipEmptyParts);
            for(quint8 i = 0; i < memories.size(); ++i)
            {
                QStringList memory_tokens = memories[i].split('=', QString::SkipEmptyParts);
                if(memory_tokens.size() != 2)
                    continue;

                QStringList mem_size_tokens = memory_tokens[1].split(':', QString::SkipEmptyParts);

                chip_definition::memorydef memdef;
                memdef.memid = i + 1;
                memdef.size = mem_size_tokens[0].toInt();
                if(mem_size_tokens.size() > 1)
                    memdef.pagesize = mem_size_tokens[1].toInt();
                else
                    memdef.pagesize = 0;
                def.getMems()[memory_tokens[0]] = memdef;
            }
        }

        // parse fuses
        for(quint32 i = 3; i < (quint32)tokens.size(); ++i)
        {
            if(tokens[i][0] == '!')
            {
                int sep_pos = tokens[i].indexOf('=');
                if(sep_pos == -1)
                    return Utils::ThrowException("Invalid syntax in the chip definition file.");
                def.getOptions()[tokens[i].mid(1, sep_pos - 1)] = tokens[i].mid(sep_pos + 1);
            }
            else
            {
                QStringList token_parts = tokens[i].split(':', QString::SkipEmptyParts);
                if(token_parts.size() != 2 && token_parts.size() != 3)
                    continue;

                QStringList bit_numbers = token_parts[1].split(',', QString::SkipEmptyParts);

                chip_definition::fuse f;
                f.name = token_parts[0];
                for(quint32 j = 0; j < (quint32)bit_numbers.size(); ++j)
                    f.bits.push_back(bit_numbers[j].toInt());

                if(token_parts.size() == 3)
                {
                    bit_numbers = token_parts[2].split(',', QString::SkipEmptyParts);
                    for(quint32 j = 0; j < (quint32)bit_numbers.size(); ++j)
                        f.values.push_back(bit_numbers[j].toInt());
                }
                def.getFuses().push_back(f);
            }
        }

        // If chip with this signature is already loaded, rewrite it
        quint32 i = 0;
        for(; i < m_chipdefs.size() && m_chipdefs[i].getSign() != def.getSign(); ++i);

        if(i == m_chipdefs.size())
            m_chipdefs.push_back(def);
        else
            m_chipdefs[i] = def;
    }
}

void DefMgr::parseFusedesc(QTextStream &ss)
{
    for(QString line = ss.readLine().trimmed(); !line.isNull(); line = ss.readLine().trimmed())
    {
        if(line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList tokens = line.split(" | ", QString::SkipEmptyParts);

        if(tokens.size() < 3)
            continue;

        fuse_desc desc(tokens[0]);

        QStringList chips = tokens[1].split(',', QString::SkipEmptyParts);
        if(chips.isEmpty())
            continue;

        desc.setChips(chips);

        if(tokens[2].count('"') != 2)
            continue;

        desc.setDesc(tokens[2].remove('"'));

        for(int i = 3; i < tokens.size(); ++i)
        {
            QStringList parts = tokens[i].split('=', QString::SkipEmptyParts);
            if(parts.size() != 2 || !parts[0].startsWith("0b") || parts[1].count('"') != 2)
                continue;

            desc.addOption(parts[0], parts[1].remove('"'));
        }
        m_fusedesc.insert(desc.getName(), desc);
    }
}

void DefMgr::updateChipdef(chip_definition & cd)
{
    for(quint32 i = 0; i < m_chipdefs.size(); ++i)
    {
        chip_definition &templ = m_chipdefs[i];
        if(cd.getSign() == templ.getSign())
        {
            cd.setName(templ.getName());
            cd.getMems() = templ.getMems();

            for(quint32 x = 0; x < templ.getFuses().size(); ++x)
            {
                quint32 k;
                for(k = 0; k < cd.getFuses().size(); ++k)
                {
                    if(cd.getFuses()[k].name == templ.getFuses()[x].name)
                        break;
                }

                if(k == cd.getFuses().size())
                    cd.getFuses().push_back(templ.getFuses()[x]);
            }
        }
    }

    if(cd.getMems().find("fuses") == cd.getMems().end() && cd.getSign().left(4) == "avr:")
    {
        chip_definition::memorydef mem;
        mem.memid = 3;
        mem.size = 4;
        mem.pagesize = 0;
        cd.getMems()["fuses"] = mem;
    }
}

fuse_desc *DefMgr::findFuse_desc(const QString &name, const QString &chipSign)
{
   QMultiHash<QString, fuse_desc>::iterator itr = m_fusedesc.find(name);
   for(; itr != m_fusedesc.end() && itr.key() == name; ++itr)
   {
       if((*itr).isForChip(chipSign))
           return &(*itr);
   }
   return NULL;
}