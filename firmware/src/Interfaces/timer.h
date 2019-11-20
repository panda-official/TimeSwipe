/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
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
