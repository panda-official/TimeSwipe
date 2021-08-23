// -*- C++ -*-

// PANDA TimeSwipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "spi.hpp"
#include "../3rdparty/BCMsrc/bcm2835.h"

#include <iostream>

bool BcmLib::m_bLibInitialized=false;
bool BcmLib::m_bSPIInitialized[2]={false};

BcmLib::BcmLib()
{
    if(m_bLibInitialized)
        return;

    if(!bcm2835_init())
        return;
    m_bLibInitialized=true;
}
BcmLib::~BcmLib()
{
    if(m_bSPIInitialized[iSPI::SPI0])
    {
        bcm2835_spi_end();
    }
    if(m_bSPIInitialized[iSPI::SPI1])
    {
        bcm2835_aux_spi_end();
    }
    if(m_bLibInitialized)
    {
        bcm2835_close();
    }
}

bool BcmLib::init_SPI(iSPI nSPI)
{
    if(m_bSPIInitialized[nSPI])
        return true;

    bool bRes;
    if(iSPI::SPI0==nSPI)
    {
        bRes=bcm2835_spi_begin() ? true:false;
    }
    else
    {
        bRes=bcm2835_aux_spi_begin() ? true:false;
    }
    m_bSPIInitialized[nSPI]=bRes;
    return bRes;
}

void BcmLib::SPI_purge(iSPI nSPI)
{
   if(iSPI::SPI0==nSPI)
   {
       _bcm_spi_purge();
   }
}
void BcmLib::SPI_setCS(iSPI nSPI, bool how)
{
    if(iSPI::SPI0==nSPI)
    {
        _bsm_spi_cs( how ? 1:0);
    }
    else
    {
        char t=0, r;
        _bcm_aux_spi_transfernb(&t, &r, 1, how ? 1:0);
    }
}
void BcmLib::SPI_waitDone(iSPI nSPI)
{
    if(iSPI::SPI0==nSPI)
    {
        while(!_bsm_spi_is_done()){}
    }
}
Character BcmLib::SPItransfer(iSPI nSPI, Character ch)
{
    if(iSPI::SPI0==nSPI)
    {
        _bcm_spi_send_char(ch);
        return _bcm_spi_rec_char();
    }
    else
    {
        char t=ch;
        char r;
         _bcm_aux_spi_transfernb(&t, &r, 1, 1);
         return r;
    }
}

void BcmLib::SPI_set_speed_hz(iSPI nSPI, uint32_t speed_hz)
{
    if(iSPI::SPI0==nSPI)
    {
        bcm2835_spi_set_speed_hz(speed_hz);
    }
    else
    {
        bcm2835_aux_spi_setClockDivider(bcm2835_aux_spi_CalcClockDivider(speed_hz));
    }
}




CBcmSPI::CBcmSPI(iSPI nSPI)
{
    m_nSPI=nSPI;
    if(!init_SPI(nSPI))
        return;


    //set default rate:
    //SPI_set_speed_hz(100000);
    SPI_set_speed_hz(50000);

}
CBcmSPI::~CBcmSPI()
{
}

bool CBcmSPI::send(CFIFO &msg)
{
    if(!is_initialzed())
        return false;

    SPI_purge();
    SPI_setCS(true);
    m_recFIFO.reset();

    //a delay is required for CS to fall:
    bcm2835_delay(20); //corresponding to 50KHz

    //flow control:
    Character ch=0;
    m_ComCntr.start(CSyncSerComFSM::FSM::sendLengthMSB);
    while(m_ComCntr.proc(ch, msg))
    {
       SPItransfer(ch);
    }
    if(m_ComCntr.bad()) return false;

    SPItransfer(0); //provide add clock...


    //waiting for a "done" state:
    SPI_waitDone();


    m_ComCntr.start(CSyncSerComFSM::FSM::recSilenceFrame);
    do
    {
        ch=SPItransfer(0); //provide a clock
    }
    while(m_ComCntr.proc(ch, m_recFIFO));


    SPI_setCS(false);
    //a delay is required for CS to rise:
    bcm2835_delay(20); //corresponding to 50KHz


    return true;
}
bool CBcmSPI::receive(CFIFO &msg)
{
    if(!is_initialzed())
        return false;

    msg=m_recFIFO;
    return  (m_ComCntr.get_state()==CSyncSerComFSM::FSM::recOK);
}
bool CBcmSPI::send(Character ch)
{
    return false;
}
bool CBcmSPI::receive(Character &ch)
{
    return false;
}

void CBcmSPI::set_phpol(bool bPhase, bool bPol)
{

}
void CBcmSPI::set_baud_div(unsigned char div)
{

}
void CBcmSPI::set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel)
{

}
