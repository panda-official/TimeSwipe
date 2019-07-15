/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

#include "timer.h"
#include "button.h"
#include "nodeLED.h"
#include "nodeControl.h"
#include "json_evsys.h"

enum class typeMenu{none, gain, bridge, setzero };


class CMenuLogic : public CTimerEvent, public CButtonEvent,  public IJSONEvent, public CJSONEvCP
{
public:
        static constexpr typeLEDcol  GAIN_COLOR					=LEDrgb(10, 0, 0);
	static constexpr typeLEDcol  GAIN_COLOR_ACTIVE 				=LEDrgb(255, 0, 0);
	static constexpr typeLEDcol  BRIDGEVOLTAGE_COLOR 			=LEDrgb(0, 10, 0);
        static constexpr typeLEDcol  BRIDGEVOLTAGE_COLOR_ACTIVE                 =LEDrgb(0, 255, 0);
        static constexpr typeLEDcol  SETZERO_COLOR 				=LEDrgb(0, 0, 10);
	static constexpr typeLEDcol  SETZERO_COLOR_ACTIVE 			=LEDrgb(0, 0, 255);
        static constexpr typeLEDcol  RESET_COLOR 				=LEDrgb(255, 255, 0);
        static constexpr typeLEDcol  RECORD_COLOR 				=LEDrgb(0, 10, 10);
	
public:	//events:
	virtual void OnButtonState(typeButtonState nState);
	virtual void OnTimer(int nId);
        virtual void on_event(const char *key, nlohmann::json &val); //15.07.2019 now can rec an event
	
	//def ctor:
	CMenuLogic();
	

protected:
	typeMenu m_CurrentMenu;
	
	int		 m_nTimerCnt;
	typeButtonState m_LastButtonState;
	//bool	 m_bIgnoreBPress;
	bool	 m_bPreview;
	
	
	void select_menu(typeMenu menu, bool bPreview=true);
	void update_menu(typeMenu menu);
};
