#include "DACPWMht.h"
#include "sam.h"

std::shared_ptr<CSamCLK> CDacPWMht::m_pCLK;


//light IRQs:
static CDacPWMht *pPWM[2]={nullptr};
static unsigned int Repeats1, LowLevel1, HighLevel1;
static unsigned int Repeats2, LowLevel2, HighLevel2;
extern "C"{
void TC0_Handler(void)
{
    if(TC0->COUNT32.INTFLAG.reg & TC_INTFLAG_MC1 ) //low level
    {
        DAC->DATA[0].reg=LowLevel1;
    }
    else                             //high level
    {
        DAC->DATA[0].reg=HighLevel1;
        if(Repeats1)
        {
            if(--Repeats1==0)
                pPWM[0]->Start(false);
        }
    }
    TC0->COUNT32.INTFLAG.reg=0xff; //clear
}
void TC4_Handler(void)
{
    if(TC4->COUNT32.INTFLAG.reg & TC_INTFLAG_MC1 ) //high level
    {
        DAC->DATA[1].reg=LowLevel2;
    }
    else                             //low level
    {
        DAC->DATA[1].reg=HighLevel2;
        if(Repeats2)
        {
            if(--Repeats2==0)
                pPWM[1]->Start(false);
        }
    }
    TC4->COUNT32.INTFLAG.reg=0xff; //clear
}
}


Tc *glob_GetTcPtr(typeSamTC nTc);
CDacPWMht::CDacPWMht(PWM nPWM, const std::shared_ptr<CADmux>  &pMUX) :
    CSamTC(PWM1==nPWM ? typeSamTC::Tc0 : typeSamTC::Tc4)
{
    m_nPWM=nPWM;
    m_pMUX=pMUX;

    pPWM[nPWM]=this;

    //tune the timer:
    CSamTC::EnableAPBbus(true);

    //get clock:
    if(!m_pCLK)
    {
        m_pCLK=CSamCLK::Factory();
        CSamTC::ConnectGCLK(m_pCLK->CLKind());
    }

    Tc *pTc=glob_GetTcPtr(m_nTC);

    pTc->COUNT32.CTRLA.bit.MODE=2; //32bit
    pTc->COUNT32.WAVE.bit.WAVEGEN=1; //MFRQ: CC0=TOP
    pTc->COUNT32.INTENSET.reg=(TC_INTFLAG_MC0|TC_INTFLAG_MC1);
    CSamTC::EnableIRQ(true);

    pTc->COUNT32.CTRLA.bit.ENABLE=1; //enable
    pTc->COUNT32.CTRLBSET.bit.CMD=2; //keep it in the stopped state!
    m_pCLK->Enable(true);
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
    while(pTc->COUNT32.SYNCBUSY.bit.CTRLB){}
    if(bHow)
    {
        on_settings_changed();
        m_pMUX->SetDACmode(typeDACmode::SamAndExtDACs);
        synced_DAC_set(m_prmHighLevel); //start with high
        pTc->COUNT32.CTRLBSET.bit.CMD=1;    //start

    }
    else
    {
        pTc->COUNT32.CTRLBSET.bit.CMD=2;    //stop
        synced_DAC_set(0);      //clear
    }
}

void CDacPWMht::on_settings_changed()
{
    if(PWM1==m_nPWM)
    {
        Repeats1=m_prmRepeats;
        LowLevel1=m_prmLowLevel;
        HighLevel1=m_prmHighLevel;
    }
    else
    {
        Repeats2=m_prmRepeats;
        LowLevel2=m_prmLowLevel;
        HighLevel2=m_prmHighLevel;
    }
}
void CDacPWMht::impl_LoadNextHalfPeriod()
{
}
