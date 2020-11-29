/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "bulkmes.h"
#include "nodeControl.h"
#include "os.h"

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
}
void CADbulkMes::SetMeasRateHz(unsigned int nRate)
{
    if(nRate<1)
        nRate=1;
    m_nMeasRate=nRate;
}

void CADbulkMes::MeasStart(unsigned int nDuration)
{
    //preparing chans:
    nodeControl &nc=nodeControl::Instance();
    size_t nChan=nc.GetMesChannelsCount();
    unsigned long cycle_delay_us=1000000/m_nMeasRate;
    m_DataBuf.reset();

    for(size_t i=0; i<nChan; i++) if(m_nMeasMask&(1L<<i))
    {
        nc.GetMesChannel(i)->SetMesMode(m_nMeasMode ? CMesChannel::mes_mode::Current : CMesChannel::mes_mode::Voltage);
        nc.GetMesChannel(i)->ADC().SelectAveragingMode(CAdc::averaging_mode::none); //get only raw values
    }

    //start the measurements:
    unsigned long MeasStartTime=os::get_tick_mS();
    while(os::get_tick_mS()-MeasStartTime < nDuration)
    {

        //single measure:
        for(size_t i=0; i<nChan; i++) if(m_nMeasMask&(1L<<i))
        {
            int raw_meas_val=nc.GetMesChannel(i)->ADC().DirectMeasure();
            m_DataBuf<<((raw_meas_val>>8)&0xff)<<(raw_meas_val&0xff);       //push the data
        }

        //wait for a while (rate)
        os::uwait(cycle_delay_us);

    }

    for(size_t i=0; i<nChan; i++) if(m_nMeasMask&(1L<<i))
    {
        nc.GetMesChannel(i)->ADC().SelectAveragingMode(CAdc::averaging_mode::ch_default); //return to the channel default mode
    }


    //test function: just fills the buffer with the data
    /*for(unsigned int r=0; r<nDuration; r++)
    {
        for(int i=0; i<10; i++)
        {
            m_DataBuf<<(0x30+i);
        }
    }*/
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

