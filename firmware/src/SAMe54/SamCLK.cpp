/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//SAM clocks contol:

#include "SamCLK.h"
#include "sam.h"

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

