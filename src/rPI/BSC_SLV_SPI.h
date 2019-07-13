/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/


//#ifndef BCS_SLV_SPI_H
//#define BCS_SLV_SPI_H

#pragma once

#include "bcmspi.h"
class CBSCslaveSPI : public CBcmLIB
{
protected:
    bool m_Initialized=false;
    CFIFO m_recFIFO;
    CSyncSerComFSM m_ComCntr;

	unsigned long m_LastChRecTime_mS;
    void check_rx(); //polling

public:
    CBSCslaveSPI();
    virtual ~CBSCslaveSPI(); //virtual just to keep polimorphic...

    bool is_initialzed(){ return m_Initialized; }

    virtual bool send(CFIFO &msg);
    virtual bool receive(CFIFO &msg);
    virtual bool send(typeSChar ch);
    virtual bool receive(typeSChar &ch);

    virtual void set_phpol(bool bPhase, bool bPol);
    virtual void set_baud_div(unsigned char div);
    virtual void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel);
};

//#endif // BCS_SLV_SPI_H
