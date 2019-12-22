/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

/*!
*   \file
*   \brief A definition file for
*   CMenuLogic
*/

#include "timer.h"
#include "button.h"
#include "nodeLED.h"
#include "nodeControl.h"
#include "json_evsys.h"

/*!
 * \brief A list of possible menus:
 */
enum class typeMenu{none, gain, bridge, setzero};


/*!
 * \brief A class for handling menu logic
 * \details The class is designed in event-driven style and forced by events coming from a button ( SAMButton ) and timer.
 */
class CMenuLogic : public CTimerEvent, public CButtonEvent,  public IJSONEvent, public CJSONEvCP
{
public:
        //! A background color for the gain menu
        static constexpr typeLEDcol  GAIN_COLOR					=LEDrgb(10, 0, 0);
        //! A current gain selection color
        static constexpr typeLEDcol  GAIN_COLOR_ACTIVE 			=LEDrgb(255, 0, 0);
        //! A background color for the bridge menu
        static constexpr typeLEDcol  BRIDGEVOLTAGE_COLOR 		=LEDrgb(0, 10, 0);
        //! A current bridge voltage selection color
        static constexpr typeLEDcol  BRIDGEVOLTAGE_COLOR_ACTIVE =LEDrgb(0, 255, 0);
        //! A background color for finding amplifier offsets ("set zero") menu
        static constexpr typeLEDcol  SETZERO_COLOR 				=LEDrgb(0, 0, 10);
        //! An active color for running finding amplifier offsets routine
        static constexpr typeLEDcol  SETZERO_COLOR_ACTIVE 		=LEDrgb(0, 0, 255);
        //! An "reset" color
        static constexpr typeLEDcol  RESET_COLOR 				=LEDrgb(255, 255, 0);
        //! \deprecated Currently not used
        static constexpr typeLEDcol  RECORD_COLOR 				=LEDrgb(0, 10, 10);
	
public:
     /*!
     * \brief A handler for button events
     * \param nState current button state
     */
	virtual void OnButtonState(typeButtonState nState);

     /*!
     * \brief A handler for timer events
     * \param nId A timer ID
     */
	virtual void OnTimer(int nId);

     /*!
     * \brief A handler for JSON events
     * \param key An object string name (key)
     * \param val A JSON event
     */
    virtual void on_event(const char *key, nlohmann::json &val);
	
     /*!
     * \brief The class constructor
     */
	CMenuLogic();
	

protected:
    //! Current selected menu
	typeMenu m_CurrentMenu;
	
    //! A timer events counter (used to force changing of menus in a preview mode)
	int		 m_nTimerCnt;

    //! A last button state ("pressed" or "released")
	typeButtonState m_LastButtonState;

    //! Is menu in the preview mode? (The preview mode or selection mode is a mode when the available menus
    //!  are listed sequentially while user is holding a button)
	bool	 m_bPreview;
	
    /*!
     * \brief Select desired menu (the LEDs state will be changed automatically)
     * \param menu A menu to select
     * \param bPreview Use preview mode or not?  (The preview mode or selection mode is a mode when the available menus
     *  are listed sequentially while user is holding a button)
     */
	void select_menu(typeMenu menu, bool bPreview=true);

    /*!
     * \brief Updates menu view (LEDs) according to the menu param
     * \param menu A menu to update
     */
	void update_menu(typeMenu menu);
};
