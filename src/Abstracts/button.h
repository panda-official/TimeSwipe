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
