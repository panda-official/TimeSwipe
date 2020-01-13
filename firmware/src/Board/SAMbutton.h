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

class SAMButton: public CPinButton, public CJSONEvCP
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
        CButtonEvent &m_sink;

        /*!
         * \brief Obtains a button's pin state
         * \return true if the button is pressed, false - if released
         */
        virtual bool get_signal(void);

        /*!
         * \brief Generates both CButtonEvent::OnButtonState and IJSONEvent::on_event event callbacks
         * \param nState the current button state
         */
        virtual void send_event(typeButtonState nState);
public:

        /*!
         * \brief The class constructor. The implementation contains the button PINs function setup
         * \param sink A reference to a CButtonEvent events listener
         */
        SAMButton(CButtonEvent &sink);
};
