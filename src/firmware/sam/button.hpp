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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SAM_BUTTON_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SAM_BUTTON_HPP

#include "../pin_button.hpp"
#include "json/json_evsys.h"

/**
 * @brief Panda board button.
 *
 * @details Emits JSON event according to the button state.
 */
class Sam_button final : public Pin_button<Sam_button>, public CJSONEvCP {
public:
        void AdviseSink(const std::shared_ptr<Button_event> &sink)
        {
            m_pSink=sink;
        }

        /*!
         * \brief Obtains a button's pin state
         * \return true if the button is pressed, false - if released
         */
        bool impl_get_signal(void);

        /*!
         * \brief Generates both Button_event::OnButtonState and IJSONEvent::on_event event callbacks
         * \param nState the current button state
         */
        void impl_on_state_changed(Button_state state);

        /*!
         * \brief Turns buttol LED on/off
         * \param how - true=LED ON, false=LED off
         */
        void TurnButtonLED(bool how);

        /*!
         * \brief Returns actual state of the button LED
         * \return true=LED ON, false=LED off
         */
        bool IsButtonLEDon();

        /*!
         * \brief Toggles the button LED (ON->OFF, OFF->ON)
         */
        void ToggleButtonLED(){

            TurnButtonLED(!IsButtonLEDon());
        }

        /*!
         * \brief The current view access interface
         * \return The current view
         */
        static Sam_button& Instance()
        {
           static Sam_button singleton;
           return singleton;
        }

private:
        /*!
         * \brief The button state counter: even - released, odd - pressed.
         *  Incremented on every button event
         */
        unsigned long m_nStateCounter=0;

        /*!
         * \brief A single sink to Button_event
         * \deprecated Initialy it was used to subscribe a menu object for button events
         * \todo Use only IJSONEvent instead?
         */
        std::shared_ptr<Button_event> m_pSink;

        /*!
         * \brief Private class constructor
         */
        Sam_button();

        //! Forbid copy constructor
        Sam_button(const Sam_button&)=delete;

        //! Forbid copying
        Sam_button& operator=(const Sam_button&)=delete;

};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SAM_BUTTON_HPP
