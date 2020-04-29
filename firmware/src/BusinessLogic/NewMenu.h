/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CNewMenu
*/

#pragma once

#include "button.h"

/*!
 * \brief The Menu v2.0 controller
 */
class CNewMenu  : public CButtonEvent
{
public:
    /*!
     * \brief The menu modes
     */
    enum mode{

        def,            //!<default mode, data visualization is runnig
        preview,        //!<preview mode, select corresponding setting section
        inside_menu     //!<inside menu mode, change corresponding setting
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

    /*!
     * \brief The current menu mode
     */
    mode m_CurMode=mode::def;

    /*!
     * \brief The current setting index
     */
    unsigned int m_MenuInd=0;

    /*!
     * \brief The current setting value to change inside the menu (m_MenuInd)
     */
    unsigned int m_MenuEl=0;

    /*!
     * \brief The current setting lower limit
     */
    unsigned int m_MenuElMin;

    /*!
     * \brief The current setting upper limit
     */
    unsigned int m_MenuElMax;

    /*!
     * \brief Obtains current setting limits depending on current menu
     */
    void ObtainMenuElRange();

    /*!
     * \brief Applies setting for the current menu
     */
    void ApplyMenuSetting();

};
