/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "../../debug.hpp"
#include "SamADCcntr.h"
#include "NVMpage.h"

#include <sam.h>

#define SELECT_SAMADC(x) (x==(typeSamADC::Adc0) ? ADC0:ADC1);


#define GCLK_ADC0   40
#define GCLK_ADC1   41


CSamADCchan::CSamADCchan(std::shared_ptr<CSamADCcntr> &pCont, typeSamADCmuxpos posIN, typeSamADCmuxneg negIN, bool bAutoUpd)
{
    m_pCont=pCont;
    m_posIN=posIN;
    m_negIN=negIN;

    if(bAutoUpd)
    {
        pCont->m_Chans.push_back(this);
    }
}
CSamADCchan::~CSamADCchan()
{
    m_pCont->m_Chans.remove(this);
}

void CSamADCchan::handle_measurement(const short raw)
{
  // m_UnfilteredRawVal=RawVal;
  filtered_raw_ += (static_cast<float>(raw) - filtered_raw_)
    * static_cast<float>(data_age()) / filter_time_ms_;
  m_MesTStamp = os::get_tick_mS();
}

int CSamADCchan::DirectMeasure(const int nMesCnt, const float alpha) const noexcept
{
  // Select a channel and no switching beetwen mes.
  m_pCont->SelectInput(m_posIN, m_negIN);

  // Measure and average.
  float val = m_pCont->SingleConv();
  for (int i{}; i < nMesCnt; ++i)
    val = alpha*val + (1.0f - alpha)*m_pCont->SingleConv();
  return val;
}

bool CSamADCcntr::Update()
{
  for (auto* const pCh: m_Chans) {
    if (pCh->data_age() >= 1) {
      SelectInput(pCh->m_posIN, pCh->m_negIN);
      const short mes{SingleConv()};
      pCh->handle_measurement(mes);
    }
  }
  return true;
}

void CSamADCcntr::SelectInput(typeSamADCmuxpos nPos, typeSamADCmuxneg nNeg)
{
    //select ptr:
    Adc *pADC=SELECT_SAMADC(m_nADC);

    while(pADC->SYNCBUSY.bit.INPUTCTRL){}
    ADC_INPUTCTRL_Type treg;
    treg.reg=pADC->INPUTCTRL.reg;

    treg.bit.MUXPOS=(int)nPos;
    if(typeSamADCmuxneg::none!=nNeg)
    {
        treg.bit.MUXNEG=(int)nNeg;
        treg.bit.DIFFMODE=1;
    }
    else
    {
        treg.bit.MUXNEG=0;
        treg.bit.DIFFMODE=0;
    }
    pADC->INPUTCTRL.reg=treg.reg;
}
short CSamADCcntr::SingleConv()
{
    //select ptr:
    Adc *pADC=SELECT_SAMADC(m_nADC);

    //trigger the sconv:
    while(pADC->SYNCBUSY.bit.SWTRIG){}
    pADC->SWTRIG.bit.START=1;


    //wait until finished:
    while(0==pADC->INTFLAG.bit.RESRDY && 0==pADC->INTFLAG.bit.OVERRUN){}

    return (pADC->RESULT.bit.RESULT); //2-compl code
}

CSamADCcntr::CSamADCcntr(typeSamADC nADC)
{
    m_nADC=nADC;

    //select ptr:
    Adc *pADC=SELECT_SAMADC(nADC);


    //------------------------setup PINs---------------------------------
    //PA04 -> group 0, even, function "B"(ADC)=0x01: ANAREF (VREFB) AIN4
    PORT->Group[0].PMUX[2].bit.PMUXE=0x01;
    PORT->Group[0].PINCFG[4].bit.PMUXEN=1; //enable

    //PA06 -> group 0, even, function "B"(ADC)=0x01   ADC0/AIN6 (VREFC) ADC3
    PORT->Group[0].PMUX[3].bit.PMUXE=0x01;
    PORT->Group[0].PINCFG[6].bit.PMUXEN=1; //enable

    //PA07 -> group 0, odd, function "B"(ADC)=0x01   ADC0/AIN7 ADC4
    PORT->Group[0].PMUX[3].bit.PMUXO=0x01;
    PORT->Group[0].PINCFG[7].bit.PMUXEN=1; //enable

    //PB08 -> group 1, even, function "B"(ADC)=0x01   ADC0/AIN2 ADC1
    PORT->Group[1].PMUX[4].bit.PMUXE=0x01;
    PORT->Group[1].PINCFG[8].bit.PMUXEN=1; //enable

    //PB09 -> group 1, odd, function "B"(ADC)=0x01   ADC0/AIN3 ADC2
    PORT->Group[1].PMUX[4].bit.PMUXO=0x01;
    PORT->Group[1].PINCFG[9].bit.PMUXEN=1; //enable

    //-------------------------------------------------------------------


    //------------------enable main clock to drive ADC bus---------------
    if(typeSamADC::Adc0==nADC)
        MCLK->APBDMASK.bit.ADC0_=1;
    else
        MCLK->APBDMASK.bit.ADC1_=1;
    //-------------------------------------------------------------------

    //--------------------------calibrating------------------------------
    struct NVMscpage *pNVMpage=(NVMscpage *)NVMCTRL_SW0; //addr
    if(typeSamADC::Adc0==nADC)
    {
        ADC0->CALIB.bit.BIASREFBUF  =pNVMpage->ADC0_BIASREFBUF;
        ADC0->CALIB.bit.BIASR2R     =pNVMpage->ADC0_BIASR2R;
        ADC0->CALIB.bit.BIASCOMP    =pNVMpage->ADC0_BIASCOMP;
    }
    else
    {
        ADC1->CALIB.bit.BIASREFBUF  =pNVMpage->ADC1_BIASREFBUF;
        ADC1->CALIB.bit.BIASR2R     =pNVMpage->ADC1_BIASR2R;
        ADC1->CALIB.bit.BIASCOMP    =pNVMpage->ADC1_BIASCOMP;
    }
    //-------------------------------------------------------------------

    //----------------------connect default gen--------------------------
    m_pCLK = Sam_clock_generator::make();
    PANDA_TIMESWIPE_ASSERT(m_pCLK);

    int pchind=typeSamADC::Adc0==m_nADC ? GCLK_ADC0:GCLK_ADC1;
    GCLK->PCHCTRL[pchind].bit.GEN=static_cast<uint32_t>(m_pCLK->id());
    GCLK->PCHCTRL[pchind].bit.CHEN=1;

     m_pCLK->enable(true);
    //------------------------------------------------------------------


    //----------------enabling accumulation & averaging-----------------
        pADC->AVGCTRL.bit.SAMPLENUM=0x07; //128 samples
        while(pADC->SYNCBUSY.bit.AVGCTRL){}
        pADC->AVGCTRL.bit.ADJRES=0x04;      //12 bits res of 128 samples
        while(pADC->SYNCBUSY.bit.AVGCTRL){}
        pADC->CTRLB.bit.RESSEL=0x01; //16BIT For averaging mode output
        while(pADC->SYNCBUSY.bit.CTRLB){}
    //-------------------------------------------------------------------


    //--------------------------enabling---------------------------------
    pADC->REFCTRL.bit.REFSEL    =0x05; //AREFB
    while(pADC->SYNCBUSY.bit.REFCTRL){}
    pADC->CTRLA.bit.ENABLE=1;   //enable the ADC
    while(pADC->SYNCBUSY.bit.ENABLE){}
    //-------------------------------------------------------------------
}
