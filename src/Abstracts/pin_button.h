//a pin-button with a first order filter to prevent bouncing

#pragma once
#include "button.h"
//#include <iostream>

unsigned long get_tick_mS(void);
class CPinButton{
protected:

	//params:
	float m_low_trhold=0.2f;
	float m_high_trhold=0.8f;
	float m_filter_t_Sec=0.08f;
	
	//variables:
	float m_level=0.0f;
	unsigned long m_last_time_upd=get_tick_mS(); //0;
	unsigned long m_upd_quant=10;
	
	typeButtonState m_cur_state		=typeButtonState::released;
	typeButtonState m_prev_state	=typeButtonState::released;

	virtual bool get_signal(void)=0;
	virtual void send_event(typeButtonState nState)=0;
	
public:
	void update(void)
	{
		//if(0==m_last_time_upd) {m_last_time_upd=get_tick_mS(); return;}
		
		unsigned long elapsed=get_tick_mS()-m_last_time_upd;
		if(elapsed<m_upd_quant)
			return;
			
		m_last_time_upd=get_tick_mS(); //!!!
			
		m_level+=( (get_signal() ? 1.0f:0.0f) - m_level ) * ( (float)elapsed ) / (m_filter_t_Sec*1000.0f); //filtering
		if(m_level>=m_high_trhold)
		{
			m_cur_state=typeButtonState::pressed;
		}
		else if(m_level<=m_low_trhold)
		{
			m_cur_state=typeButtonState::released;
		}
			
		//std::cout<<m_level<<std::endl;
			
		if(m_prev_state!=m_cur_state)
		{
			m_prev_state=m_cur_state;
			send_event(m_cur_state);
			
			//std::cout<<(typeButtonState::pressed==m_cur_state ? "1":"0")<<std::endl;
		}
	}
};
