/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "View.h"

void CViewChannel::SelectVisMode(vismode nMode)
{
    m_VisMode=nMode;

    //reset LED:
    m_LED.SetBlinkMode(false);
    m_LED.SetColor( background==nMode ? m_LastBackgroundCol:0);
    m_LED.ON(true);
}


void CViewChannel::SetSensorIntensity(float normI)
{
    CView &v=CView::Instance();
    typeLEDcol col=v.GetBasicColor()*normI;
    m_LastBackgroundCol=col;

    if(background!=m_VisMode)
        return;

    m_LED.SetColor(col);
}


void CViewChannel::SetZeroSearchingMark()
{
    if(vismode::UI!=m_VisMode)
        return;

    m_LED.SetBlinkMode(true);
    m_LED.SetBlinkPeriodAndCount(100);
    m_LED.SetColor(CView::MENU_COLORS[CView::menu::Offsets][1]);
    m_LED.ON(true);
}
void CViewChannel::SetZeroFoundMark()
{
    if(vismode::UI!=m_VisMode)
        return;

    m_LED.SetBlinkMode(false);
    m_LED.SetColor(CView::MENU_COLORS[CView::menu::Offsets][1]);
}
void CViewChannel::SetZeroSearchErrorMark()
{
    if(vismode::UI!=m_VisMode)
        return;

    m_LED.SetBlinkMode(false);
    m_LED.SetColor(CView::ERROR_COLOR);
}

void CView::ZeroSearchCompleted()
{
    for(unsigned int i=0; i<m_Channels.size(); i++)
    {
        m_Channels[i].m_LED.SetBlinkMode(false);
        m_Channels[i].m_LED.ON(true);
    }
    Delay(2000, &CView::procApplySettingsEnd);
}


void CView::BlinkAtStart()
{
    SelectVisMode(CViewChannel::vismode::UI);
    nodeLED::blinkMultipleLED(typeLED::LED1, typeLED::LED4, m_BasicBoardCol, 2, 300);
    SetDefaultModeAfter(1200);
}

void CView::SetRecordMarker()
{
    SelectVisMode(CViewChannel::vismode::UI);
    nodeLED::blinkMultipleLED(typeLED::LED1, typeLED::LED4, MARKER_COLOR, 1, 300);
    SetDefaultModeAfter(400);
}

void CView::SelectMenuPrevew(unsigned int nMenu)
{
    SelectVisMode(CViewChannel::vismode::UI);
    /*if(nMenu>=menu::total) //this is reset
    {
        m_Channels[0].m_LED.SetColor(RESET_COLOR);
        for(unsigned int i=1; i<m_Channels.size(); i++)
        {
            m_Channels[i].m_LED.SetColor(0);
        }
        return;
    }*/
    for(unsigned int i=0; i<m_Channels.size(); i++)
    {
        //m_Channels[i].m_LED.SetColor(MENU_COLORS[i][i==nMenu ? 1:0]);
        m_Channels[i].m_LED.SetColor(MENU_COLORS[i][1]);
        m_Channels[i].m_LED.SetBlinkMode( i==nMenu );
        m_Channels[i].m_LED.SetBlinkPeriodAndCount(500);
        m_Channels[i].m_LED.ON(true);

    }
}
void CView::SelectMenu(unsigned int nMenu, unsigned int nActive, unsigned int nSelMin, unsigned int nSelMax)
{
    m_ActSelMenu=nMenu; m_ActSelElement=nActive;
    m_nSelRangeMin=nSelMin;  m_nSelRangeMax=nSelMax;

    SelectVisMode(CViewChannel::vismode::UI);
    for(unsigned int i=0; i<m_Channels.size(); i++)
    {
        //m_Channels[i].m_LED.SetColor(MENU_COLORS[nMenu][i==nActive ? 1:0]);

        m_Channels[i].m_LED.SetColor( (i>=nSelMin && i<=nSelMax) ? MENU_COLORS[nMenu][1] :0);
        m_Channels[i].m_LED.SetBlinkMode( i==nActive );
        m_Channels[i].m_LED.SetBlinkPeriodAndCount(500);
        m_Channels[i].m_LED.ON(true);

    }
}
void CView::ApplyMenu()
{
    SelectVisMode(CViewChannel::vismode::UI);
    //nodeLED::setMultipleLED(typeLED::LED1, typeLED::LED4, MENU_COLORS[m_ActSelMenu][1]);

    for(unsigned int i=0; i<m_Channels.size(); i++)
    {
        m_Channels[i].m_LED.SetColor( (i>=m_nSelRangeMin && i<=m_nSelRangeMax) ? MENU_COLORS[m_ActSelMenu][1] :0);
        m_Channels[i].m_LED.SetBlinkMode( true );
        m_Channels[i].m_LED.SetBlinkPeriodAndCount(100, 2);
        m_Channels[i].m_LED.ON(true);
    }


    Delay(500, &CView::procApplySettingsEnd);
}
void CView::ResetSettings()
{
    SelectVisMode(CViewChannel::vismode::UI);
    nodeLED::blinkMultipleLED(typeLED::LED1, typeLED::LED4, RESET_COLOR, 2, 300);

    //Delay(1200, &CView::procResetSettingsEnd);
    SetDefaultModeAfter(1200);
}

void CView::SetButtonHeartbeat(bool how)
{
    m_ButtonLEDphase=how;
    m_ButtonLEDphaseBeginTime_mS=os::get_tick_mS();
    SAMButton::Instance().TurnButtonLED(how);
}

void CView::Update()
{
    if(m_CurStep)
    {
        (this->*(m_CurStep))();
    }
    nodeLED::Update();

    if(m_ButtonLEDphase)
    {
        unsigned long time_now=os::get_tick_mS();
        if(m_ButtonLEDphase>=4)
        {
            if( (time_now - m_ButtonLEDphaseBeginTime_mS)<10000)
                return;

            m_ButtonLEDphase=1;
        }
        else
        {
            if( (time_now - m_ButtonLEDphaseBeginTime_mS)<500)
                return;

            m_ButtonLEDphase++;
        }

        m_ButtonLEDphaseBeginTime_mS=time_now;
        SAMButton::Instance().TurnButtonLED(m_ButtonLEDphase&1);

    }
}

