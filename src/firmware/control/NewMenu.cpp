/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "NewMenu.h"
#include "View.h"
#include "nodeControl.h"

CNewMenu::CNewMenu()
{

}

void CNewMenu::ObtainMenuElRange()
{
    nodeControl &nc=nodeControl::Instance();
    switch(m_MenuInd)
    {
        case CView::menu::Gains:
            m_MenuEl=static_cast<unsigned int>(nc.GetGain()-1); m_MenuElMin=0; m_MenuElMax=3;
        break;

        case CView::menu::Bridge:
            m_MenuEl=nc.GetBridge() ? 1:0; m_MenuElMin=0; m_MenuElMax=1;
        break;

        case CView::menu::Offsets:
            m_MenuEl=0; m_MenuElMin=0; m_MenuElMax=2;
        break;

        case CView::menu::SetSecondary:
            m_MenuEl=static_cast<unsigned int>(nc.GetSecondary()); m_MenuElMin=0; m_MenuElMax=1;
        break;
    }
}
void CNewMenu::ApplyMenuSetting()
{
    nodeControl &nc=nodeControl::Instance();
    switch(m_MenuInd)
    {
        case CView::menu::Gains:
            nc.SetGain( static_cast<int>(m_MenuEl+1) );
        break;

        case CView::menu::Bridge:
            nc.SetBridge(m_MenuEl);
        break;

        case CView::menu::Offsets:
            nc.SetOffset(m_MenuEl+1);
            m_CurMode=mode::preview;
        return;

        case CView::menu::SetSecondary:
            nc.SetSecondary( static_cast<int>(m_MenuEl) );
        break;
     }
    CView::Instance().ApplyMenu();
    m_CurMode=mode::preview;
}

void CNewMenu::OnButtonState(typeButtonState nState)
{
    CView &v=CView::Instance();

    if(typeButtonState::very_long_click==nState)
    {
        nodeControl::Instance().SetDefaultSettings();
        m_CurMode=mode::def;
        v.ResetSettings();
        return;
    }

    if(mode::def==m_CurMode)
    {
        switch(nState)
        {
            case typeButtonState::short_click:
                nodeControl::Instance().StartRecord(true);
                v.SetRecordMarker();
            return;

            case typeButtonState::long_click:
                m_CurMode=mode::preview;
                v.SelectMenuPrevew(m_MenuInd);
            return;

        default: return;
        }
    }

    //block buttons while calibration is running:
    if(nodeControl::Instance().GetOffsetRunSt())
        return;


    if(mode::preview==m_CurMode)
    {
        switch(nState)
        {
            case typeButtonState::double_click:
                m_CurMode=mode::def;
                v.ExitMenu();
            return;

            case typeButtonState::short_click:
                if(++m_MenuInd>=(CView::menu::total))
                    m_MenuInd=0;

                v.SelectMenuPrevew(m_MenuInd);
            return;

            case typeButtonState::long_click:
                if(m_MenuInd<CView::menu::total)
                {
                     m_CurMode=mode::inside_menu;
                     ObtainMenuElRange();
                     v.SelectMenu(m_MenuInd, m_MenuEl, m_MenuElMin, m_MenuElMax);
                }
            return;

            default: return;
        }
    }
    if(mode::inside_menu==m_CurMode)
    {
        switch(nState)
        {
            case typeButtonState::double_click:
                m_CurMode=mode::preview;
                v.SelectMenuPrevew(m_MenuInd);
            return;

            case typeButtonState::short_click:
                if(++m_MenuEl>m_MenuElMax)
                    m_MenuEl=m_MenuElMin;

                v.SelectMenu(m_MenuInd, m_MenuEl, m_MenuElMin, m_MenuElMax);
            return;

            case typeButtonState::long_click:
                ApplyMenuSetting();
            return;

            default: return;
        }
    }
}
