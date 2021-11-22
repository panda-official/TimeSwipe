// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "clock_generator.hpp"

#include <sam.h>

std::list<CSamCLK *> CSamCLK::m_Clocks;
bool                 CSamCLK::m_bOcupied[12]={0};

//factory:
std::shared_ptr<CSamCLK> CSamCLK::Factory()
{
    for(int i=static_cast<int>(typeSamCLK::GCLK2); i<=static_cast<int>(typeSamCLK::GCLK11); i++)
    {
        //check if hardware occupied by a first time
        if(GCLK->GENCTRL[i].bit.GENEN)
        {
            m_bOcupied[i]=true; continue;
        }


        if(!m_bOcupied[i])
        {
            m_bOcupied[i]=true;
            CSamCLK *pClk= new CSamCLK; //because of protected ctor
            pClk->m_nCLK=i;
            m_Clocks.push_back(pClk);
            GCLK->GENCTRL[i].bit.SRC=GCLK_GENCTRL_SRC_DFLL; //def source
            pClk->WaitSync();
            return std::shared_ptr<CSamCLK>(pClk);
        }
    }
    return nullptr;
}
CSamCLK::~CSamCLK()
{
    m_Clocks.remove(this);
    m_bOcupied[m_nCLK]=false;
}

void CSamCLK::WaitSync()
{
    while( GCLK->SYNCBUSY.reg & (4UL<<m_nCLK)){}
}
void CSamCLK::SetDiv(unsigned short div)
{
    GCLK->GENCTRL[m_nCLK].bit.DIV=div;
    WaitSync();
}
void CSamCLK::Enable(bool how)
{
    GCLK->GENCTRL[m_nCLK].bit.GENEN=how ? 1:0;
    WaitSync();
}
