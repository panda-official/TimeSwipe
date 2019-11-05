/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//SERCOM2's SPI:

#include "SamSPIsc2.h"
#include "sam.h"

Sercom *glob_GetSercomPtr(typeSamSercoms nSercom);
#define SELECT_SAMSPI(nSercom) &(glob_GetSercomPtr(nSercom)->SPI)

CSamSPIsc2::CSamSPIsc2(bool bMaster) : CSamSPI(typeSamSercoms::Sercom2, bMaster)
{
    //----------setup PINs: IOSET1 PA12, PA13, PA14, PA15----------------
    //PA12 -> group 0, even, function "C"(PAD0)=0x02: MOSI
    PORT->Group[0].PMUX[6].bit.PMUXE=0x02;
    PORT->Group[0].PINCFG[12].bit.PMUXEN=1; //enable

    //PA13 -> group 0, odd, function "C"(PAD1)=0x02:  SCK
    PORT->Group[0].PMUX[6].bit.PMUXO=0x02;
    PORT->Group[0].PINCFG[13].bit.PMUXEN=1; //enable

    //PA14 -> group 0, even, function "C"(PAD2)=0x02: SS
    PORT->Group[0].PMUX[7].bit.PMUXE=0x02;
    PORT->Group[0].PINCFG[14].bit.PMUXEN=1; //enable

    //PA15 -> group 0, odd, function "C"(PAD3)=0x02:  MISO
    PORT->Group[0].PMUX[7].bit.PMUXO=0x02;
    PORT->Group[0].PINCFG[15].bit.PMUXEN=1; //enable //15!!!!!!
    //--------------------------------------------------------------------


    //---------------------finishing init---------------------------------
     SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

     //1: slave mode:
     //pSPI->CTRLA.bit.MODE=0x02;

     //2: CPOL/CPHA - default
     //......................

     //3: FORM - default (SPI frame)
     //.....................

     //4: configure DIPO:
     //DIPO->in slave operation, DI is MOSI, i.e. PAD0 (0x00)
     pSPI->CTRLA.bit.DIPO=0x00;

     //5: configure DOPO:
     //DOPO-> DO, SCK, SS=MISO, SCK, SS=0x02;
     pSPI->CTRLA.bit.DOPO=0x02;


     //6: char size: 8bits default
     //..........................

     //7: data order: MSB defult (0)
     //.........................

     //8: skip
     //......

     //9: enable the receiver
     pSPI->CTRLB.bit.SSDE=1;
    // pSPI->CTRLB.bit.PLOADEN=1;
     pSPI->CTRLB.bit.RXEN=1;
     while( pSPI->SYNCBUSY.bit.CTRLB ){} //wait sync

     //baud: skip
     //.........

     //addr: skip
     //.........

     //enabling device:
     pSPI->CTRLA.bit.ENABLE=1; //started....

     while( pSPI->SYNCBUSY.bit.ENABLE ){} //wait sync
    //--------------------------------------------------------------------
}
