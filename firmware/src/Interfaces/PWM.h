/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "os.h"

/*!
*   @file
*   @brief A definition file for basic PWM interface:
*   CPWM
*
*/

/*!
 * \brief The PWM base class
 */
template <class T>
class CPWM
{
protected:

    /*!
     * \brief Pulse halh-period durations
     */
    unsigned long m_HalfPeriod_mS[2];

    /*!
     * \brief Time when current half-period has been started
     */
    unsigned long m_HalfPeriodStartTime;

    /*!
     * \brief Current half-period index
     */
    unsigned int m_CurHalfPeriodIndex;

    /*!
     * \brief Elapsed periods counter
     */
    unsigned int m_PeriodsCnt;

    /*!
     * \brief Generation status: true=started, false=stopped
     */
    bool m_bStarted=false;

    /*!
     * \brief PWM frequency setting
     */
    unsigned int m_prmFrequency=50;

    /*!
     * \brief Number of periods to generate. 0=infinite
     */
    unsigned int m_prmRepeats=0;

    /*!
     * \brief Duty cycle (pulse width) 0.001 - 0.999
     */
    float        m_prmDutyCycle=0.5f;

    /*!
     * \brief Output high level
     */
    int m_prmHighLevel=3072;

    /*!
     * \brief Output low level
     */
    int m_prmLowLevel=2048;

    /*!
     * \brief Output maximum possible value (setting limiter)
     */
    int m_prmLevelHighLim=4095;

    /*!
     * \brief Output minimum possible value (setting limiter)
     */
    int m_prmLevelLowLim=0;

    /*!
     * \brief Calculate half-periods duration based on current settings
     */
    void obtain_half_periods(){

        m_HalfPeriod_mS[0]=(1000.0f*m_prmDutyCycle)/m_prmFrequency;
        m_HalfPeriod_mS[1]=(1000.0f*(1.0f-m_prmDutyCycle))/m_prmFrequency;

        static_cast<T*>(this)->on_obtain_half_periods();
    }

public:

    /*!
     * \brief Returns frequncy setting
     * \return Curent frequency
     */
    unsigned int GetFrequency(){ return m_prmFrequency; }

    /*!
     * \brief Sets the frequency
     * \param Freq The frequency to set
     */
    void SetFrequency(unsigned int Freq){
        if(Freq<1)
            Freq=1;
        if(Freq>20000)
            Freq=20000;
        m_prmFrequency=Freq; obtain_half_periods();
    }

    /*!
     * \brief Returns number of periods to generate
     * \return
     */
    unsigned int GetRepeats(){ return m_prmRepeats; }

    /*!
     * \brief Sets number of periods to generate
     * \param Repeats The number of periods: 0=infinite, otherwise generate a burst of pulses=Repeats and then stop
     */
    void SetRepeats(unsigned int Repeats){ m_prmRepeats=Repeats; static_cast<T*>(this)->on_settings_changed(); }

    /*!
     * \brief Returns duty cycle (pulse width) setting
     * \return
     */
    float GetDutyCycle(){ return m_prmDutyCycle; }

    /*!
     * \brief Sets duty cycle (pulse width)
     * \param Duty The duty cycle (0.001-0.999)
     */
    void SetDutyCycle(float Duty){ if(Duty<0.001) Duty=0.001; if(Duty>0.999) Duty=0.999; m_prmDutyCycle=Duty; obtain_half_periods(); }

    /*!
     * \brief Returns the output high level
     * \return The output high level
     */
    int GetHighLevel(){ return m_prmHighLevel; }

    /*!
     * \brief Sets the output high level
     * \param Level The level to set
     */
    void SetHighLevel(int Level){
        if(Level<m_prmLevelLowLim) Level=m_prmLevelLowLim;
        if(Level>m_prmLevelHighLim) Level=m_prmLevelHighLim;
        m_prmHighLevel=Level;
        static_cast<T*>(this)->on_settings_changed();
    }

    /*!
     * \brief Returns the output low level
     * \return The output low level
     */
    int GetLowLevel(){ return m_prmLowLevel; }

    /*!
     * \brief Sets the output low level
     * \param Level The level to set
     */
    void SetLowLevel(int Level){
        if(Level<m_prmLevelLowLim) Level=m_prmLevelLowLim;
        if(Level>m_prmLevelHighLim) Level=m_prmLevelHighLim;
        m_prmLowLevel=Level;
        static_cast<T*>(this)->on_settings_changed();
    }

    /*!
     * \brief Is generation started?
     * \return true=started, false=stopped
     */
    bool IsStarted() {return m_bStarted;}

    /*!
     * \brief Starts or stops the generation
     * \param How true=start, false=stop
     */
    void Start(bool How){

        if(How)
        {
            if(How==m_bStarted) //little opt
                return;

            obtain_half_periods();
            m_CurHalfPeriodIndex=0;
            m_PeriodsCnt=0;
            m_HalfPeriodStartTime=os::get_tick_mS();
        }
        m_bStarted=How;
        static_cast<T*>(this)->impl_Start(How);
    }

    /*!
     * \brief Returns the time left for current half-period
     * \return The current half-period time left in mS
     */
    long GetHalfPeriodTimeLeft(){

        return m_HalfPeriod_mS[m_CurHalfPeriodIndex]-(os::get_tick_mS() - m_HalfPeriodStartTime);
    }

    /*!
     * \brief Call this method when current half period time is over.
     *  It will update control variables with the new values and set corresponding output level for the next half-period
     */
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

    /*!
     * \brief The method has to be called repeatedly when using the instance in a polling mode
     * (software generation, no timer interrupts)
     */
    void Update(){

        if(!m_bStarted)
            return;

        if(GetHalfPeriodTimeLeft()<=0)
            LoadNextHalfPeriod();
    }
};

