/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#ifdef EMU
#include <iostream>
#endif
#include "menu_logic.h"
#include <string.h>

CMenuLogic::CMenuLogic()
{
	m_CurrentMenu=typeMenu::none;
	m_nTimerCnt=0;
	m_bPreview=false;
    m_LastButtonState=typeButtonState::released;
}

//helpers:
void CMenuLogic::update_menu(typeMenu menu)
{
	switch(menu)
	{			
		case typeMenu::gain:
		{
            nodeLED::selectLED(static_cast<typeLED>(nodeControl::GetGain()), GAIN_COLOR_ACTIVE, typeLED::LED1, typeLED::LED4, GAIN_COLOR);
		}
		break;
		
		case typeMenu::bridge:
		{
            nodeLED::selectLED(nodeControl::GetBridge() ? typeLED::LED2:typeLED::LED1, BRIDGEVOLTAGE_COLOR_ACTIVE, typeLED::LED1, typeLED::LED2, BRIDGEVOLTAGE_COLOR);
		}
		break;
		
        case typeMenu::setzero:
		{
            nodeLED::setMultipleLED(typeLED::LED1, typeLED::LED4, SETZERO_COLOR);
        }
	}
#ifdef EMU
	std::cout<<"menu updated..."<<std::endl;
#endif

}
void CMenuLogic::select_menu(typeMenu menu, bool bPreview)
{
	m_CurrentMenu=menu;
	m_bPreview=bPreview;
#ifdef EMU
	switch(menu)
	{
		case typeMenu::none: 	std::cout<<"exiting menu"; break;
		case typeMenu::gain: 	std::cout<<"entering gain.."; break;
		case typeMenu::bridge:	std::cout<<"entering bridge"; break;
		case typeMenu::setzero: std::cout<<"entering setzero..."; break;
	}
	std::cout<<std::endl;
#endif

    //generate an event:
    nlohmann::json v=static_cast<int>(m_CurrentMenu);
    Fire_on_event("Menu", v);
	update_menu(menu);
}


//events:
void CMenuLogic::on_event(const char *key, nlohmann::json &val)
{
    if(0==strcmp("Zero", key))
    {
        select_menu(val ? typeMenu::setzero : typeMenu::none, false);
    }
}


void CMenuLogic::OnButtonState(typeButtonState nState)
{
#ifdef EMU
	std::cout<<( typeButtonState::released==nState ? "but_rel":"but_pressed")<<std::endl;
#endif
	
	m_LastButtonState=nState;
	if(typeButtonState::released==nState)
	{
		//proc event:
		m_nTimerCnt=0; //reset timer cnt 		
		if(m_bPreview)
		{
			m_bPreview=false;
			if(typeMenu::setzero!=m_CurrentMenu)
				return;
		}
	
		
		//short clicks:
		switch(m_CurrentMenu) //if in current menu
		{
			case typeMenu::none:
			{
                nodeControl::StartRecord(true);
			}
			break;
			
			case typeMenu::gain:
			{
				//increment gain:
				nodeControl::IncGain(+1);
			}
			break;
			
			case typeMenu::bridge:
			{
				//toggle bridge:
				nodeControl::SetBridge(!nodeControl::GetBridge());
				
			}
			break;
			
			case typeMenu::setzero:
			{
                nodeControl::SetZero(true); //02.05.2019
			}
			return;
		}
		update_menu(m_CurrentMenu);
	}
}
void CMenuLogic::OnTimer(int nId) //every second
{
	
	if(typeButtonState::pressed!=m_LastButtonState)
		return;
	
	
	m_nTimerCnt++;
	if(typeMenu::none!=m_CurrentMenu && !m_bPreview) //if we are inside the menu chain
	{
		if(m_nTimerCnt>=2)
		{
            nodeLED::resetALL();
			select_menu(typeMenu::none);
			m_nTimerCnt=11;
		}
		return;
	}
	
	//entering menu chain....only when record is not started
	if(!nodeControl::IsRecordStarted()) switch(m_nTimerCnt)
	{
		case 2: // 2 sec
			select_menu(typeMenu::gain);
		break;
		
		case 4: //4 sec
			select_menu(typeMenu::bridge);
		break;
		
		case 6: //6 sec
			select_menu(typeMenu::setzero);
		break;
		
		case 10: //10 sec - cancel
            nodeLED::resetALL();
			select_menu(typeMenu::none);
            nodeLED::blinkLED(typeLED::LED1, RESET_COLOR);
	}
	
}
