/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//a pin-button with a first order filter to prevent bouncing

#pragma once
#include "os.h"
#include "button.h"

class CPinButton{
protected:

	//params:
	float m_low_trhold=0.2f;
	float m_high_trhold=0.8f;
	float m_filter_t_Sec=0.018f;
	
	//variables:
	float m_level=0.0f;
    unsigned long m_last_time_upd=os::get_tick_mS(); //0;
	unsigned long m_upd_quant=10;
	
	typeButtonState m_cur_state		=typeButtonState::released;
	typeButtonState m_prev_state	=typeButtonState::released;

	virtual bool get_signal(void)=0;
	virtual void send_event(typeButtonState nState)=0;
	
public:
	void update(void)
	{	
        unsigned long elapsed=os::get_tick_mS()-m_last_time_upd;
		if(elapsed<m_upd_quant)
			return;
			
        m_last_time_upd=os::get_tick_mS();
			
		m_level+=( (get_signal() ? 1.0f:0.0f) - m_level ) * ( (float)elapsed ) / (m_filter_t_Sec*1000.0f); //filtering
		if(m_level>=m_high_trhold)
		{
			m_cur_state=typeButtonState::pressed;
		}
		else if(m_level<=m_low_trhold)
		{
			m_cur_state=typeButtonState::released;
		}	
		if(m_prev_state!=m_cur_state)
		{
			m_prev_state=m_cur_state;
			send_event(m_cur_state);
		}
	}
};
