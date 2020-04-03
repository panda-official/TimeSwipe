/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "NewMenu.h"
#include "View.h"

CNewMenu::CNewMenu()
{

}

void CNewMenu::OnButtonState(typeButtonState nState)
{
    CView &v=CView::Instance();
    if(mode::def==m_CurMode)
    {
        switch(nState)
        {
            case typeButtonState::short_click:
                v.SetRecordMarker();
            return;

            case typeButtonState::long_click:
                m_CurMode=mode::preview;
                v.SelectMenuPrevew(0);
            return;

        default: return;
        }
    }

    //double click: always return to default:
  /*  if(typeButtonState::double_click==nState)
    {
        m_CurMode=mode::def;
        //v.ExitMenu();
        return;
    }*/

    if(mode::preview==m_CurMode)
    {
        switch(nState)
        {
            case typeButtonState::double_click:
                m_CurMode=mode::def;
                v.ExitMenu();
            return;

            case typeButtonState::short_click:
                if(++m_PreviewInd>(CView::menu::total))
                    m_PreviewInd=0;

                v.SelectMenuPrevew(m_PreviewInd);
            return;

            case typeButtonState::long_click:
                if(m_PreviewInd<CView::menu::total)
                {
                     m_CurMode=mode::inside_menu;
                     v.SelectMenu(m_PreviewInd, m_MenuInd);
                }
                else
                {
                    v.ResetSettings();
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
                v.SelectMenuPrevew(m_PreviewInd);
            return;

            case typeButtonState::short_click:
                //iterate menu:
                if(++m_MenuInd>=CView::menu::total)
                    m_MenuInd=0;

                v.SelectMenu(m_PreviewInd, m_MenuInd);
            return;

            case typeButtonState::long_click:
                v.ApplyMenu();
            return;

            default: return;
        }
    }
}
