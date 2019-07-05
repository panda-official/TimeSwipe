//SAM's regular SPI:
#pragma once

#include "SamSPI.h"

class CSamSPIsc7 : public CSamSPI
{
public:
        CSamSPIsc7(bool bMaster=false); //ctor, bus init
//        virtual ~CSamSPIsc7(){} //just to keep polymorphic behaviour, should be never called

        virtual void chip_select(bool how);
	
	//serial:
        /*virtual bool send(CFIFO &msg);
	virtual bool receive(CFIFO &msg);
	
	//specific:
	void set_phpol(bool bPhase, bool bPol);
	void set_baud_div(unsigned char div);
        void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel);*/
};


