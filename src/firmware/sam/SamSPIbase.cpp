/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "SamSPIbase.h"
#include "../os.h"

#include <sam.h>

#include <cassert>

Sercom *glob_GetSercomPtr(Sam_sercom::Id nSercom);
#define SELECT_SAMSPI(nSercom) &(glob_GetSercomPtr(nSercom)->SPI)

CSamSPIbase::CSamSPIbase(bool bMaster, Id nSercom,
  Sam_pin::Id MOSI, Sam_pin::Id MISO, Sam_pin::Id CLOCK, std::optional<Sam_pin::Id> CS,
                         std::shared_ptr<CSamCLK> pCLK) :
    Sam_sercom(nSercom)
{
    Sam_pin::Id DO, DI;

    m_bMaster=bMaster;
    if(bMaster)
    {
        DO=MOSI;
        DI=MISO;
    }
    else
    {
        DO=MISO;
        DI=MOSI;
    }

    bool bRes;
    Sam_pin::Pad DOpad, DIpad, CLOCKpad; //, CSpad;
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);
   // Port *pPort=PORT;

    //enable sercom bus:
    Sam_sercom::EnableSercomBus(nSercom, true);

    bRes=Sam_pin::connect(DO, nSercom, DOpad);
    assert(bRes);
    bRes=Sam_pin::connect(DI, nSercom, DIpad);
    assert(bRes);
    bRes=Sam_pin::connect(CLOCK, nSercom, CLOCKpad);
    assert(bRes);
    assert(Sam_pin::pad::PAD1==CLOCKpad); //always

    if(CS)
    {
        m_pCS = std::make_shared<Sam_pin>(*CS, bMaster);
        assert(m_pCS);
        bRes=m_pCS->connect(nSercom);
        assert(bRes && Sam_pin::pad::PAD2==m_pCS->GetPAD()); //always

        //if set CS pin in constructor, make it hardware controlled:
        pSPI->CTRLB.bit.MSSEN=bMaster; //auto cs
    }


    //config DIPO/DOPO depending on PAD:
   // assert(Sam_pin::pad::PAD0==DOpad || Sam_pin::pad::PAD3==DIpad);
    if(Sam_pin::Pad::pad0==DOpad) //variant DOPO=0
    {
        //DI->PAD3
        assert(Sam_pin::pad::PAD3==DIpad);

        pSPI->CTRLA.bit.DOPO=0x00;
        pSPI->CTRLA.bit.DIPO=0x03;
    }
    else                            //variant DOPO=2
    {
        //DI->PAD0
        assert(Sam_pin::pad::PAD0==DIpad);

        pSPI->CTRLA.bit.DOPO=0x02;
        pSPI->CTRLA.bit.DIPO=0x00;
     }

     if(bMaster)
     {
         pSPI->CTRLA.bit.MODE=0x03; //master


        //in the master mode clock is also required:
        if(pCLK)
        {
            m_pCLK=pCLK;                //use custom clock
        }
        else
        {
            m_pCLK=CSamCLK::Factory();  //or generate automatically
            assert(m_pCLK);
        }
        ConnectGCLK(m_nSercom, m_pCLK->CLKind());
        m_pCLK->Enable(true);
        pSPI->BAUD.bit.BAUD=0xff; //lowest possible by default
     }
     else
     {
         pSPI->CTRLA.bit.MODE=0x02; //slave
     }


    //usualy the receiver is required:
    pSPI->CTRLB.bit.SSDE=1;
    pSPI->CTRLB.bit.RXEN=1;
    while( pSPI->SYNCBUSY.bit.CTRLB ){} //wait sync

    //and enable:
    pSPI->CTRLA.bit.ENABLE=1;
    while( pSPI->SYNCBUSY.bit.ENABLE ){} //wait sync
}

uint32_t CSamSPIbase::transfer_char(uint32_t nChar)
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    while( 0==(pSPI->INTFLAG.bit.DRE) ){}
    pSPI->DATA.bit.DATA=nChar;
    while( 0==(pSPI->INTFLAG.bit.TXC)  || 0==(pSPI->INTFLAG.bit.RXC)){}
    return pSPI->DATA.bit.DATA;
}

bool CSamSPIbase::send_char(uint32_t ch)
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    unsigned long WaitBeginTime=os::get_tick_mS();
    while( 0==(pSPI->INTFLAG.bit.DRE) )
    {
        if( (os::get_tick_mS()-WaitBeginTime) >m_SendCharTmt_mS )
        {
            //chip_select(false);
            return false;
        }
    }
    pSPI->DATA.bit.DATA=ch;
    return true;
}


bool CSamSPIbase::transfer(CFIFO &out_msg, CFIFO &in_msg)
{
    //only possible in the master mode (means master clock is provided)
    assert(m_bMaster);

    in_msg.reset(); //???

    //cs on:
    //chip_select(true);

    while(out_msg.in_avail())
    {
        Character b;
        out_msg>>b;
        in_msg<<static_cast<Character>( transfer_char( static_cast<uint32_t>(b)) );
    }


    //cs off:
    //chip_select(false);

    return true;
}
bool CSamSPIbase::send(CFIFO &out_msg)
{
    //chip_select(true);

    //blocking mode:
    while(out_msg.in_avail())
    {
        Character b;
        out_msg>>b;
        if( !send_char( static_cast<uint32_t>(b)) )
        {
            //chip_select(false);
            return false;
        }
    }

    //chip_select(false);
    return true;
}


void CSamSPIbase::set_phpol(bool bPhase, bool bPol)
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    while( pSPI->SYNCBUSY.bit.ENABLE ){} //wait sync
    pSPI->CTRLA.bit.ENABLE=0;

    pSPI->CTRLA.bit.CPHA=bPhase ? 1:0;
    pSPI->CTRLA.bit.CPOL=bPol ? 1:0;

    pSPI->CTRLA.bit.ENABLE=1;
    while( pSPI->SYNCBUSY.bit.ENABLE ){} //wait sync

}
void CSamSPIbase::set_baud_div(unsigned char div)
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    pSPI->BAUD.bit.BAUD=div;

}


void CSamSPIbase::EnableIRQs(bool how)
{
    //select ptr:
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);
 //   Port *pPort=PORT;

    m_bIRQmode=how;
    if(how)
    {
        pSPI->INTENSET.reg=(SERCOM_SPI_INTENSET_TXC|SERCOM_SPI_INTENSET_RXC|SERCOM_SPI_INTENSET_SSL);
    }
    else
    {
        //clear all:
        pSPI->INTENCLR.reg=SERCOM_SPI_INTENCLR_MASK;
    }

    //tune NVIC:
    Sam_sercom::EnableIRQ(Irq::irq0, how);
    Sam_sercom::EnableIRQ(Irq::irq1, how);
    Sam_sercom::EnableIRQ(Irq::irq2, how);
    Sam_sercom::EnableIRQ(Irq::irq3, how);
}
