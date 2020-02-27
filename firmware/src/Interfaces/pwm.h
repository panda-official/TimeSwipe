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
    int m_CurHalfPeriodIndex;

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

    void obtain_half_periods();

public:
    unsigned int GetFrequency(){ return m_prmFrequency; }
    void SetFrequency(unsigned int Freq){m_prmFrequency=Freq; obtain_half_periods(); }
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
            m_HalfPeriodStartTime=os::get_tick_mS();
        }
    }
    long GetHalfPeriodTimeLeft(){

        return (os::get_tick_mS() - m_HalfPeriodStartTime);
    }
    void LoadNextHalfPeriod(){

        ++m_CurHalfPeriodIndex&=1;
        m_HalfPeriodStartTime=os::get_tick_mS();
    }
    void Update(){

        if(!m_bStarted)
            return;

        if(GetHalfPeriodTimeLeft()<=0)
            LoadNextHalfPeriod();
    }
};

