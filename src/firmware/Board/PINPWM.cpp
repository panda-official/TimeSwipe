/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#include "PINPWM.h"
#include "sam.h"


Tc *glob_GetTcPtr(typeSamTC nTc);
CPinPWM::CPinPWM(CSamPORT::group nGroup, CSamPORT::pin nPin) : CSamTC(typeSamTC::Tc6)
{
    m_prmPortGroup=nGroup;
    m_prmPortMask=(1L<<nPin);
    PORT->Group[nGroup].DIRSET.reg=m_prmPortMask;

    //tune the timer (32 bit, pair)
    CSamTC::EnableAPBbus(true);
    CSamTC::EnableAPBbus(static_cast<typeSamTC>(static_cast<int>(m_nTC)+1), true);

    m_pCLK=CSamCLK::Factory();
    CSamTC::ConnectGCLK(m_pCLK->CLKind());
    m_pCLK->Enable(true);

    CSamDMAC &dmac=CSamDMAC::Instance();

    m_pHLevDMAch=dmac.Factory();
    m_pHLevDMAch->SetupTrigger(CSamDMAChannel::trigact::BLOCK, CSamDMAChannel::trigsrc::TC6MC0);
    m_pHLevDMAch->AddBlock().Setup(&m_prmPortMask, (const void*)&(PORT->Group[nGroup].OUTSET.reg), 1, CSamDMABlock::beatsize::WORD32);
    m_pHLevDMAch->SetLoopMode();
    m_pHLevDMAch->Enable(true);

    m_pLLevDMAch=dmac.Factory();
    m_pLLevDMAch->SetupTrigger(CSamDMAChannel::trigact::BLOCK, CSamDMAChannel::trigsrc::TC6MC1);
    m_pLLevDMAch->AddBlock().Setup(&m_prmPortMask, (const void*)&(PORT->Group[nGroup].OUTCLR.reg), 1, CSamDMABlock::beatsize::WORD32);
    m_pLLevDMAch->SetLoopMode();
    m_pLLevDMAch->Enable(true);

    Tc *pTc=glob_GetTcPtr(m_nTC);

    pTc->COUNT32.CTRLA.bit.MODE=2; //32bit
    pTc->COUNT32.WAVE.bit.WAVEGEN=1; //MFRQ: CC0=TOP
    pTc->COUNT32.CC[0].reg=0xffff; //prevent IRQ on start
    pTc->COUNT32.CC[1].reg=0xffff;

    pTc->COUNT32.CTRLA.bit.ENABLE=1; //enable
    pTc->COUNT32.CTRLBSET.bit.CMD=2; //keep it in the stopped state!
}
void CPinPWM::set_pin(bool how)
{
    if(how)
        PORT->Group[m_prmPortGroup].OUTSET.reg=m_prmPortMask;
    else
        PORT->Group[m_prmPortGroup].OUTCLR.reg=m_prmPortMask;
}

void CPinPWM::on_obtain_half_periods()
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
            set_pin(true);
            pTc->COUNT32.CTRLBSET.bit.CMD=1;
        }
    }
}

void CPinPWM::impl_Start(bool bHow)
{
    Tc *pTc=glob_GetTcPtr(m_nTC);
    while(pTc->COUNT32.SYNCBUSY.bit.CTRLB){}

    if(bHow)
    {
        on_settings_changed();
        set_pin(true);
        pTc->COUNT32.CTRLBSET.bit.CMD=1;    //start
    }
    else
    {
        pTc->COUNT32.CTRLBSET.bit.CMD=2;    //stop
        set_pin(false);
    }
}

void CPinPWM::on_settings_changed()
{
}
void CPinPWM::impl_LoadNextHalfPeriod()
{
}



