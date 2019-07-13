/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

//Panda's BOARD button

#pragma once

#include "pin_button.h"
#include "json_evsys.h"

class SAMButton: public CPinButton, public CJSONEvCP //to do add sink list inside the class!
{
protected:
        CButtonEvent &m_sink;

        virtual bool get_signal(void);
        virtual void send_event(typeButtonState nState);
public:
        SAMButton(CButtonEvent &sink);
};
