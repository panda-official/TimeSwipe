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
//#include "nodeControl.h"
#include "json_evsys.h"


class CButtonLogic : public CTimerEvent, public CButtonEvent,  public IJSONEvent, public CJSONEvCP
{
public:
        static constexpr typeLEDcol  MAIN_COLOR					=LEDrgb(0x32, 0x97, 0xF7);
        static constexpr typeLEDcol  RECORDING_COLOR            		=LEDrgb(0xFF, 0x40, 0x81);
	
public:	//events:
	virtual void OnButtonState(typeButtonState nState);
        virtual void OnTimer(int nId){}
        virtual void on_event(const char *key, nlohmann::json &val){} //15.07.2019 now can rec an event
	
	//def ctor:
        CButtonLogic();
	

protected:
	typeButtonState m_LastButtonState;
        bool bRecording=false;
};
