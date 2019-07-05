#pragma once

#include "timer.h"
#include "button.h"
#include "nodeLED.h"
#include "nodeControl.h"
#include "json_evsys.h"

enum class typeMenu{none, gain, bridge, setzero };


class CMenuLogic : public CTimerEvent, public CButtonEvent, public CJSONEvCP
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
