/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

//SAM's regular SPI:
#pragma once

#include "SPI.h"
#include "SamSercom.h"
#include "SyncCom.h"    //20.05.219

class CSamSPI : public CSamSercom, public CSPI
{
protected:
        bool           m_bMaster; //master/slave mode 03.06.2019
        bool           m_bIRQmode;  //IRQmode 25.06.2019
        inline bool    isIRQmode(){return m_bIRQmode;}

        //typeSamSercoms m_nSercom; -> moved to the super 24.06.2019
        std::shared_ptr<CSamCLK> m_pCLK;

        //flow control:
        bool m_bCSactive=false;
        CSyncSerComFSM m_ComCntr;
        CFIFO m_recFIFO;
        CFIFO m_recFIFOhold;    //25.06.2019 - hold the result


        //working with rx buf:
        //void check_rx();
        void IRQhandler();

        //04.06.2019:
        bool send_char(typeSChar ch);

        //03.06.2019:
        virtual void chip_select(bool how){}
	
//public:
        CSamSPI(typeSamSercoms nSercom, bool bMaster=false); //ctor, bus init
        virtual ~CSamSPI(); //just to keep polymorphic behaviour, should be never called

        virtual void OnIRQ0();
        virtual void OnIRQ1();
        virtual void OnIRQ2();
        virtual void OnIRQ3();

public:
	//serial:
	virtual bool send(CFIFO &msg);
	virtual bool receive(CFIFO &msg);
        virtual bool send(typeSChar ch);
        virtual bool receive(typeSChar &ch);
	
	//specific:
        virtual void set_phpol(bool bPhase, bool bPol);
        virtual void set_baud_div(unsigned char div);
        virtual void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel);

        //10.05.2019: updation
        void Update(); //{ check_rx(); } //+++dbg only version

        //24.06.2019:
        void EnableIRQs(bool how);
};


