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
