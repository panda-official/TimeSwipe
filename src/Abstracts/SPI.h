//abstract SPI interface:

#pragma once

#include "Serial.h"

class CSPI : public virtual ISerial{
	
public:
	virtual bool send(CFIFO &msg)=0;
	virtual bool receive(CFIFO &msg)=0;
        virtual bool send(typeSChar ch)=0;
        virtual bool receive(typeSChar &ch)=0;
	
	virtual void set_phpol(bool bPhase, bool bPol)=0;
	virtual void set_baud_div(unsigned char div)=0;
	virtual void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel)=0;

        //virtual ~CSPI()=0;
        virtual ~CSPI()=default;
};
