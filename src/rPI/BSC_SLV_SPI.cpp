/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#include "BSC_SLV_SPI.h"

#include "BCMregs.h"
#include "bcm2835.h"

#define SELECT_BSC_SLV() (typeBSC_SLV *)(bcm2835_peripherals + (0x214000/4))

unsigned long get_tick_mS(void);
CBSCslaveSPI::CBSCslaveSPI()
{
    if(!m_bLibInitialized)
        return;

    //start init: GPIO
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_12, BCM2835_GPIO_FSEL_ALT3);       //GPIO18 MOSI
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_35, BCM2835_GPIO_FSEL_ALT3);       //GPIO19 SCLK
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_38, BCM2835_GPIO_FSEL_ALT3);       //GPIO20 MISO
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_40, BCM2835_GPIO_FSEL_ALT3);       //GPIO21 CE_N

    typeBSC_SLV *pBSC=SELECT_BSC_SLV();

    //setup control:
    typeBSC_SLV_CR cr;
    cr.reg=0;
    cr.bit.EN=1;
    cr.bit.SPI=1;
    cr.bit.TXE=1;
    cr.bit.RXE=1;
    bcm2835_peri_write( (volatile uint32_t *)&(pBSC->CR), cr.reg);
   
   //+++dbg: read back test: 
    typeBSC_SLV_CR cr_t;
    cr_t.reg=bcm2835_peri_read((volatile uint32_t *)&(pBSC->CR));
    
    //m_ComCntr.start(CSyncSerComFSM::FSM::recLengthMSB); //+++dbg
    m_LastChRecTime_mS=get_tick_mS();

    m_Initialized=true;
}
CBSCslaveSPI::~CBSCslaveSPI()
{
    if(!m_Initialized)
        return;

    //return pins to the normal state:
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_12, BCM2835_GPIO_FSEL_INPT);       //GPIO18 MOSI
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_35, BCM2835_GPIO_FSEL_INPT);       //GPIO19 SCLK
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_38, BCM2835_GPIO_FSEL_INPT);       //GPIO20 MISO
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_40, BCM2835_GPIO_FSEL_INPT);       //GPIO21 CE_N

}

//polling:
void CBSCslaveSPI::check_rx() //polling
{
    typeBSC_SLV *pBSC=SELECT_BSC_SLV();

    typeBSC_SLV_DR dr;
    dr.reg=bcm2835_peri_read((volatile uint32_t *)&(pBSC->DR));
    
    while(dr.bit.RXFLEVEL>0)
    {
		typeSChar ch=dr.bit.DATA;
        m_ComCntr.proc(ch, m_recFIFO);        
        m_LastChRecTime_mS=get_tick_mS();
        dr.reg=bcm2835_peri_read((volatile uint32_t *)&(pBSC->DR));
	}
	
	//04.06.19 since we have no CS select detection, use timeout frame:
	if( (get_tick_mS()-m_LastChRecTime_mS)>100 )
	{
		m_LastChRecTime_mS=get_tick_mS();
		m_recFIFO.reset();
		m_ComCntr.start(CSyncSerComFSM::FSM::recLengthMSB);
	}
}

bool CBSCslaveSPI::send(CFIFO &msg)
{
    if(!is_initialzed())
        return false;

    //blocking mode:
    typeBSC_SLV *pBSC=SELECT_BSC_SLV();
    
    typeBSC_SLV_CR cr;
    cr.reg=bcm2835_peri_read((volatile uint32_t *)&(pBSC->CR));
    cr.bit.BRK=1;
    bcm2835_peri_write( (volatile uint32_t *)&(pBSC->CR), cr.reg);
    
    
    typeSChar ch;
    m_ComCntr.start(CSyncSerComFSM::FSM::sendSilenceFrame); //21.05.2019 - start with silent frame
    while(m_ComCntr.proc(ch, msg))
    {

        typeBSC_SLV_DR dr;
        unsigned long WaitBeginTime=get_tick_mS();
        do
        {
            if( (get_tick_mS()-WaitBeginTime) >100 )
                return false;

            dr.reg=bcm2835_peri_read((volatile uint32_t *)&(pBSC->DR));
        }
        while(dr.bit.TXFF);

        //send:
        dr.bit.DATA=ch;
        bcm2835_peri_write((volatile uint32_t *)&(pBSC->DR), dr.reg);

    }
    return true;
}
bool CBSCslaveSPI::receive(CFIFO &msg)
{
    if(!is_initialzed())
        return false;

    //blocking mode:

  /*  while(m_ComCntr.get_state()<CSyncSerComFSM::FSM::recOK)
    {
        check_rx();
    }*/

    //non-blocking, polling dbg...

    if(m_ComCntr.get_state()<CSyncSerComFSM::FSM::recOK)
    {
        check_rx();
        return false;
    }
    if(CSyncSerComFSM::FSM::recOK==m_ComCntr.get_state())
    {
        msg=m_recFIFO;
        m_recFIFO.reset();
        m_ComCntr.start(CSyncSerComFSM::FSM::recLengthMSB);
        return true;
    }
    m_ComCntr.start(CSyncSerComFSM::FSM::recLengthMSB);
    return false;
}

bool CBSCslaveSPI::send(typeSChar ch)
{
    return false;
}
bool CBSCslaveSPI::receive(typeSChar &ch)
{
    return false;
}

void CBSCslaveSPI::set_phpol(bool bPhase, bool bPol)
{

}
void CBSCslaveSPI::set_baud_div(unsigned char div)
{

}
void CBSCslaveSPI::set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel)
{

}





