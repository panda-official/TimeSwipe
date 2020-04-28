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
    //! A brightness constant for calculating the actual LED brightness for visualization
    static const constexpr float b_brght = 7.0f;//2.25f;
    static const constexpr float bright_factor=1/(b_brght-1.0f);

    const float ILowLim=0.02f;

    //! The upper visualization range boundary. The actual measuerement values are visualized within this range, which is constantly adapted. Correlates to max. brightness. 
   /* int meas_max;
    //! The lower visualization range boundary. The actual measuerement values are visualized within this range, which is constantly adapted. Correlates to min. brightness. 
    int meas_min;
    //! Min. visualization range at start. Is set around actual measurement value after startup and after a reset. The measurement value has to surpass this range (+/-50) one time, for the visualization to become active for this channel.
    int min_wind = 100;
    //! Min. distance of the lower and upper boarder to the actual measurement value. Two times this value gives the min. range, when the visualization is in progress. The reset of the visualization boarders stops at this distance from the measurment value. 
    int min_dist = 30;
    //! Proportional factor for the adjustment of the visualization range boundaries 
    const float k_range = 0.004;
    //! Boolean variable for saving the activation status of the visualization for the four measurement channels.
    bool senscon_chan=false;

    //! Drop-out factor: used to determine the sensor is disconnected
    const float drop_out_factor=2.0f;*/


    /*!
     * \brief A time stamp when object state has been updated last time
     */
    unsigned long last_time_vis = 0;

    /*!
     * \brief A bool variable for initializing the visualization range with reset() function after startup
     */
    bool first_update = true;


    /*!
     * \brief State updation/recalculation period (for CDataVis::Update())
     */
     long m_upd_tspan_mS=25;

    /*!
     * \brief A pointer to input data source
     */
    std::shared_ptr<CAdc> m_pADC;

    /*!
     * \brief A pointer to visualization LED to display processed data
     */
    CView::vischan m_nCh;

    CMA<float> m_MA;
    float m_CurStdDev;
    float m_HalfRange;
    const float m_InflationFactor=1.5f;
    const int m_StdDevPer=20;
    int m_StdDevRecalcCountDown=0;
    bool m_bSensorDetected=false;
    const float m_DetectThrhold=70.0f;
    const float m_DropThrhold=70.0f;

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
    CDataVis(const std::shared_ptr<CAdc> &pADC, CView::vischan nCh);

    /*!
     * \brief The object state update method
     * \details Gets the CPU time to update internal state of the object.
     *  Must be called from a "super loop" or from corresponding thread
     */
    void Update();
};
