/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "PGA280.h"
#include "os.h"


bool CPGA280cmdBuf::transfer(CSPI &spi_bus, IPin &CS)
{
    size_t sz=m_cmd.size();
    for(size_t i=0; i<sz; i++)
    {
        m_cmd[i].PushToStream(m_ostr, m_bCSmode, i==(sz-1));
    }

    //setup the bus:
    spi_bus.set_phpol(false, true);
    CS.Set(true);
    os::uwait(80);

    bool tres=spi_bus.transfer(m_ostr, m_istr);

    CS.Set(false);
    os::uwait(80);

    if(!tres)
        return false;

    for(size_t i=0; i<sz; i++)
    {
        if(!m_cmd[i].PopFromStream(m_istr, m_bCSmode, i==(sz-1)))
            return false;
    }

    return true;
}

CPGA280::CPGA280(std::shared_ptr<CSPI> pSPIbus, std::shared_ptr<IPin> pCS)
{
    m_pSPIbus=pSPIbus;
    m_pCS=pCS;
}

bool CPGA280::ReadRegister(reg nReg, uint8_t &RegValue)
{
    m_CmdBuf.reset();
    m_CmdBuf.m_cmd.emplace_back(CPGA280cmd::cmd::read, nReg, 0);
    if( !m_CmdBuf.transfer(*m_pSPIbus, *m_pCS) )
        return false;

    RegValue=m_CmdBuf.m_cmd[0].m_InData;
    return true;

}
bool CPGA280::WriteRegister(reg nReg, uint8_t RegValue)
{
    m_CmdBuf.reset();
    m_CmdBuf.m_cmd.emplace_back(CPGA280cmd::cmd::write, nReg, RegValue);
    return m_CmdBuf.transfer(*m_pSPIbus, *m_pCS);
}


bool CPGA280::SetGains(ogain og, igain ig)
{
    typeCPGA280GainMuxReg reg;
    reg.reg=0;
    reg.bit.OGAIN=og;
    reg.bit.IGAIN=ig;

    return  WriteRegister(reg::gain_mux, reg.reg);
}
bool CPGA280::GetGains(ogain &og, igain &ig)
{
    typeCPGA280GainMuxReg reg;

    bool rv=ReadRegister(reg::gain_mux, reg.reg);
    og=static_cast<ogain>(reg.bit.OGAIN);
    ig=static_cast<igain>(reg.bit.IGAIN);
    return rv;
}
