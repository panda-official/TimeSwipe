/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "bulkmes.h"
#include "nodeControl.h"
#include "os.h"
#include "SamADCcntr.h"

CADbulkMes::CADbulkMes()
{

}

void CADbulkMes::SetMeasMode(unsigned int nMode)
{
    m_nMeasMode=nMode;
}
void CADbulkMes::SetMeasChanMask(unsigned int nMask)
{
    m_nMeasMask=nMask;

    //__SetMeasRateHz(m_nMeasRate, false);
}
void CADbulkMes::SetMeasRateHz(unsigned int nRate)
{
    if(nRate<1)
        nRate=1;
    if(nRate>250000)
        nRate=250000;

    m_nMeasRate=nRate;

    //__SetMeasRateHz(nRate, false);
}
unsigned int CADbulkMes::__SetMeasRateHz(unsigned int nRate, bool bForce)
{
    if(nRate<1)
        nRate=1;

    //get corresponding ADC board:
    nodeControl &nc=nodeControl::Instance();

    size_t nChan=nc.GetMesChannelsCount();
    assert(nChan);
    CSamADCchan *pChan=dynamic_cast<CSamADCchan*>(&(nc.GetMesChannel(0)->ADC()));
    assert(pChan);

    unsigned int nActChans=0;
    for(size_t i=0; i<nChan; i++) if(m_nMeasMask&(1L<<i)){

        nActChans++;
    }
    if(0==nActChans)
    {
        m_nMeasRate=nRate;
        return 0;
    }

    CSamADCcntr *pCont=pChan->GetCont().get();
    unsigned int norm_rate=nRate*nActChans;
    unsigned int real_rate=pCont->SetSamplingRate(norm_rate, bForce);
    unsigned int av_cycles=real_rate/norm_rate;
    if(!av_cycles)
        av_cycles=1;

    //m_nMeasRate=(av_cycles*real_rate)/nActChans;

    return av_cycles;

}

void CADbulkMes::MeasStart(unsigned int nDuration)
{
    //preparing chans:
    nodeControl &nc=nodeControl::Instance();
    size_t nChan=nc.GetMesChannelsCount();
    m_DataBuf.reset();


    std::vector<typeSamADCmuxpos> ipos;
    std::vector<typeSamADCmuxneg> ineg;
    std::vector<int> mes_result;

    size_t nActChans=0;
    for(size_t i=0; i<nChan; i++) if(m_nMeasMask&(1L<<i))
    {
        nActChans++;

        nc.GetMesChannel(i)->SetMesMode(m_nMeasMode ? CMesChannel::mes_mode::Current : CMesChannel::mes_mode::Voltage);
      //  nc.GetMesChannel(i)->ADC().SelectAveragingMode(CAdc::averaging_mode::none); //get only raw values

        CSamADCchan *pChan=dynamic_cast<CSamADCchan*>(&(nc.GetMesChannel(i)->ADC()));
        assert(pChan);

        ipos.emplace_back(pChan->GetPosInput());
        ineg.emplace_back(pChan->GetNegInput());
        mes_result.emplace_back(0);
    }
    if(!nActChans)
        return;

   // mes_result.resize(nActChans);

    //board:
    CSamADCchan *pChan=dynamic_cast<CSamADCchan*>(&(nc.GetMesChannel(0)->ADC()));
    assert(pChan);
    CSamADCcntr *pCont=pChan->GetCont().get();
    assert(pCont);

    //save prev rate:
    unsigned int prev_rate=pCont->GetSamplingRate();

    //set rate:
    unsigned int nCycles=__SetMeasRateHz(m_nMeasRate, true);



    //start the measurements:
    unsigned long MeasStartTime=os::get_tick_mS();
    while(os::get_tick_mS()-MeasStartTime < nDuration)
    {

        for(unsigned int i=0; i<nCycles; i++){

            for(size_t ch=0; ch<nActChans; ch++){

                pCont->SelectInput(ipos[ch], ineg[ch]);
                mes_result[ch]+=pCont->SingleConv();
            }
        }

        //dump:
        for(size_t ch=0; ch<nActChans; ch++){

            int raw_meas_val=mes_result[ch]/nCycles;
            mes_result[ch]=0;
            m_DataBuf<<((raw_meas_val>>8)&0xff)<<(raw_meas_val&0xff);

        }
    }

    //restore the rate after finishing:
    pCont->SetSamplingRate(prev_rate);
}

int CADbulkMes::ReadBuffer(CFrmStream &Stream, unsigned int nStartPos, unsigned int nRead)
{
    m_DataBuf.rewind();

    int nAvCnt=m_DataBuf.in_avail()-static_cast<int>(nStartPos);
    if(nAvCnt<=0)
        return -1;

    unsigned int nCount = static_cast<unsigned int>(nAvCnt);
    if(nCount>nRead)
        nCount=nRead;

    for(unsigned int i=nStartPos, end=nStartPos+nCount; i<end; i++)
    {
        Stream.push(m_DataBuf[i]);
    }
    return static_cast<int>(nCount);

}

