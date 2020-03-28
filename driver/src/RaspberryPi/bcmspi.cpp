/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "bcmspi.h"
#include <iostream>

#include "bcm2835.h"
bool CBcmLIB::m_bLibInitialized=false;
bool CBcmLIB::m_bSPIInitialized[2]={false};

CBcmLIB::CBcmLIB()
{
    if(m_bLibInitialized)
        return;

    if(!bcm2835_init())
        return;
    m_bLibInitialized=true;
}
CBcmLIB::~CBcmLIB()
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

bool CBcmLIB::init_SPI(iSPI nSPI)
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

void CBcmLIB::SPI_purge(iSPI nSPI)
{
   if(iSPI::SPI0==nSPI)
   {
       _bcm_spi_purge();
   }
}
void CBcmLIB::SPI_setCS(iSPI nSPI, bool how)
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
void CBcmLIB::SPI_waitDone(iSPI nSPI)
{
    if(iSPI::SPI0==nSPI)
    {
        while(!_bsm_spi_is_done()){}
    }
}
typeSChar CBcmLIB::SPItransfer(iSPI nSPI, typeSChar ch)
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

void CBcmLIB::SPI_set_speed_hz(iSPI nSPI, uint32_t speed_hz)
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
    typeSChar ch=0;
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
bool CBcmSPI::send(typeSChar ch)
{
    return false;
}
bool CBcmSPI::receive(typeSChar &ch)
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
