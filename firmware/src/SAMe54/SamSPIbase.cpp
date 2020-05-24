/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include <cassert>
#include "SamSPIbase.h"
#include "sam.h"

Sercom *glob_GetSercomPtr(typeSamSercoms nSercom);
#define SELECT_SAMSPI(nSercom) &(glob_GetSercomPtr(nSercom)->SPI)

CSamSPIbase::CSamSPIbase(bool bMaster, typeSamSercoms nSercom, CSamPORT::pxy MOSI, CSamPORT::pxy MISO, CSamPORT::pxy CLOCK, CSamPORT::pxy CS) :
    CSamSercom(nSercom)
{

    bool bRes;

    CSamPORT::pad MOSIpad, MISOpad, CLOCKpad, CSpad;

    bRes=CSamPORT::MUX(MOSI, nSercom, MOSIpad);
    assert(bRes);
    bRes=CSamPORT::MUX(MISO, nSercom, MISOpad);
    assert(bRes);
    bRes=CSamPORT::MUX(CLOCK, nSercom, CLOCKpad);
    assert(bRes);
    assert(CSamPORT::pad::PAD1==CLOCKpad); //always

    if(CSamPORT::pxy::none!=CS)
    {
        bRes=CSamPORT::MUX(CS, nSercom, CSpad);
        assert(bRes);
        assert(CSamPORT::pad::PAD2==CSpad); //always
    }

    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

    //enable sercom bus:
    CSamSercom::EnableSercomBus(nSercom, true);


    //config DIPO/DOPO depending on PAD:
    if(bMaster)
    {
        assert(CSamPORT::pad::PAD0==MOSIpad || CSamPORT::pad::PAD3==MOSIpad);
        if(CSamPORT::pad::PAD0==MOSIpad) //variant DOPO=0
        {
            //MISO->PAD3
            assert(CSamPORT::pad::PAD3==MISOpad);

            pSPI->CTRLA.bit.DOPO=0x00;
            pSPI->CTRLA.bit.DIPO=0x03;
        }
        else                            //variant DOPO=2
        {
            //MISO->PAD0
            assert(CSamPORT::pad::PAD0==MISOpad);

            pSPI->CTRLA.bit.DOPO=0x02;
            pSPI->CTRLA.bit.DIPO=0x00;
        }

        //in the master mode clock is also required:
        m_pCLK=CSamCLK::Factory();
        ConnectGCLK(m_nSercom, m_pCLK->CLKind());
        m_pCLK->Enable(true);
        pSPI->BAUD.bit.BAUD=0xff; //lowest possible by default

    }
    else    //in the slave mode output is MISO
    {
        assert(CSamPORT::pad::PAD0==MISOpad || CSamPORT::pad::PAD3==MISOpad);
        if(CSamPORT::pad::PAD0==MISOpad) //variant DOPO=0
        {
            //MOSI->PAD3
            assert(CSamPORT::pad::PAD3==MOSIpad);

            pSPI->CTRLA.bit.DOPO=0x00;
            pSPI->CTRLA.bit.DIPO=0x03;
        }
        else                            //variant DOPO=2
        {
            //MOSI->PAD0
            assert(CSamPORT::pad::PAD0==MOSIpad);

            pSPI->CTRLA.bit.DOPO=0x02;
            pSPI->CTRLA.bit.DIPO=0x00;
        }
    }

    pSPI->CTRLA.bit.MODE=bMaster ? 0x03: 0x02;


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
    while( 0==(pSPI->INTFLAG.bit.TXC) ){}
    return pSPI->DATA.bit.DATA;
}

bool CSamSPIbase::transfer(CFIFO &out_msg, CFIFO &in_msg)
{
    //cs on:
    //.....

    while(out_msg.in_avail())
    {
        typeSChar b;
        out_msg>>b;
        in_msg<<transfer_char(b);
    }


    //cs off:
    //.......
}

