/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/


//SAM's regular SPI:

#include "SamSPI.h"
#include "sam.h"

Sercom *glob_GetSercomPtr(typeSamSercoms nSercom);
#define SELECT_SAMSPI(nSercom) &(glob_GetSercomPtr(nSercom)->SPI)

void Wait(unsigned long time_mS);
unsigned long get_tick_mS(void);

CSamSPI::CSamSPI(typeSamSercoms nSercom, bool bMaster) : CSamSercom(nSercom)
{
    //m_nSercom=nSercom;
    m_bMaster=bMaster;
    m_bIRQmode=false;
	
	//select ptr:
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);
	
	
	//---------------enable main clock to drive SPI bus------------------
    CSamSercom::EnableSercomBus(nSercom, true);


    //if constructed as a master, need to connect a clock source: 03.06.2019
    if(bMaster)
    {
        m_pCLK=CSamCLK::Factory();

        //connect:
        ConnectGCLK(m_nSercom, m_pCLK->CLKind());

        //m_pCLK->SetDiv(8); //???
        m_pCLK->Enable(true);

       // pSPI->CTRLB.bit.MSSEN=1; //auto CS
        pSPI->BAUD.bit.BAUD=0xff; //lowest possible by default
    }

    pSPI->CTRLA.bit.MODE=bMaster ? 0x03: 0x02;
	//-------------------------------------------------------------------
}
CSamSPI::~CSamSPI() //24.06.2019
{
    EnableIRQs(false); //disable all IRQs to prevent possible crash from unhandled IRQ
}

//24.06.2019: IRQ handlers:
void CSamSPI::IRQhandler()
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);
    if(pSPI->INTFLAG.bit.SSL) //start of frame
    {
       m_bCSactive=true;
       m_recFIFO.reset();
       m_ComCntr.start(CSyncSerComFSM::FSM::recLengthMSB);
        pSPI->INTFLAG.bit.SSL=1;
    }
    if(pSPI->INTFLAG.bit.RXC)
    {
        typeSChar ch=pSPI->DATA.bit.DATA;
        m_ComCntr.proc(ch, m_recFIFO);
    }
    if(pSPI->INTFLAG.bit.ERROR)
    {
        pSPI->INTFLAG.bit.ERROR=1;
    }
    if(pSPI->INTFLAG.bit.TXC)
    {
        pSPI->INTFLAG.bit.TXC=1;
    }
}

void CSamSPI::OnIRQ0()  //dre
{
    IRQhandler();
}
void CSamSPI::OnIRQ1()  //RX
{
    IRQhandler();
}
void CSamSPI::OnIRQ2()  //error?
{
    IRQhandler();
}
void CSamSPI::OnIRQ3() //ssl?
{
    IRQhandler();
}


//24.06.2019:
void CSamSPI::EnableIRQs(bool how)
{
    //select ptr:
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    m_bIRQmode=how;
    if(how)
    {
        pSPI->INTENSET.reg=(SERCOM_SPI_INTENSET_TXC|SERCOM_SPI_INTENSET_RXC|SERCOM_SPI_INTENSET_SSL);
    }
    else
    {
        //clear all:
        pSPI->INTENCLR.reg=SERCOM_SPI_INTENCLR_MASK; //(SERCOM_SPI_INTENCLR_DRE|SERCOM_SPI_INTENCLR_TXC|SERCOM_SPI_INTENCLR_RXC|SERCOM_SPI_INTENCLR_SSL);
    }

    //tune NVIC:
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ0, how);
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ1, how);
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ2, how);
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ3, how);
}
void CSamSPI::Update()
{
    if(!isIRQmode()) //if not IRQ mode, poll
    {
        IRQhandler();
    }

    //check: thread-safe
    bool bProc=false;
    __disable_irq();
        if(m_ComCntr.get_state()==CSyncSerComFSM::FSM::recOK)
        {
            m_recFIFOhold.reset();
            m_recFIFOhold.swap(m_recFIFO);
            m_ComCntr.start(CSyncSerComFSM::FSM::halted);
            bProc=true;
        }
    __enable_irq();

    if(bProc)
    {
        while(m_recFIFOhold.in_avail())
        {
            typeSChar ch;
            m_recFIFOhold>>ch;
            Fire_on_rec_char(ch);
        }
    }
}

//04.06.2019:
bool CSamSPI::send_char(typeSChar ch)
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    unsigned long WaitBeginTime=get_tick_mS();
    while( 0==(pSPI->INTFLAG.bit.DRE) )
    {
        if( (get_tick_mS()-WaitBeginTime) >100 )
        {
            chip_select(false);
            return false;
        }
    }
    pSPI->DATA.bit.DATA=ch;
    return true;
}

bool CSamSPI::send(CFIFO &msg)
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    chip_select(true);

    //blocking mode:
    typeSChar ch;
    CSyncSerComFSM cntr;    //??? separate, IRQ influence? YES!!! It is still rec clocks!!!
    cntr.start(CSyncSerComFSM::FSM::sendSilenceFrame); //21.05.2019 - start with silent frame
    while(cntr.proc(ch, msg))
    {
       if(!send_char(ch))
           return false;
    }

    chip_select(false);
    return true;
}

bool CSamSPI::receive(CFIFO &msg)
{
    //????????????????
    //................
    return false;
}

//10.05.2019:
bool CSamSPI::send(typeSChar ch)
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    //send:
    pSPI->DATA.bit.DATA=ch;
    while( 0==(pSPI->INTFLAG.bit.DRE) ){}
    return true;
}
bool CSamSPI::receive(typeSChar &ch)
{
    return false;
}


//specific:
void CSamSPI::set_phpol(bool bPhase, bool bPol)
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    pSPI->CTRLA.bit.CPHA=bPhase ? 1:0;
    pSPI->CTRLA.bit.CPOL=bPol ? 1:0;
}
void CSamSPI::set_baud_div(unsigned char div)
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    pSPI->BAUD.bit.BAUD=div;

}
void CSamSPI::set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel)
{

}

