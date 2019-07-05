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
