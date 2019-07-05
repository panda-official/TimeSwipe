#include "zerocal.h"
#include <math.h>


void CADpointSearch::Update()
{
    if(typePTsrcState::searching!=m_State)
        return;

    float CurPoint=m_pADC->GetRealVal();
    float CurSetPoint=m_pDAC->GetRealVal();
    float err=(m_TargPoint-CurPoint);
    bool  bErrSign=std::signbit(err);

    m_MesCnt++;
    if(m_MesCnt>1)
    {
        //correction:
        float dp=(CurPoint-m_LastPoint);
        float ds=(CurSetPoint-m_LastSetPoint);
        //float k=(CurPoint-m_LastPoint)/(CurSetPoint-m_LastSetPoint);

        if(std::abs(ds)>0.25){ //trhold - dbg only - prevent an error

        float k=ds/dp;
        if(std::abs(k)>m_k_limit) //this cannot be -> error
        {
            m_State=typePTsrcState::error;
            return;
        }
        m_k=k;
        }
    }
    else
    {
        m_bLastErrSign=bErrSign;
    }

    //prediction:
    float Amp=err*m_k*m_Q; //02.05.2019
    float ModAmp=fabs(Amp);
    if(m_MesCnt>1)
    {
        //check proc end conditions:
        if(std::abs(err)<=m_TargErrTolerance && ModAmp<=m_MinAmpVal)
        {
            int raw_val=m_pADC->GetRawBinVal(); //for testing...
            m_State=typePTsrcState::found;
            return;
        }

        if( (ModAmp>m_LastAmp || bErrSign!=m_bLastErrSign) && ModAmp>m_MinAmpVal)
        {
            m_Q*=0.5f; //kill reso
            Amp*=0.5f;
            m_LastAmp=ModAmp; //change this only once
            m_bLastErrSign=bErrSign;
        }
    }
    else
    {
        m_LastAmp=ModAmp;
    }

    float  NewSetpoint=CurSetPoint+Amp;
    m_pDAC->SetVal(NewSetpoint);

    //m_LastAmp=fabs(Amp); //????
    m_LastPoint=CurPoint;
    m_LastSetPoint=CurSetPoint;
}

#include "menu_logic.h"
void CCalMan::Start()
{
    unsigned int nSize=m_ChanCal.size();
    for(unsigned int i=0; i<nSize; i++)
    {
        m_ChanCal[i].Search(0.0f);
        m_pLED[i]->ON(true); //!!!!
        m_pLED[i]->SetBlinkMode(true);
        m_pLED[i]->SetColor(CMenuLogic::SETZERO_COLOR_ACTIVE);
    }
}
 void CCalMan::StopReset()
 {
     unsigned int nSize=m_ChanCal.size();
     for(unsigned int i=0; i<nSize; i++)
     {
         m_ChanCal[i].StopReset();
         m_pLED[i]->ON(false);
     }
 }

unsigned long get_tick_mS(void);
void CCalMan::Update()
{
    unsigned long cur_time=get_tick_mS();
    if( cur_time-m_LastTimeUpd < 150)
        return;
    m_LastTimeUpd=cur_time;

    unsigned int nSize=m_ChanCal.size();
    for(unsigned int i=0; i<nSize; i++)
    {
        m_ChanCal[i].Update();
        typePTsrcState tstate=m_ChanCal[i].state();
        if(tstate!=m_State[i])
        {
            m_State[i]=tstate;
            switch (tstate)
            {
                case typePTsrcState::error:
                    m_pLED[i]->SetBlinkMode(false);
                    m_pLED[i]->SetColor(LEDrgb(255,0,0));
                break;

                case typePTsrcState::found:
                    m_pLED[i]->SetBlinkMode(false);
                    m_pLED[i]->SetColor(CMenuLogic::SETZERO_COLOR_ACTIVE);
            }
        }
    }
}
