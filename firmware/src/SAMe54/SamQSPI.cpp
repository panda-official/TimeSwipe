/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//SAM QSPI implementation:

#include "os.h"
#include "SamQSPI.h"
#include "sam.h"

//ctor:
CSamQSPI::CSamQSPI(bool bAutoCS)
{
	//setup QSPI outputs: PA08, PA09, PB10, PB11

    //PA08 -> group 0, even, function "H"(qspi)=0x07
    PORT->Group[0].PMUX[4].bit.PMUXE=0x07;
    PORT->Group[0].PINCFG[8].bit.PMUXEN=1; //enable

    //PB10 -> group 1, even, function "H"(qspi)=0x07
    PORT->Group[1].PMUX[5].bit.PMUXE=0x07;
    PORT->Group[1].PINCFG[10].bit.PMUXEN=1; //enable

    //PB11 -> group 1, odd,  function "H"(qspi)=0x07
    if(bAutoCS)
    {
        PORT->Group[1].PMUX[5].bit.PMUXO=0x07;
        PORT->Group[1].PINCFG[11].bit.PMUXEN=1; //enable
        QSPI->CTRLB.bit.CSMODE=0x01; //keep CS active during whole transfer
    }


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
bool CSamQSPI::send(CFIFO &msg)
{
    while(msg.in_avail())
    {
        typeSChar b;
		msg>>b;
		
		//send:
		QSPI->TXDATA.bit.DATA=b;
		while( 0==(QSPI->INTFLAG.bit.DRE) ){}
	}
    QSPI->CTRLA.reg=0x1000002; //deselect CS

    return true;
}
bool CSamQSPI::receive(CFIFO &msg){return false;}
