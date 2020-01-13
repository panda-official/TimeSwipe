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
#include "nodeLED.h"

/*!
 * \brief Data visualization class: displays the measured signal levels of the ADC channel using the LED indicator. Like this it visualizes the actual measurement values.
 */
class CDataVis
{
protected:
    //! A brightness constant for calculating the actual LED brightness for visualization
    const float b_brght = 55.0;

    //! The upper visualization range boundary. The actual measuerement values are visualized within this range, which is constantly adapted. Correlates to max. brightness. 
    unsigned int meas_max;
    //! The lower visualization range boundary. The actual measuerement values are visualized within this range, which is constantly adapted. Correlates to min. brightness. 
    unsigned int meas_min;
    //! Min. visualization range. Is set around actual measurement value after startup and after a reset. 

    unsigned int min_wind = 100;
    //! Proportional factor for the adjustment of the visualization range boundaries 
    const float k_range = 0.004;

    /*!
     * \brief A time stamp when object state has been updated last time
     */
    unsigned long last_time_vis = 0;

    /*!
     * \brief A bool variable for initializing the visualization range with reset() function after startup
     */
    bool first_update = true;

    /*!
     * \brief The visualisation process is started
     */
    bool          m_bStarted=true;

    /*!
     * \brief "Start" order for initialization - to be executed in CDataVis::Update() method
     * \details Preparing/re-initialize object internals for visualization after issuing a CDataVis::Start method
     */
    bool          m_bStartInitOder=false;

    /*!
     * \brief State updation/recalculation period (for CDataVis::Update())
     */
    unsigned long m_upd_tspan_mS=1000;

    /*!
     * \brief A pointer to input data source
     */
    std::shared_ptr<CAdc> m_pADC;

    /*!
     * \brief A pointer to visualization LED to display processed data
     */
    std::shared_ptr<CLED> m_pLED;

public:
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
    CDataVis(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CLED> &pLED);

    /*!
     * \brief Starts/Stops the data visualization process
     * \param bHow true=Start, false=Stop
     * \param nDelay_mS A delay before process will be started after calling this method with bHow=true
     */
    void Start(bool bHow, unsigned long nDelay_mS);

    /*!
     * \brief The object state update method
     * \details Gets the CPU time to update internal state of the object.
     *  Must be called from a "super loop" or from corresponding thread
     */
    void Update();
};
