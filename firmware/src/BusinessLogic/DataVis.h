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
 * \brief Data visualization class: displays the ADC channel's measured signal levels by binded LED
 */
class CDataVis
{
protected:
    //! Color codes for DMS board
    unsigned int col_DMS[3] = {24, 250, 208};

    //! Color codes for IEPE board
    unsigned int col_IEPE[3] = {50, 151, 247};

    //! Actual displayed color
    unsigned int col_act[3];

    //parameters:

    const float b_brght = 55.0; //should it be const?

    unsigned int meas_max = 2048.0;
    unsigned int meas_min = 2048.0;
    unsigned int min_wind = 100;
    const float k_range = 0.004;

    unsigned long last_time_vis = 0;
    bool first_update = true;

    bool          m_bStarted=true;
    bool          m_bStartInitOder=false;
    unsigned long m_upd_tspan_mS=1000;

    std::shared_ptr<CAdc> m_pADC; //data source
    std::shared_ptr<CLED> m_pLED; //output led

public:
    //helpers:
    void reset();

public:
    CDataVis(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CLED> &pLED);

    void Start(bool bHow, unsigned long nDelay_mS);
    void Update(); //updation
};
