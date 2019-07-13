/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/


//SAM QSPI abstraction:

#pragma once

//#include "Serial.h"
#include "SPI.h"

class CSamQSPI : public CSPI
{
public:
	CSamQSPI(); //ctor, bus init
	
	//serial:
	virtual bool send(CFIFO &msg);
	virtual bool receive(CFIFO &msg);
        virtual bool send(typeSChar ch);
        virtual bool receive(typeSChar &ch);
	
	//specific:
	void set_phpol(bool bPhase, bool bPol);
	void set_baud_div(unsigned char div);
	void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel);

        virtual ~CSamQSPI(){} //just to keep polymorphic behaviour, should be never called
};
