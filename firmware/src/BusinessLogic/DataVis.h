/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CDataVis
*/

#pragma once

#include <memory>
#include "ADC.h"
#include "View.h"
#include "mav.h"

/*!
 * \brief Data visualization class: displays the measured signal levels of the ADC channel using the LED indicator. Like this it visualizes the actual measurement values.
 * \details The intensity normalized value is calculated as I=(B^x-1)/(B-1), where x [0, 1]
 *
 * https://www.mikrocontroller.net/articles/LED-Fading
 *
 * To get Intensity in the middle x=0.5 the equation I=(B^0.5-1)/(B-1) has to be solved.
 * The roots are: B=( ( 1+- sqrt(1- 4I(1-I)) )/2I )^2
 * For I=0.4 the larger root is 2.25
*/
class CDataVis
{
protected:

    /*!
     * \brief The sum of raw ADC values for m_AvPeriod
     */
    float m_AvSumm=0;

    /*!
     * \brief The counter used for initial averaging of raw ADC values
     */
    int   m_MesCounter=0;

    /*!
     * \brief The period of the initial averaging
     */
    static constexpr int m_AvPeriod=12;


    //! The brightness constant for calculating the actual LED brightness for visualization
    static constexpr float b_brght = 7.0f;

    /*!
     * \brief Pre-calculated brightness factor
     */
    static constexpr float bright_factor=1/(b_brght-1.0f);

    /*!
     * \brief Normalized intensity low limit (prevents LED flickering)
     */
    const float ILowLim=0.02f;


    /*!
     * \brief The time stamp when object state has been updated last time
     */
    unsigned long last_time_vis = 0;

    /*!
     * \brief The bool variable for initializing the visualization range with reset() function after startup
     */
    bool first_update = true;


    /*!
     * \brief State updation/recalculation period (for CDataVis::Update())
     */
     long m_upd_tspan_mS=1;

    /*!
     * \brief The pointer to input data source
     */
   // std::shared_ptr<CAdc> m_pADC;

    /*!
     * \brief The pointer to visualization LED to display processed data
     */
    CView::vischan m_nCh;


    /*!
     * \brief The moving average of the input signal
     */
    CMA<float> m_MA;

    /*!
     * \brief Current Standard Deviation of the input signal
     */
    float m_CurStdDev;

    /*!
     * \brief The half range of the signal range dynamic window = Standard Deviation x Inflation Factor
     */
    float m_HalfRange;


    /*!
     * \brief The Inflation factor used to calculate signal range dynamic window
     */
    const float m_InflationFactor=1.5f;

    /*!
     * \brief The period of the Standard Deviation
     */
    const int m_StdDevPer=20;

    /*!
     * \brief The countdown to Standard Deviation calculation (to not calculate it on every update, but after some updates)
     */
    int m_StdDevRecalcCountDown=0;

    /*!
     * \brief The sensor detected flag
     */
    bool m_bSensorDetected=false;

    /*!
     * \brief The sensor detection threshold: abs(Signal-SignalMA)>m_DetectThrhold
     */
    const float m_DetectThrhold=70.0f;

    /*!
     * \brief The sensor drop out threshold: abs(ZeroLevel-SignalMA)<m_DropThrhold && Standard Deviation<m_DropThrhold
     */
    const float m_DropThrhold=70.0f;

    /*!
     * \brief Assumed default(Zero) signal level when no sensor is connected
     */
    float m_ZeroLevel=2048.0f;

    /*!
     * \brief Resets internal state of the object by setting the min. visualization range (min_wind) around the actual measurement value
     */
    void reset();

public:
    /*!
     * \brief The class constructor
     * \param pADC A pointer to an ADC channel
     * \param pLED A pointer to a LED
     */
    CDataVis(CView::vischan nCh);

    inline CView::vischan GetVisChannel(){ return m_nCh; }

    /*!
     * \brief The object state update method
     * \details Gets the CPU time to update internal state of the object.
     *  Must be called from a "super loop" or from corresponding thread
     */
    void Update(float InputValue);
};
