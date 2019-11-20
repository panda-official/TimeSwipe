//setup system clocks + CORTEX-Mx SysTick:

#include "sam.h"

#define SRC_GEN 2

unsigned long get_tick_mS(void);


//----------freq formulas---------
//DPLL=Fsrc*(LDR + 1 + LDFRAC/32 )
//Fdiv(DPLL)=fxOSC/(2*(DIV+1))
//GCLKdiv=2^(N+1)
//--------------------------------

extern "C" {

extern uint32_t __isr_vector;

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

   /* SCB->CPACR |= (0xFu << 20);
    __DSB();
    __ISB();*/

    SysTick_Config(120000); //1mS
}
#endif
}

int sys_clock_init(void){

#ifndef KEMU
	
	//use DFLL+gen2: 20.03.2019
	
	//connect source gen to DPLL0:
	GCLK->PCHCTRL[1].bit.GEN=SRC_GEN;
	GCLK->PCHCTRL[1].bit.CHEN=1;
	 
        //connect DFLL48 to gen2 GCLKdiv=3->2^(3+1)=16, 48/16=3MHz
        GCLK->GENCTRL[SRC_GEN].reg=GCLK_GENCTRL_SRC_DFLL | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_DIV(3) | GCLK_GENCTRL_DIVSEL;
	
	//wait for connection:
	while (1 == GCLK->SYNCBUSY.bit.GENCTRL2);
	
        //3MHz output:
	


	// Set up DPLL0 to output 120MHz using XOSC1 output divided by 12 - max input to the PLL is 3.2MHz
	OSCCTRL->Dpll[0].DPLLRATIO.bit.LDRFRAC  = 0;
        OSCCTRL->Dpll[0].DPLLRATIO.bit.LDR      = 39; //LDR+1=40->3*40=120MHz
        //OSCCTRL->Dpll[0].DPLLCTRLB.bit.DIV      = 5; // 2 * (DIV + 1)
	//OSCCTRL->Dpll[0].DPLLCTRLB.bit.REFCLK   = 3; // use XOSC1 clock reference
	OSCCTRL->Dpll[0].DPLLCTRLB.bit.REFCLK	= 0; //20.03.2019 - now it is dedicated GCLCK (SRC_GEN)
	OSCCTRL->Dpll[0].DPLLCTRLA.bit.ONDEMAND = 0;
	OSCCTRL->Dpll[0].DPLLCTRLA.bit.ENABLE   = 1; // enable the PLL


	// Wait for PLL to be locked and ready
	while(0 == OSCCTRL->Dpll[0].DPLLSTATUS.bit.LOCK || 0 == OSCCTRL->Dpll[0].DPLLSTATUS.bit.CLKRDY);


	// Connect DPLL0 to clock generator 0 (120MHz) - frequency used by CPU, AHB, APBA, APBB
        GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC_DPLL0 | GCLK_GENCTRL_GENEN;
        while (1 == GCLK->SYNCBUSY.bit.GENCTRL0);
 
#endif
 
	//init Cortex-M4 core timer:
	//return SysTick_Config(120000); //=1mS tick value
	return 0;
}

