/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "SamTC.h"
#include "sam.h"

CSamTC::CSamTC(typeSamTC nTC)
{
    m_nTC=nTC;
}

void CSamTC::EnableIRQ(bool how)
{
    IRQn_Type nIRQ=static_cast<IRQn_Type>(TC0_IRQn + static_cast<int>(m_nTC));

    if(how)
    {
        __NVIC_EnableIRQ(nIRQ);
    }
    else
    {
        __NVIC_DisableIRQ(nIRQ);
    }
}
void CSamTC::EnableAPBbus(typeSamTC nTC, bool how)
{
    unsigned int set=how ? 1:0;
    switch(nTC)
    {
        case typeSamTC::Tc0 : MCLK->APBAMASK.bit.TC0_=set; break;
        case typeSamTC::Tc1 : MCLK->APBAMASK.bit.TC1_=set; break;
        case typeSamTC::Tc2 : MCLK->APBBMASK.bit.TC2_=set; break;
        case typeSamTC::Tc3 : MCLK->APBBMASK.bit.TC3_=set; break;
        case typeSamTC::Tc4 : MCLK->APBCMASK.bit.TC4_=set; break;
        case typeSamTC::Tc5 : MCLK->APBCMASK.bit.TC5_=set; break;
        case typeSamTC::Tc6 : MCLK->APBDMASK.bit.TC6_=set; break;
        case typeSamTC::Tc7 : MCLK->APBDMASK.bit.TC7_=set; break;
    }
}
void CSamTC::ConnectGCLK(typeSamCLK nCLK)
{
    int pind;
    switch(m_nTC)
    {
        case typeSamTC::Tc0: pind=9;   break;
        case typeSamTC::Tc1: pind=9;   break;
        case typeSamTC::Tc2: pind=26;  break;
        case typeSamTC::Tc3: pind=26;  break;
        case typeSamTC::Tc4: pind=30;  break;
        case typeSamTC::Tc5: pind=30;  break;
        case typeSamTC::Tc6: pind=39;  break;
        case typeSamTC::Tc7: pind=39;
    }

    if(nCLK==typeSamCLK::none)
    {
        GCLK->PCHCTRL[pind].bit.CHEN=0; //remove
    }
    else
    {
       GCLK->PCHCTRL[pind].bit.GEN=(uint32_t)nCLK;
       GCLK->PCHCTRL[pind].bit.CHEN=1; //add
    }
}

Tc *glob_GetTcPtr(typeSamTC nTc)
{
    switch(nTc)
    {
        case typeSamTC::Tc0: return TC0;
        case typeSamTC::Tc1: return TC1;
        case typeSamTC::Tc2: return TC2;
        case typeSamTC::Tc3: return TC3;
        case typeSamTC::Tc4: return TC4;
        case typeSamTC::Tc5: return TC5;
        case typeSamTC::Tc6: return TC6;
        case typeSamTC::Tc7: return TC7;
    }
    return nullptr;
}

