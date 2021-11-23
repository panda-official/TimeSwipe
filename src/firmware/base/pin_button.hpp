// -*- C++ -*-

// PANDA TimeSwipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PANDA_TIMESWIPE_FIRMWARE_BASE_PIN_BUTTON_HPP
#define PANDA_TIMESWIPE_FIRMWARE_BASE_PIN_BUTTON_HPP

#include "../button.hpp"
#include "../os.h"

/**
 * @brief The button which uses digital pin state as an input signal with a
 * debouncing code.
 *
 * @details This class follows the CRTP design pattern. The derived class T must
 * provide:
 *   -# impl_get_signal() which returns a value of type `bool`. This function is
 *   called to acquire a raw signal;
 *   -# impl_on_state_changed(Button_state) which returns `void`. This function
 *   is called to emit the corresponding button event.
 * To remove the signal noise (debouncing) a simple 1-order digital filter is
 * used. When filtered signal level drops down below m_low_trhold the "released"
 * state is established. When filtered signal level exceeds m_high_trhold the
 * "pressed" state is established.
 *
 * @see Button_event.
 */
template <class T>
class Pin_button {
public:
  /// Updates the button state.
  void update()
  {
    // Return if min update interval doesn't reached.
    const unsigned long elapsed = os::get_tick_mS() - m_last_time_upd;
    if (elapsed < m_upd_quant)
      return;
    else
      m_last_time_upd = os::get_tick_mS();

    // Get the signal level and apply 1-order digital filter.
    m_level += (get_signal() - m_level) * elapsed * m_filter_factor;

    // Determine the button state based on filtered signal level.
    if (m_level >= m_high_trhold)
      m_cur_state = Button_state::pressed;
    else if (m_level <= m_low_trhold)
      m_cur_state = Button_state::released;

    if (m_prev_state == Button_state::pressed) {
      const unsigned long pressing_time = os::get_tick_mS() - m_press_time_stamp_mS;
      if (!m_bLongClickIsSet) {
        if (pressing_time > m_short_click_max_duration_mS) {
          m_bFirstClickOfDouble = false;
          m_bLongClickIsSet = true;
          on_state_changed(Button_state::long_click);
        }
      }
      if (!m_bVeryLongClickIsSet) {
        if (pressing_time > m_very_long_click_duration_mS) {
          m_bFirstClickOfDouble = false;
          m_bVeryLongClickIsSet = true;
          on_state_changed(Button_state::very_long_click);
        }
      }
    } else if (m_bFirstClickOfDouble) {
      if ((os::get_tick_mS() - m_release_time_stamp_mS) > m_double_click_trhold_mS) {
        m_bFirstClickOfDouble = false;
        on_state_changed(Button_state::short_click);
      }
    }

    // Emit Button_event if the state changed.
    if (m_prev_state != m_cur_state) {
      if (m_cur_state == Button_state::pressed) {
        m_press_time_stamp_mS = os::get_tick_mS();
        m_interclick_time_span_mS = m_press_time_stamp_mS - m_release_time_stamp_mS;
      } else {
        m_bLongClickIsSet = false;
        m_bVeryLongClickIsSet = false;

        m_release_time_stamp_mS = os::get_tick_mS();
        m_click_duration_mS = m_release_time_stamp_mS - m_press_time_stamp_mS;

        // Click.
        if (m_click_duration_mS < m_short_click_max_duration_mS) {
          if (m_click_duration_mS < m_double_click_trhold_mS) {
            if (m_bFirstClickOfDouble) {
              if (m_interclick_time_span_mS < m_double_click_trhold_mS)
                on_state_changed(Button_state::double_click);
              m_bFirstClickOfDouble = false;
            } else
              m_bFirstClickOfDouble = true;
          } else {
            on_state_changed(Button_state::short_click);
            m_bFirstClickOfDouble = false;
          }
        } else
          m_bFirstClickOfDouble = false;
      }
      on_state_changed(m_cur_state);
      m_prev_state=m_cur_state;
    }
  }

private:
    /*!
     * \brief Low threshold level of a filtered signal level to detect "released" state
     */
    float m_low_trhold=0.05f;

    /*!
     * \brief High threshold level of a filtered signal level to detect "pressed" state
     */
    float m_high_trhold=0.95f;

    /*!
     * \brief 1st order digital filter pre-calculated factor
     */
    float m_filter_factor=1.0f/(0.013f*1000.0f);

    /*!
     * \brief A filtered signal level
     */
    float m_level=0.0f;

    /*!
     * \brief Last time when update() method was called, mSec
     */
    unsigned long m_last_time_upd=os::get_tick_mS();

    /*!
     * \brief The timestamp of last button press, mSec
     */
    unsigned long m_press_time_stamp_mS;

    /*!
     * \brief The timestamp of last button release, mSec
     */
    unsigned long m_release_time_stamp_mS;

    /*!
     * \brief The current click (press/release) duration, mSec
     */
    unsigned long m_click_duration_mS;

    /*!
     * \brief The time interval between two consecutive clicks
     */
    unsigned long m_interclick_time_span_mS;

    /*!
     * \brief The first short click of double-click detection flag
     */
    bool m_bFirstClickOfDouble=false;

    /*!
     * \brief "Long Click" was set
     */
    bool m_bLongClickIsSet=false;

    /*!
     * \brief "Very Long Click" was set
     */
    bool m_bVeryLongClickIsSet=false;

    /*!
     * \brief The maximum duration of the Short Click (minimum duration of the Long Click)
     */
    unsigned long m_short_click_max_duration_mS=1200;

    /*!
     * \brief The Double Click maximum duration (single click + interclick time span)
     */
    unsigned long m_double_click_trhold_mS=400;

    /*!
     * \brief The Very Long Click minimum duration, mSec
     */
    unsigned long m_very_long_click_duration_mS=6000;

    /*!
     * \brief Minimum time between two consecutive updates
     */
    unsigned long m_upd_quant=4;

    /*!
     * \brief Current state of the button
     */
    Button_state m_cur_state		=Button_state::released;

    /*!
     * \brief Previous state of the button. Used to generate an event by compare with m_cur_state
     */
    Button_state m_prev_state	=Button_state::released;

    /*!
     * \brief Acquires a raw signal level from a pin. The function must be implemented in derived class
     * \return The signal level: true=pressed, false=released.
     */
    bool get_signal()
    {
        return static_cast<T*>(this)->impl_get_signal();
    }

    /*!
     * \brief Generates a button event. The function must be implemented in derived class
     * \param state The button state: pressed=Button_state::pressed or released=Button_state::released
     */
    void on_state_changed(const Button_state state)
    {
        static_cast<T*>(this)->impl_on_state_changed(state);
    }
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_BASE_PIN_BUTTON_HPP
