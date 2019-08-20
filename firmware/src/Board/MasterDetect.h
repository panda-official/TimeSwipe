/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
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
