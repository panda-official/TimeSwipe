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

}
void CViewChannel::SetZeroFoundMark()
{

}
void CViewChannel::SetZeroSearchErrorMark()
{

}

void CView::SelectMenuPrevew(unsigned int nMenu)
{
    SelectVisMode(CViewChannel::vismode::UI);
    if(nMenu>=menu::total) //this is reset
    {
        m_Channels[0].m_LED.SetColor(RESET_COLOR);
        for(unsigned int i=1; i<m_Channels.size(); i++)
        {
            m_Channels[i].m_LED.SetColor(0);
        }
        return;
    }
    for(unsigned int i=0; i<m_Channels.size(); i++)
    {
        m_Channels[i].m_LED.SetColor(MENU_COLORS[i][i==nMenu ? 1:0]);
    }
}
void CView::SelectMenu(unsigned int nMenu, unsigned int nActive)
{
    SelectVisMode(CViewChannel::vismode::UI);
    for(unsigned int i=0; i<m_Channels.size(); i++)
    {
        m_Channels[i].m_LED.SetColor(MENU_COLORS[nMenu][i==nActive ? 1:0]);
    }
}
void CView::ApplyMenu(unsigned int nMenu)
{
    SelectVisMode(CViewChannel::vismode::UI);
    nodeLED::blinkMultipleLED(typeLED::LED1, typeLED::LED4, MENU_COLORS[nMenu][0], 1, 300);
}
void CView::ResetSettings()
{
    SelectVisMode(CViewChannel::vismode::UI);
    nodeLED::blinkMultipleLED(typeLED::LED1, typeLED::LED4, RESET_COLOR, 1, 300);
}

