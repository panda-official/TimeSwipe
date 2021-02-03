/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#include "SamTempSensor.h"
#include "sam.h"
#include "NVMpage.h"

CSamTempSensor::CSamTempSensor(std::shared_ptr<CSamADCcntr> &pCont) :
    m_VTP(pCont, typeSamADCmuxpos::PTAT, typeSamADCmuxneg::none, 0.0f, 2.5f, false),
    m_VTC(pCont, typeSamADCmuxpos::CTAT, typeSamADCmuxneg::none, 0.0f, 2.5f, false)
{
    //get data from NVM Software Calibration Area and cash them:
    struct NVMscpage *pNVMpage=(NVMscpage *)NVMCTRL_SW0; //addr

    float TL=pNVMpage->TLI + 0.1f * pNVMpage->TLD;
    float TH=pNVMpage->THI + 0.1f * pNVMpage->THD;

    m_TLVPH_THVPL =TL*pNVMpage->VPH - TH*pNVMpage->VPL;
    m_THVCL_TLVCH =TH*pNVMpage->VCL - TL*pNVMpage->VCH;
    m_VCL_VCH   =pNVMpage->VCL - pNVMpage->VCH;
    m_VPH_VPL   =pNVMpage->VPH - pNVMpage->VPL;

    //turn the SUP:
    SUPC->VREF.bit.TSEN=1;
}

void CSamTempSensor::Update()
{
    //!here we assume that  the SUPC is not in on-demand mode:

    //select TP:
    SUPC->VREF.bit.TSSEL=0;
    float TP=m_VTP.DirectMeasure(20, 0.7);

    //select TC:
    SUPC->VREF.bit.TSSEL=1;
    float TC=m_VTC.DirectMeasure(20, 0.7);

    //calculation:
    m_MeasuredTempCD=( TC*m_TLVPH_THVPL + TP*m_THVCL_TLVCH )/( TP*m_VCL_VCH + TC*m_VPH_VPL );
}
