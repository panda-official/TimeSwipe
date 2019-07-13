/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#pragma once


#include "json_evsys.h"

class CMasterDetect :  public CJSONEvCP
{
protected:
    unsigned long m_PinChangeTStamp_mS;
    unsigned long m_ActivityTmt_mS=1000;
    bool          m_LastPinState;
    bool          m_bLastAliveState=true;

    bool          GetPinState();

public:
    CMasterDetect(); //ctor

    bool IsMasterAlive();

    void Update();

};

/*#ifndef MASTERDETECT_H
#define MASTERDETECT_H

#endif // MASTERDETECT_H*/
