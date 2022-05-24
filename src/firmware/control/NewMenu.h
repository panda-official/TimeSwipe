/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "../button.hpp"

/**
 * @brief The Menu v2.0 controller.
 *
 * @details Another feature is a data visualization allowing to display the
 * measured signal levels by using LEDs when an user is not interacting with the
 * board (default View mode). This feature is implemented in the class `CDataVis`.
 *
 * @see CDataVis.
 */
class CNewMenu final : public Button_event {
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
    void handle_state(Button_state state) override;

    /*!
    * \brief The class constructor
    */
    CNewMenu();

private:

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
