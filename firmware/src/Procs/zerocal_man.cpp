/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "os.h"
#include "zerocal_man.h"


void CCalMan::Serialize(CStorage &st)
{
    bool bSet=st.IsDownloading();
    for(auto &ch : m_ChanCal)
    {
        if(st.IsDefaultSettingsOrder())
            ch.m_PrmOffset=2048;

        st.ser( (ch.m_PrmOffset) );
        if(bSet)
        {
            ch.m_pDAC->SetRawOutput(ch.m_PrmOffset);
        }
    }
}

void CCalMan::Start(int val)
{
    //nlohmann::json v=true;
    //Fire_on_event("Zero", v);

    m_PState=FSM::running;
    unsigned int nSize=m_ChanCal.size();
    for(unsigned int i=0; i<nSize; i++)
    {
        m_ChanCal[i].Search(val);
        CView::Instance().GetChannel(m_VisChan[i]).SetZeroSearchingMark();
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
     }
     m_PState=FSM::halted;
     CView::Instance().ZeroSearchCompleted();

     //nlohmann::json v=false;
     //Fire_on_event("Zero", v);
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
            CViewChannel &ch=CView::Instance().GetChannel(m_VisChan[i]);
            switch (tstate)
            {
                case typePTsrcState::error:
                    ch.SetZeroSearchErrorMark();
                break;

                case typePTsrcState::found:
                    ch.SetZeroFoundMark();
                break;
            }
        }
    }
    if(!bRunning)
    {
        StopReset();

    } }
}
