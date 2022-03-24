/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "../board.hpp"
#include "NewMenu.h"
#include "View.h"

CNewMenu::CNewMenu()
{

}

void CNewMenu::ObtainMenuElRange()
{
    Board& board = Board::instance();
    switch (m_MenuInd) {
        case CView::menu::Gains:
          m_MenuEl=static_cast<unsigned int>(board.gain() - 1);
          m_MenuElMin=0;
          m_MenuElMax=3;
        break;

        case CView::menu::Bridge:
          m_MenuEl=board.is_bridge_enabled();
          m_MenuElMin=0;
          m_MenuElMax=1;
        break;

        case CView::menu::Offsets:
          m_MenuEl=0;
          m_MenuElMin=0;
          m_MenuElMax=2;
        break;

        case CView::menu::SetSecondary:
          m_MenuEl=static_cast<unsigned int>(board.secondary_measurement_mode());
          m_MenuElMin=0;
          m_MenuElMax=1;
        break;
    }
}

void CNewMenu::ApplyMenuSetting()
{
    Board& board = Board::instance();
    switch(m_MenuInd)
    {
        case CView::menu::Gains:
            board.set_gain(static_cast<int>(m_MenuEl+1));
        break;

        case CView::menu::Bridge:
            board.enable_bridge(m_MenuEl);
        break;

        case CView::menu::Offsets:
            board.start_offset_search(m_MenuEl + 1);
            m_CurMode=mode::preview;
        return;

        case CView::menu::SetSecondary:
            board.set_secondary_measurement_mode(static_cast<int>(m_MenuEl));
        break;
     }
    CView::Instance().ApplyMenu();
    m_CurMode=mode::preview;
}

void CNewMenu::handle_state(const Button_state state)
{
    CView &v=CView::Instance();

    if(Button_state::very_long_click==state)
    {
        Board::instance().reset_settings();
        m_CurMode=mode::def;
        v.ResetSettings();
        return;
    }

    if(mode::def==m_CurMode)
    {
        switch(state)
        {
            case Button_state::short_click:
                v.SetRecordMarker();
            return;

            case Button_state::long_click:
                m_CurMode=mode::preview;
                v.SelectMenuPrevew(m_MenuInd);
            return;

        default: return;
        }
    }

    // block buttons while calibration is running:
    if (Board::instance().is_offset_search_started())
        return;


    if(mode::preview==m_CurMode)
    {
        switch(state)
        {
            case Button_state::double_click:
                m_CurMode=mode::def;
                v.ExitMenu();
            return;

            case Button_state::short_click:
                if(++m_MenuInd>=(CView::menu::total))
                    m_MenuInd=0;

                v.SelectMenuPrevew(m_MenuInd);
            return;

            case Button_state::long_click:
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
        switch(state)
        {
            case Button_state::double_click:
                m_CurMode=mode::preview;
                v.SelectMenuPrevew(m_MenuInd);
            return;

            case Button_state::short_click:
                if(++m_MenuEl>m_MenuElMax)
                    m_MenuEl=m_MenuElMin;

                v.SelectMenu(m_MenuInd, m_MenuEl, m_MenuElMin, m_MenuElMax);
            return;

            case Button_state::long_click:
                ApplyMenuSetting();
            return;

            default: return;
        }
    }
}
