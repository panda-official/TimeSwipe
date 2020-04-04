/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "button.h"
class CNewMenu  : public CButtonEvent
{
public:
    enum mode{

        def,
        preview,
        inside_menu
    };

    /*!
    * \brief A handler for button events
    * \param nState current button state
    */
    virtual void OnButtonState(typeButtonState nState);

    /*!
    * \brief The class constructor
    */
    CNewMenu();

protected:
    mode m_CurMode=mode::def;

    unsigned int m_MenuInd=0;
    unsigned int m_MenuEl=0;
    unsigned int m_MenuElMin;
    unsigned int m_MenuElMax;

    void ObtainMenuElRange();
    void ApplyMenuSetting();

};
