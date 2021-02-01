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

    bool tres=spi_bus.transfer(m_ostr, m_istr);

    CS.Set(false);

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

    m_GainMuxReg.reg=0;
    SetMode(mode::Voltage);
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
bool CPGA280::WriteRegister(reg nReg, uint8_t RegValue, bool TBUF)
{
    m_CmdBuf.reset();
    m_CmdBuf.m_cmd.emplace_back(CPGA280cmd::cmd::write, nReg, RegValue, TBUF);
    return m_CmdBuf.transfer(*m_pSPIbus, *m_pCS);
}

 bool CPGA280::SetMode(mode nMode)
 {
     //switch1 (assuming all other switches are zero after reset)
     typeCPGA280ISw1Reg sw1;
     sw1.reg=0;
     if(mode::Voltage==nMode)
     {
         sw1.bit.SW_A1=1;
         sw1.bit.SW_A2=1;
     }
     else
     {
         sw1.bit.SW_B1=1;
         sw1.bit.SW_B2=1;
     }
     if(!WriteRegister(reg::ISw1, sw1.reg))
         return false;


     //buf tmt to 0:
     if(!WriteRegister(reg::BUFtmt, 0, true))
         return false;

     m_nMode=nMode;
     return true;
 }

 bool CPGA280::SetIGain(igain ig)
 {
     typeCPGA280GainMuxReg reg;
     reg.reg=m_GainMuxReg.reg;

     reg.bit.IGAIN=ig;
     if(!WriteRegister(reg::gain_mux, reg.reg, true))
         return false;

     m_GainMuxReg.reg=reg.reg;
     return true;

 }
 bool CPGA280::SetOGain(ogain og)
 {
     typeCPGA280GainMuxReg reg;
     reg.reg=m_GainMuxReg.reg;

     reg.bit.OGAIN=og;
     if(!WriteRegister(reg::gain_mux, reg.reg, true))
         return false;

     m_GainMuxReg.reg=reg.reg;
     return true;
 }

 bool CPGA280::SetGains(igain ig, ogain og)
 {
     typeCPGA280GainMuxReg reg;
     reg.reg=m_GainMuxReg.reg;

     reg.bit.IGAIN=ig;
     reg.bit.OGAIN=og;
     if(!WriteRegister(reg::gain_mux, reg.reg, true))
         return false;

     m_GainMuxReg.reg=reg.reg;
     return true;
 }



