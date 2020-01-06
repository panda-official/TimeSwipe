/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//ADC-DAC mux:


#include "ADmux.h"
#include "sam.h"

CADmux::CADmux()
{
	//setup gain pins: PB14,PB15
    PORT->Group[1].DIRSET.reg=(1L<<14);
    PORT->Group[1].DIRSET.reg=(1L<<15);
    SetGain(typeADgain::gainX1);
    
    //setup ADC/DAC cntr pins:
    //DAC control switch PIN PB04:
    PORT->Group[1].DIRSET.reg=(1L<<4);
    //PORT->Group[1].OUTCLR.reg=(1L<<4);
    SetDACmode(typeDACmode::ExtDACs);

    //ADC control measure switch PIN PB13:
    PORT->Group[1].DIRSET.reg=(1L<<13);
    PORT->Group[1].OUTCLR.reg=(1L<<13);
    m_bADmesEnabled=false; //18.06.2019

    //set UBR PC07 pin to zero:
    PORT->Group[2].DIRSET.reg=(1L<<7);
    PORT->Group[2].OUTCLR.reg=(1L<<7);
    m_UBRVoltage=false;

    //setup fan pins(ventilator) PA09
    PORT->Group[0].DIRSET.reg=(1L<<9);
    PORT->Group[0].OUTCLR.reg=(1L<<9);
    m_bFanIsStarted=false;

    PORT->Group[3].DIRSET.reg=(1L<<10);
    PORT->Group[3].OUTSET.reg=(1L<<10);

}

void CADmux::StartFan(bool how)
{
    m_bFanIsStarted=how;
    if(how)
        PORT->Group[0].OUTSET.reg=(1L<<9);	//on
    else
        PORT->Group[0].OUTCLR.reg=(1L<<9);	//off
}

void CADmux::EnableADmes(bool how)
{
    m_bADmesEnabled=how;
	if(how)
		PORT->Group[1].OUTSET.reg=(1L<<13);	//on
	else
		PORT->Group[1].OUTCLR.reg=(1L<<13); //off
		
}
void CADmux::SetUBRvoltage(bool how)
{
    m_UBRVoltage=how;
    if(how)
        PORT->Group[2].OUTSET.reg=(1L<<7);
    else
        PORT->Group[2].OUTCLR.reg=(1L<<7);

}

void CADmux::SetDACmode(typeDACmode mode)
{
    m_CurDACmode=mode;
    if(typeDACmode::ExtDACs==mode)
    {
        PORT->Group[1].OUTCLR.reg=(1L<<4);
    }
    else
    {
        PORT->Group[1].OUTSET.reg=(1L<<4);
    }
}

void CADmux::SetGain(typeADgain gain)
{
	unsigned int pval=PORT->Group[1].OUT.reg; //cur val
    unsigned int set=0;

    m_CurGain=gain;
    switch(gain)
    {
		case typeADgain::gainX1:
			set=0;
		break;
		
        case typeADgain::gainX2:
            set=(1L<<15);
        break;

        case typeADgain::gainX4:
            set=(1L<<14);
        break;

        case typeADgain::gainX8:
            set=(1L<<14)|(1<<15);
    }

    unsigned int pset=(~((1L<<14)|(1<<15))&pval)|set;

    //set port:
    PORT->Group[1].OUT.reg=pset;
}
