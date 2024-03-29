// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Setup system clocks + CORTEX-Mx SysTick.

#include "system_clock.hpp"

#include <sam.h>

#define SRC_GEN 2

//----------frequency formulas---------
//DPLL=Fsrc*(LDR + 1 + LDFRAC/32 )
//Fdiv(DPLL)=fxOSC/(2*(DIV+1))
//GCLKdiv=2^(N+1)
//-------------------------------------

extern "C" {
#ifndef __NO_SYSTEM_INIT
void SystemInit()
{
    uint32_t *pSrc;
    SCB_Type *pSCB=SCB;

    /* Set the vector table base address */
__disable_irq();
    pSrc      =(uint32_t *)&__isr_vector;
    SCB->VTOR = ((uint32_t)pSrc & SCB_VTOR_TBLOFF_Msk);

    //enable FPU: 17.06.2019:
     SCB->CPACR |= (0xFu << 20);

    __DSB (); //Complete all memory requests
__enable_irq();
    __ISB();


   // SysTick_Config(120000); //1mS
}
#endif
} // extern "C"

int initialize_system_clock()
{
#ifndef KEMU
    //connect source gen to DPLL0:
    GCLK->PCHCTRL[1].bit.GEN=SRC_GEN;
    GCLK->PCHCTRL[1].bit.CHEN=1;

    //connect DFLL48 to gen2 GCLKdiv=3->2^(3+1)=16, 48/16=3MHz
    GCLK->GENCTRL[SRC_GEN].reg=GCLK_GENCTRL_SRC_DFLL | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_DIV(3) | GCLK_GENCTRL_DIVSEL;

    //wait for connection:
    while (1 == GCLK->SYNCBUSY.bit.GENCTRL2);


    // Set up DPLL0 to output 120MHz
    OSCCTRL->Dpll[0].DPLLRATIO.bit.LDRFRAC  = 0;
    OSCCTRL->Dpll[0].DPLLRATIO.bit.LDR      = 39; //LDR+1=40->3*40=120MHz
    OSCCTRL->Dpll[0].DPLLCTRLB.bit.REFCLK	= 0;  //it is dedicated GCLCK (SRC_GEN)
    OSCCTRL->Dpll[0].DPLLCTRLA.bit.ONDEMAND = 0;
    OSCCTRL->Dpll[0].DPLLCTRLA.bit.ENABLE   = 1;  //enable the PLL


    // Wait for PLL to be locked and ready
    while(0 == OSCCTRL->Dpll[0].DPLLSTATUS.bit.LOCK || 0 == OSCCTRL->Dpll[0].DPLLSTATUS.bit.CLKRDY);


    // Connect DPLL0 to clock generator 0 (120MHz) - frequency used by CPU, AHB, APBA, APBB
    GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC_DPLL0 | GCLK_GENCTRL_GENEN;
    while (1 == GCLK->SYNCBUSY.bit.GENCTRL0);

#endif

    SysTick_Config(120000); //1mS
    return 0;
}
