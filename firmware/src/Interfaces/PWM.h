/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "os.h"
template <class T>
class CPWM
{
protected:

    unsigned long m_HalfPeriod_mS[2];
    unsigned long m_HalfPeriodStartTime;
    unsigned int m_CurHalfPeriodIndex;
    unsigned int m_PeriodsCnt;

    bool m_bStarted=false;

    //-------------
    unsigned int m_prmFrequency=50;
    unsigned int m_prmRepeats=0;
    float        m_prmDutyCycle=0.5f;
    int m_prmHighLevel=2000;
    int m_prmLowLevel=0;

    int m_prmLevelHighLim=4095;
    int m_prmLevelLowLim=0;
    //-------------

    void obtain_half_periods(){

        m_HalfPeriod_mS[0]=(1000.0f*m_prmDutyCycle)/m_prmFrequency;
        m_HalfPeriod_mS[1]=(1000.0f*(1.0f-m_prmDutyCycle))/m_prmFrequency;

        static_cast<T*>(this)->on_obtain_half_periods();
    }

public:
    unsigned int GetFrequency(){ return m_prmFrequency; }
    void SetFrequency(unsigned int Freq){
        if(Freq<1)
            Freq=1;
        if(Freq>1000)
            Freq=1000;
        m_prmFrequency=Freq; obtain_half_periods();
    }
    unsigned int GetRepeats(){ return m_prmRepeats; }
    void SetRepeats(unsigned int Repeats){ m_prmRepeats=Repeats; }
    float GetDutyCycle(){ return m_prmDutyCycle; }
    void SetDutyCycle(float Duty){ if(Duty<0.001) Duty=0.001; if(Duty>0.999) Duty=0.999; m_prmDutyCycle=Duty; obtain_half_periods(); }
    int GetHighLevel(){ return m_prmHighLevel; }
    void SetHighLevel(int Level){
        if(Level<m_prmLevelLowLim) Level=m_prmLevelLowLim;
        if(Level>m_prmLevelHighLim) Level=m_prmLevelHighLim;
        m_prmHighLevel=Level;
    }
    int GetLowLevel(){ return m_prmLowLevel; }
    void SetLowLevel(int Level){
        if(Level<m_prmLevelLowLim) Level=m_prmLevelLowLim;
        if(Level>m_prmLevelHighLim) Level=m_prmLevelHighLim;
        m_prmLowLevel=Level;
    }

    bool IsStarted() {return m_bStarted;}
    void Start(bool How){

        m_bStarted=How;
        if(How)
        {
            obtain_half_periods();
            m_CurHalfPeriodIndex=0;
            m_PeriodsCnt=0;
            m_HalfPeriodStartTime=os::get_tick_mS();
        }
        static_cast<T*>(this)->impl_Start(How);
    }
    long GetHalfPeriodTimeLeft(){

        return m_HalfPeriod_mS[m_CurHalfPeriodIndex]-(os::get_tick_mS() - m_HalfPeriodStartTime);
    }
    void LoadNextHalfPeriod(){

        if(m_CurHalfPeriodIndex){

           if(m_prmRepeats){

               if(++m_PeriodsCnt>=m_prmRepeats){

                   Start(false);
                   return;
               }
           }
           m_CurHalfPeriodIndex=0;
        }
        else{

           m_CurHalfPeriodIndex=1;
        }
        m_HalfPeriodStartTime=os::get_tick_mS();
        static_cast<T*>(this)->impl_LoadNextHalfPeriod();
    }
    void Update(){

        if(!m_bStarted)
            return;

        if(GetHalfPeriodTimeLeft()<=0)
            LoadNextHalfPeriod();
    }
};

