/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/


#ifdef EMU
#include <iostream>
#endif
#include "menu_logic.h"

CMenuLogic::CMenuLogic()
{
	m_CurrentMenu=typeMenu::none;
	m_nTimerCnt=0;
	m_bPreview=false;
    m_LastButtonState=typeButtonState::released; //17.05.2019!!!!
}

//helpers:
void CMenuLogic::update_menu(typeMenu menu)
{
	switch(menu)
	{
       /* case typeMenu::none:
		{
            if(nodeControl::IsRecordStarted())
                nodeLED::setMultipleLED(typeLED::LED1, typeLED::LED4, RECORD_COLOR);
			else
                nodeLED::resetALL();

            nodeLED::resetALL();

		}
        break;*/
				
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

    //19.06.2019: generate an event:
    nlohmann::json v=static_cast<int>(m_CurrentMenu);
    Fire_on_event("Menu", v);

/*    if(typeMenu::none==menu)                      //removed: 25.06.2019 (it is not inf now)
    {
        nodeControl::SetZero(false); //stop cal proc
    }*/
	update_menu(menu);
}


//events:
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
                //nodeControl::StartRecord(!nodeControl::IsRecordStarted());
                nodeControl::StartRecord(true);
                //nodeLED::blinkMultipleLED(typeLED::LED1, typeLED::LED4, rstamp, 3, 300);
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
                //nodeLED::blinkMultipleLED(1, 4, SETZERO_COLOR_ACTIVE, 2, 200);
                //select_menu(typeMenu::none, false); //no preview!
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
