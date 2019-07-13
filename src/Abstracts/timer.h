/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#pragma once

class CTimerEvent{
public:
	virtual void OnTimer(int nId)=0;

    //forbid coping:
    CTimerEvent()=default;
    CTimerEvent(const CTimerEvent&) = delete;
    CTimerEvent& operator=(const CTimerEvent&) = delete;
protected:
        virtual ~CTimerEvent()=default;
};
