/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/


#include "zerocal_man.h"
//#include <math.h>


#include "menu_logic.h"
unsigned long get_tick_mS(void);

void CCalMan::Start()
{
    unsigned int nSize=m_ChanCal.size();
    for(unsigned int i=0; i<nSize; i++)
    {
        m_ChanCal[i].Search(2048);
        m_pLED[i]->ON(true); //!!!!
        m_pLED[i]->SetBlinkMode(true);
        m_pLED[i]->SetColor(CMenuLogic::SETZERO_COLOR_ACTIVE);
    }
    m_LastTimeUpd=get_tick_mS();
}
 void CCalMan::StopReset()
 {
     unsigned int nSize=m_ChanCal.size();
     for(unsigned int i=0; i<nSize; i++)
     {
         m_ChanCal[i].StopReset();
         m_pLED[i]->ON(false);
     }
 }
void CCalMan::Update()
{
    unsigned long cur_time=get_tick_mS();
    if( cur_time-m_LastTimeUpd < 100)
        return;
    m_LastTimeUpd=cur_time;

    unsigned int nSize=m_ChanCal.size();
    for(unsigned int i=0; i<nSize; i++)
    {
        m_ChanCal[i].Update();
        typePTsrcState tstate=m_ChanCal[i].state();
        if(tstate!=m_State[i])
        {
            m_State[i]=tstate;
            switch (tstate)
            {
                case typePTsrcState::error:
                    m_pLED[i]->SetBlinkMode(false);
                    m_pLED[i]->SetColor(LEDrgb(255,0,0));
                break;

                case typePTsrcState::found:
                    m_pLED[i]->SetBlinkMode(false);
                    m_pLED[i]->SetColor(CMenuLogic::SETZERO_COLOR_ACTIVE);
            }
        }
    }
}
