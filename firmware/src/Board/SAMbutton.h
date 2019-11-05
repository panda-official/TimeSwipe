/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//Panda's BOARD button

#pragma once

#include "pin_button.h"
#include "json_evsys.h"

class SAMButton: public CPinButton, public CJSONEvCP //to do add sink list inside the class!
{
protected:
        unsigned long m_nStateCounter=0; //button state counter: even - released, odd - pressed

        CButtonEvent &m_sink;

        virtual bool get_signal(void);
        virtual void send_event(typeButtonState nState);
public:
        SAMButton(CButtonEvent &sink);
};
