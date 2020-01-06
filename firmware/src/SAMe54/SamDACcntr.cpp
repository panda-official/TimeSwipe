/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//SAM DAC controller class impl:

#include "os.h"
#include "SamDACcntr.h"
#include "sam.h"

#define GCLK_DAC   42

bool   CSamDACcntr::m_bInitialized=false;

CSamDACcntr::CSamDACcntr(typeSamDAC nChan, float RangeMin, float RangeMax)
{
    m_chan=nChan;
    m_IntRange=4095;
    SetRange(RangeMin, RangeMax);

    common_init();
}

//driver function:
void CSamDACcntr::DriverSetVal(float val, int out_bin)
{
    //1: wait while DAC is ready:
    if(typeSamDAC::Dac0==m_chan)
    {
        while(0==DAC->STATUS.bit.READY0){}
        while(DAC->SYNCBUSY.bit.DATA0){}
        DAC->DATA[0].bit.DATA=out_bin;
        while(0==DAC->STATUS.bit.EOC0){}
    }
    else
    {
         while(0==DAC->STATUS.bit.READY1){}
         while(DAC->SYNCBUSY.bit.DATA1){}
         DAC->DATA[1].bit.DATA=out_bin;
         while(0==DAC->STATUS.bit.EOC1){}
    }
}
void CSamDACcntr::common_init() //common settings for both dacs
{
    if(m_bInitialized) return;

    //----------------------setup PINs:-------------------------------
    //reference PA03 -> group 0, odd, function "B"(DAC)=0x01: ANAREF-> VREFA
    PORT->Group[0].PMUX[1].bit.PMUXO=0x01;
    PORT->Group[0].PINCFG[3].bit.PMUXEN=1; //enable

    //DAC0 VOUT PA02 -> group 0, even, function "B"(DAC)=0x01: DAC/VOUT[0]
    PORT->Group[0].PMUX[1].bit.PMUXE=0x01;
    PORT->Group[0].PINCFG[2].bit.PMUXEN=1; //enable

    //DAC1 VOUT PA05 -> group 0, odd, function "B"(DAC)=0x01: DAC/VOUT[1]
    PORT->Group[0].PMUX[2].bit.PMUXO=0x01;
    PORT->Group[0].PINCFG[5].bit.PMUXEN=1; //enable
    //-----------------------------------------------------------------


    //----------------enable main clock to drive DAC bus--------------
    MCLK->APBDMASK.bit.DAC_=1;
    //----------------------------------------------------------------

    //-----------------------connect default generator----------------
    m_pCLK=CSamCLK::Factory();

    //enable gclk: set to Current Control default: CC100K GCLK_DAC ≤ 1.2MHz (100kSPS) 48MHz/64=750KHz
    GCLK->PCHCTRL[GCLK_DAC].bit.GEN=static_cast<uint32_t>(m_pCLK->CLKind());
    GCLK->PCHCTRL[GCLK_DAC].bit.CHEN=1;

    m_pCLK->SetDiv(6);
    m_pCLK->Enable(true);
    //----------------------------------------------------------------


    //----------------------finishing init & enabling--------------------
    DAC->CTRLB.bit.REFSEL=0;

    //Control: all default except REFRESH and ENABLE...
    DAC->DACCTRL[0].bit.REFRESH=1;
    DAC->DACCTRL[1].bit.REFRESH=1;
    DAC->DACCTRL[0].bit.ENABLE=1;
    DAC->DACCTRL[1].bit.ENABLE=1;

     os::wait(2); //the waiting is required

    //enable controller itself:
    DAC->CTRLA.bit.ENABLE=1;
    while(DAC->SYNCBUSY.bit.ENABLE){}
    //-------------------------------------------------------------------


    m_bInitialized=true; //init is finished
}
