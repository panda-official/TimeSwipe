/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

enum class typeButtonState{ pressed, released };

class CButtonEvent{
public:	
	virtual void OnButtonState(typeButtonState nState)=0;

    //forbid coping:
    CButtonEvent()=default;
    CButtonEvent(const CButtonEvent&) = delete;
    CButtonEvent& operator=(const CButtonEvent&) = delete;
protected:
        virtual ~CButtonEvent()=default;
};
