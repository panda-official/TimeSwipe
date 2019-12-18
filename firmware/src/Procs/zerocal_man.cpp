/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "os.h"
#include "zerocal_man.h"


#include "menu_logic.h"
unsigned long get_tick_mS(void);

void CCalMan::Start()
{
    nlohmann::json v=true;
    Fire_on_event("Zero", v);

    m_PState=FSM::running;
    unsigned int nSize=m_ChanCal.size();
    for(unsigned int i=0; i<nSize; i++)
    {
        m_ChanCal[i].Search(2048);
        m_pLED[i]->ON(true); //!!!!
        m_pLED[i]->SetBlinkMode(true);
        m_pLED[i]->SetColor(CMenuLogic::SETZERO_COLOR_ACTIVE);
    }
    m_LastTimeUpd=os::get_tick_mS();
    m_UpdSpan=100;

}
 void CCalMan::StopReset()
 {
     unsigned int nSize=m_ChanCal.size();
     for(unsigned int i=0; i<nSize; i++)
     {
         m_ChanCal[i].StopReset();
         m_pLED[i]->ON(false);
     }
     m_PState=FSM::halted;

     nlohmann::json v=false;
     Fire_on_event("Zero", v);
 }
void CCalMan::Update()
{
    unsigned long cur_time=os::get_tick_mS();
    if( cur_time-m_LastTimeUpd < m_UpdSpan)
        return;
    m_LastTimeUpd=cur_time;

    if(FSM::halted==m_PState)
        return;

    if(FSM::running==m_PState){

    bool bRunning=false;
    unsigned int nSize=m_ChanCal.size();
    for(unsigned int i=0; i<nSize; i++)
    {
        m_ChanCal[i].Update();
        typePTsrcState tstate=m_ChanCal[i].state();
        if(typePTsrcState::searching==tstate)
        {
            bRunning=true;
        }
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
    if(!bRunning)
    {
        m_UpdSpan=1000; //1 sec delay
        m_PState=FSM::delay;
    }
    }
    else
    {
        StopReset();
    }
}
