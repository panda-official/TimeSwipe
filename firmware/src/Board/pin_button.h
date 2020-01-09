/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

/*!
*   \file
*   \brief A definition file for an implementation  template of a button which uses digital pin state as an input signal with a debouncing code
*   CPinButton
*
*/

#include "os.h"
#include "button.h"

/*!
 * \brief An implementation template of a button which uses digital pin state as an input signal with a debouncing code
 * \details A raw signal is acquired via pure virtual function bool get_signal(void) which must be overridden
 * in the derived class. To remove the signal noise (debouncing) a simple 1st order digital filter is used.
 * When filtered signal level drops down below m_low_trhold the "released" state is established.
 * When filtered signal level  exceeds m_high_trhold the "pressed" state is established.
 * The generation of the corresponding event is delegated to the derived class -
 * it must provide an overridden send_event(typeButtonState nState) virtual function
 */
class CPinButton{
protected:

    /*!
     * \brief Low threshold level of a filtered signal level to detect "released" state
     */
	float m_low_trhold=0.2f;

    /*!
     * \brief High threshold level of a filtered signal level to detect "pressed" state
     */
	float m_high_trhold=0.8f;

    /*!
     * \brief 1st order digital filter time constant, seconds
     */
	float m_filter_t_Sec=0.018f;
	
    /*!
     * \brief A filtered signal level
     */
	float m_level=0.0f;

    /*!
     * \brief Last time when update() method was called, mSec
     */
    unsigned long m_last_time_upd=os::get_tick_mS();

    /*!
     * \brief Minimum time between two consecutive updates
     */
	unsigned long m_upd_quant=10;
	
    /*!
     * \brief Current state of the button
     */
	typeButtonState m_cur_state		=typeButtonState::released;

    /*!
     * \brief Previous state of the button. Used to generate an event by compare with m_cur_state
     */
	typeButtonState m_prev_state	=typeButtonState::released;

    /*!
     * \brief Acquires a raw signal level from a pin. The function must be implemented in derived class
     * \return The signal level: true=pressed, false=released.
     */
	virtual bool get_signal(void)=0;

    /*!
     * \brief Generates a button event. The function must be implemented in derived class
     * \param nState The button state: pressed=typeButtonState::pressed or released=typeButtonState::released
     */
	virtual void send_event(typeButtonState nState)=0;
	
public:

    /*!
     * \brief The button state update method
     * \details
     */
	void update(void)
	{	
        //! check if minimum updation time has elapsed since last updation
        unsigned long elapsed=os::get_tick_mS()-m_last_time_upd;
		if(elapsed<m_upd_quant)
			return;
			
        //! if yes, set new updation time stamp
        m_last_time_upd=os::get_tick_mS();
			
        //! get the signal level and apply a 1-st order digital filter
        m_level+=( (get_signal() ? 1.0f:0.0f) - m_level ) * ( (float)elapsed ) / (m_filter_t_Sec*1000.0f);

        //! determine the button state based on filtered signal level
		if(m_level>=m_high_trhold)
		{
			m_cur_state=typeButtonState::pressed;
		}
		else if(m_level<=m_low_trhold)
		{
			m_cur_state=typeButtonState::released;
		}	

        //! if the state differs from previous state, generate a button event
		if(m_prev_state!=m_cur_state)
		{
			m_prev_state=m_cur_state;
			send_event(m_cur_state);
		}
	}
};
