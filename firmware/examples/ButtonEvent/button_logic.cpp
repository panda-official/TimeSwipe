/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include "button_logic.h"

CButtonLogic::CButtonLogic()
{
    m_LastButtonState=typeButtonState::released;
    nodeLED::setMultipleLED(typeLED::LED1, typeLED::LED4, MAIN_COLOR);
}
void CButtonLogic::OnButtonState(typeButtonState nState)
{	
	m_LastButtonState=nState;
	if(typeButtonState::released==nState)
	{
            //toggle state:
            bRecording=!bRecording;
            nodeLED::setMultipleLED(typeLED::LED1, typeLED::LED4, bRecording ? RECORDING_COLOR:MAIN_COLOR);
	}
}
