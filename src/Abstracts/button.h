/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
