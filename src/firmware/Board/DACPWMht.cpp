/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "DACPWMht.h"
#include "sam.h"

std::shared_ptr<CSamCLK> CDacPWMht::m_pCLK;


//light IRQs:
static CDacPWMht *pPWM[2]={nullptr};
static unsigned int LowLevel1, HighLevel1;
static unsigned int LowLevel2, HighLevel2;
extern "C"{
void TC0_Handler(void)
{
    Dmac *pDMA=DMAC;

    if(TC0->COUNT32.INTFLAG.reg & TC_INTFLAG_MC1 ) //low level
    {
        DAC->DATA[0].reg=LowLevel1;
    }
    else                             //high level
    {
        DAC->DATA[0].reg=HighLevel1;
    }
    TC0->COUNT32.INTFLAG.reg=0xff; //clear
}
void TC2_Handler(void)
{
    if(TC2->COUNT32.INTFLAG.reg & TC_INTFLAG_MC1 ) //low level
    {
        DAC->DATA[1].reg=LowLevel2;
    }
    else                             //high level
    {
        DAC->DATA[1].reg=HighLevel2;
    }
    TC2->COUNT32.INTFLAG.reg=0xff; //clear
}

void TC4_Handler(void)
{
    pPWM[0]->Start(false);
    TC4->COUNT16.INTFLAG.reg=0xff; //clear
}
void TC5_Handler(void)
{
    pPWM[1]->Start(false);
    TC5->COUNT16.INTFLAG.reg=0xff; //clear
}

}


Tc *glob_GetTcPtr(typeSamTC nTc);
CDacPWMht::CDacPWMht(PWM nPWM, const std::shared_ptr<CPin> &pDACsw, mode nOpMode) :
    CSamTC(PWM1==nPWM ? typeSamTC::Tc0 : typeSamTC::Tc2),
    m_PeriodsCounter(PWM1==nPWM ? typeSamTC::Tc4 : typeSamTC::Tc5)
{
    m_nPWM=nPWM;
    m_pDACsw=pDACsw;

    pPWM[nPWM]=this;

    //tune the timer (32 bit, pair)
    CSamTC::EnableAPBbus(true);
    CSamTC::EnableAPBbus(static_cast<typeSamTC>(static_cast<int>(m_nTC)+1), true);

    //get clock:
    if(!m_pCLK)
    {
        m_pCLK=CSamCLK::Factory();
        m_pCLK->Enable(true); //???
    }
    CSamTC::ConnectGCLK(m_pCLK->CLKind());


    //DMA support:
    if(mode::DMA==nOpMode)
    {
        CSamDMAC &dmac=CSamDMAC::Instance();

        m_pHLevDMAch=dmac.Factory();
        m_pHLevDMAch->SetupTrigger(CSamDMAChannel::trigact::BLOCK, PWM1==nPWM ? CSamDMAChannel::trigsrc::TC0MC0 : CSamDMAChannel::trigsrc::TC2MC0);
        m_pHLevDMAch->AddBlock().Setup(&m_prmHighLevel16, (const void*)&(DAC->DATA[nPWM]), 1, CSamDMABlock::beatsize::HWORD16);
        m_pHLevDMAch->SetLoopMode();
        m_pHLevDMAch->Enable(true);

        m_pLLevDMAch=dmac.Factory();
        m_pLLevDMAch->SetupTrigger(CSamDMAChannel::trigact::BLOCK, PWM1==nPWM ? CSamDMAChannel::trigsrc::TC0MC1 : CSamDMAChannel::trigsrc::TC2MC1);
        m_pLLevDMAch->AddBlock().Setup(&m_prmLowLevel16, (const void*)&(DAC->DATA[nPWM]), 1, CSamDMABlock::beatsize::HWORD16);
        m_pLLevDMAch->SetLoopMode();
        m_pLLevDMAch->Enable(true);
    }


    Tc *pTc=glob_GetTcPtr(m_nTC);

    pTc->COUNT32.CTRLA.bit.MODE=2; //32bit
    pTc->COUNT32.WAVE.bit.WAVEGEN=1; //MFRQ: CC0=TOP
    pTc->COUNT32.CC[0].reg=0xffff; //prevent IRQ on start
    pTc->COUNT32.CC[1].reg=0xffff;
    if(mode::IRQ==nOpMode)
    {
        pTc->COUNT32.INTENSET.reg=(TC_INTFLAG_MC0|TC_INTFLAG_MC1);
        CSamTC::EnableIRQ(true);
    }

    pTc->COUNT32.EVCTRL.bit.MCEO1=1;
    pTc->COUNT32.CTRLA.bit.ENABLE=1; //enable
    pTc->COUNT32.CTRLBSET.bit.CMD=2; //keep it in the stopped state!

    //interconnect them with an event system:
    MCLK->APBBMASK.bit.EVSYS_=1;
    EVSYS->USER[ static_cast<int>(m_PeriodsCounter.GetID())+44 ].bit.CHANNEL=nPWM+13;//+1 !!!
    EVSYS->Channel[nPWM+12].CHANNEL.bit.EVGEN=(PWM1==nPWM ? 0x4B:0x51);
    EVSYS->Channel[nPWM+12].CHANNEL.bit.PATH=2; //assync

    //period counter:
    m_PeriodsCounter.EnableAPBbus(true);
    Tc *pTc2=glob_GetTcPtr(m_PeriodsCounter.GetID());
    pTc2->COUNT16.CTRLA.bit.ONDEMAND=1;
    pTc2->COUNT16.EVCTRL.bit.EVACT=2; //count on event
    pTc2->COUNT16.EVCTRL.bit.TCEI=1;  //input event enable
    pTc2->COUNT16.WAVE.bit.WAVEGEN=1; //MFRQ: CC0=TOP
    pTc2->COUNT16.INTENSET.reg=(TC_INTFLAG_MC0);

    m_PeriodsCounter.ConnectGCLK(m_pCLK->CLKind()); //clock is always required
    m_PeriodsCounter.EnableIRQ(true);
}

void CDacPWMht::on_obtain_half_periods()
{
    Tc *pTc=glob_GetTcPtr(m_nTC);

    float tper=48000000/m_prmFrequency;
    unsigned int cc0val=(unsigned int)tper;
    unsigned int cc1val=(unsigned int)(tper*m_prmDutyCycle);

    //sync with CCs:
    while(pTc->COUNT32.SYNCBUSY.bit.CC0 || pTc->COUNT32.SYNCBUSY.bit.CC1){}
    pTc->COUNT32.CC[0].reg=cc0val;
    pTc->COUNT32.CC[1].reg=cc1val;

    //check if timer counter overruns:
    if(m_bStarted)
    {
        //get counter:
        while(pTc->COUNT32.SYNCBUSY.bit.CTRLB){}
        pTc->COUNT32.CTRLBSET.bit.CMD=4;
        while(pTc->COUNT32.SYNCBUSY.bit.COUNT || pTc->COUNT32.SYNCBUSY.bit.CTRLB){}
        if(pTc->COUNT32.COUNT.reg > (cc0val-10) )
        {
            //restart:
            synced_DAC_set(m_prmHighLevel);
            pTc->COUNT32.CTRLBSET.bit.CMD=1;
        }
    }
}

void CDacPWMht::synced_DAC_set(unsigned int nLevel)
{
    if(PWM1==m_nPWM)
    {
        while( (0==DAC->STATUS.bit.READY0) || DAC->SYNCBUSY.bit.DATA0){} // || (0==DAC->STATUS.bit.EOC0) ){}
    }
    else
    {
        while( (0==DAC->STATUS.bit.READY1) || DAC->SYNCBUSY.bit.DATA1){} // || (0==DAC->STATUS.bit.EOC1) ){}
    }
    DAC->DATA[m_nPWM].reg=nLevel;

    if(PWM1==m_nPWM)
    {
        while(0==DAC->STATUS.bit.EOC0){}
    }
    else
    {
        while(0==DAC->STATUS.bit.EOC1){}
    }
}

void CDacPWMht::impl_Start(bool bHow)
{
    Tc *pTc=glob_GetTcPtr(m_nTC);
    Tc *pTc2=glob_GetTcPtr(m_PeriodsCounter.GetID());
    while(pTc->COUNT32.SYNCBUSY.bit.CTRLB){}

    Evsys *pEv=EVSYS;

    if(bHow)
    {
        on_settings_changed();
        synced_DAC_set(m_prmHighLevel); //start with high
        m_pDACsw->Set(true);

        while(pTc2->COUNT16.SYNCBUSY.bit.ENABLE){}
        if(m_prmRepeats)
        {
            while(pTc2->COUNT16.SYNCBUSY.bit.CTRLB || pTc2->COUNT16.SYNCBUSY.bit.CC0){}

            pTc2->COUNT16.CC[0].reg=m_prmRepeats;
            pTc2->COUNT16.CTRLA.bit.ENABLE=1;
            pTc2->COUNT16.CTRLBSET.bit.CMD=1;
        }
        else {
            pTc2->COUNT16.CTRLA.bit.ENABLE=0;
        }
        pTc->COUNT32.CTRLBSET.bit.CMD=1;    //start


    }
    else
    {
        pTc->COUNT32.CTRLBSET.bit.CMD=2;    //stop
        synced_DAC_set(2048);      //clear
    }
}

void CDacPWMht::on_settings_changed()
{
    m_prmHighLevel16=static_cast<uint16_t>(m_prmHighLevel);
    m_prmLowLevel16=static_cast<uint16_t>(m_prmLowLevel);


    if(PWM1==m_nPWM)
    {
        //Repeats1=m_prmRepeats;
        LowLevel1=m_prmLowLevel;
        HighLevel1=m_prmHighLevel;
    }
    else
    {
        //Repeats2=m_prmRepeats;
        LowLevel2=m_prmLowLevel;
        HighLevel2=m_prmHighLevel;
    }
}
void CDacPWMht::impl_LoadNextHalfPeriod()
{
}
