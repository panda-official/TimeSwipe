/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/


//SERCOM2's SPI:

#include "SamSPIsc7.h"
#include "sam.h"

Sercom *glob_GetSercomPtr(typeSamSercoms nSercom);
#define SELECT_SAMSPI(nSercom) &(glob_GetSercomPtr(nSercom)->SPI)

void Wait(unsigned long time_mS);
void CSamSPIsc7::chip_select(bool how)
{
    if(!m_bMaster)
        return;

    if(how)
    {
        PORT->Group[3].OUTCLR.bit.OUTCLR=(1L<<10);
        Wait(1);
    }
    else
    {
        Wait(5); //???
        PORT->Group[3].OUTSET.bit.OUTSET=(1L<<10);  //initial state=HIGH
    }
}

CSamSPIsc7::CSamSPIsc7(bool bMaster) : CSamSPI(typeSamSercoms::Sercom7, bMaster)
{
    Port *pPORTS=PORT;

    //----------setup PINs: IOSET1 PD08, PD09, PD10, PD11----------------
    //PD08 -> group 3, even, function "C"(PAD0)=0x02: MOSI
    PORT->Group[3].PMUX[4].bit.PMUXE=0x02;
    PORT->Group[3].PINCFG[8].bit.PMUXEN=1; //enable

    //PD09 -> group 3, odd, function "C"(PAD1)=0x02:  SCK
    PORT->Group[3].PMUX[4].bit.PMUXO=0x02;
    PORT->Group[3].PINCFG[9].bit.PMUXEN=1; //enable

    if(m_bMaster) //configure as output:
    {
        PORT->Group[3].PINCFG[10].bit.PMUXEN=0;
        PORT->Group[3].DIRSET.bit.DIRSET=(1L<<10);
        PORT->Group[3].OUTSET.bit.OUTSET=(1L<<10);  //initial state=HIGH
    }
    else
    {
        //PD10 -> group 3, even, function "C"(PAD2)=0x02: SS
        PORT->Group[3].PMUX[5].bit.PMUXE=0x02;
        PORT->Group[3].PINCFG[10].bit.PMUXEN=1; //enable
    }

    //PD11 -> group 0, odd, function "C"(PAD3)=0x02:  MISO
    PORT->Group[3].PMUX[5].bit.PMUXO=0x02;
    PORT->Group[3].PINCFG[11].bit.PMUXEN=1;
    //--------------------------------------------------------------------


    //---------------------finishing init---------------------------------
     SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);

     //1: slave mode:
   //  pSPI->CTRLA.bit.MODE=0x02;

     //2: CPOL/CPHA - default
     //......................

     //3: FORM - default (SPI frame)
     //.....................

     if(m_bMaster)
     {
         pSPI->CTRLA.bit.DIPO=0x03;
         pSPI->CTRLA.bit.DOPO=0x00;
         //pSPI->CTRLA.bit.DIPO=0x00;
         //pSPI->CTRLA.bit.DOPO=0x02;
     }
     else{

     //4: configure DIPO:
     //DIPO->in slave operation, DI is MOSI, i.e. PAD0 (0x00)
     pSPI->CTRLA.bit.DIPO=0x00;

     //5: configure DOPO:
     //DOPO-> DO, SCK, SS=MISO, SCK, SS=0x02;
     pSPI->CTRLA.bit.DOPO=0x02;

     }


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
