/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//SAM QSPI implementation:

#include "SamQSPI.h"
#include "sam.h"

//ctor:
CSamQSPI::CSamQSPI()
{
	//preparing periph:
	//setup QSPI outputs: PA08, PA09, PB10, PB11

    //PA08 -> group 0, even, function "H"(qspi)=0x07
    PORT->Group[0].PMUX[4].bit.PMUXE=0x07;
    PORT->Group[0].PINCFG[8].bit.PMUXEN=1; //enable

    //PA09 -> group 0, even, function "H"(qspi)=0x07
    PORT->Group[0].PMUX[4].bit.PMUXO=0x07;
    PORT->Group[0].PINCFG[9].bit.PMUXEN=1; //enable

    //PB10 -> group 1, even, function "H"(qspi)=0x07
    PORT->Group[1].PMUX[5].bit.PMUXE=0x07;
    PORT->Group[1].PINCFG[10].bit.PMUXEN=1; //enable

    //PB11 -> group 1, odd,  function "H"(qspi)=0x07 //CS
    PORT->Group[1].PMUX[5].bit.PMUXO=0x07;
    PORT->Group[1].PINCFG[11].bit.PMUXEN=1; //enable

/*    PORT->Group[1].DIRSET.bit.DIRSET=(1L<<11);
    PORT->Group[1].OUTSET.bit.OUTSET=(1L<<11); //keep hi*/
    
    //now connecting SPI clocks:
    //CLK_QSPI_APB -enabled by default
    //CLK_QSPI_AHB -enabled by default
    //CLK_QSPI2X_AHB -enabled by default
    
    QSPI->CTRLB.bit.CSMODE=0x01; //keep CS active during whole transfer
    QSPI->CTRLA.bit.ENABLE=1; //enble QSPI
}

//settings:
void CSamQSPI::set_phpol(bool bPhase, bool bPol)
{
	QSPI->BAUD.bit.CPHA= bPhase		? 1:0; //phase
	QSPI->BAUD.bit.CPOL= bPol 		? 1:0; //polarity
}
void CSamQSPI::set_baud_div(unsigned char div)
{
	QSPI->BAUD.bit.BAUD=div;
}
void CSamQSPI::set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel)
{
	QSPI->CTRLB.bit.DLYCS=CSminDel;
	QSPI->CTRLB.bit.DLYBCT=IntertransDel;
	QSPI->BAUD.bit.DLYBS=BeforeClockDel;
}

//serial impl:
void Wait(unsigned long time_mS);
bool CSamQSPI::send(CFIFO &msg)
{
   /* PORT->Group[1].OUTCLR.bit.OUTCLR=(1L<<11); //cs low
    Wait(1);*/

    while(msg.in_avail())
    {
        typeSChar b;
		msg>>b;
		
		//send:
		QSPI->TXDATA.bit.DATA=b;
		while( 0==(QSPI->INTFLAG.bit.DRE) ){}
	}
    QSPI->CTRLA.reg=0x1000002; //deselect

   /* Wait(1);
    PORT->Group[1].OUTSET.bit.OUTSET=(1L<<11); //cs high*/
}
bool CSamQSPI::receive(CFIFO &msg){return false;} //stub

//10.05.2019
bool CSamQSPI::send(typeSChar ch){ return false;} //stub
bool CSamQSPI::receive(typeSChar &ch){ return false; }//stub
