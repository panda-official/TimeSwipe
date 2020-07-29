/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//Panda's BOARD button

#pragma once

/*!
*   \file
*   \brief A definition file for
*   SAMButton
*
*/

#include "pin_button.h"
#include "json_evsys.h"

/*!
 * \brief A hardware-dependent realization of the board's button with ability of generation a JSON event from the button state
 */

class SAMButton: public CPinButton<SAMButton>, public CJSONEvCP
{
protected:

        /*!
         * \brief The button state counter: even - released, odd - pressed.
         *  Incremented on every button event
         */
        unsigned long m_nStateCounter=0;

        /*!
         * \brief A single sink to CButtonEvent
         * \deprecated Initialy it was used to subscribe a menu object for button events
         * \todo Use only IJSONEvent instead?
         */
        std::shared_ptr<CButtonEvent> m_pSink;

public:

        void AdviseSink(const std::shared_ptr<CButtonEvent> &sink)
        {
            m_pSink=sink;
        }

        /*!
         * \brief Obtains a button's pin state
         * \return true if the button is pressed, false - if released
         */
        bool impl_get_signal(void);

        /*!
         * \brief Generates both CButtonEvent::OnButtonState and IJSONEvent::on_event event callbacks
         * \param nState the current button state
         */
        void impl_on_state_changed(typeButtonState nState);

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
         * \brief The current view access interface
         * \return The current view
         */
        static SAMButton& Instance()
        {
           static SAMButton singleton;
           return singleton;
        }

private:
        /*!
         * \brief Private class constructor
         */
        SAMButton();

        //! Forbid copy constructor
        SAMButton(const SAMButton&)=delete;

        //! Forbid copying
        SAMButton& operator=(const SAMButton&)=delete;
};
