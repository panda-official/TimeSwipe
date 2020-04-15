/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CFanControlSimple
*/

#pragma once

#include "SamTempSensor.h"
#include"SamPORT.h"
#include "os.h"

/*!
 * \brief The class implements simple control of fan in ON/OFF mode
 * \details Two temperature thresholds set: when higher one is exceeded fan is started, when temperature drops below lower threshold fan is stopped
 */
class CFanControlSimple
{
protected:

    /*!
     * \brief Higher temperature threshold
     */
    float m_TempOnC0;

    /*!
     * \brief Lower temperature threshold
     */
    float m_TempOffC0;

    /*!
     * \brief Port Group of the fan control pin
     */
    CSamPORT::group m_PortGroup;

    /*!
     * \brief Port Pin of the fan control pin
     */
    CSamPORT::pin   m_PortPin;

    /*!
     * \brief Last time when update() method was called, mSec
     */
    unsigned long m_last_time_upd_mS=os::get_tick_mS();

    /*!
     * \brief Minimum time between two consecutive updates
     */
    unsigned long m_upd_quant_mS=200;

    /*!
     * \brief The pointer to the temperature sensor
     */
    std::shared_ptr<CSamTempSensor> m_pTempSens;


public:
    /*!
     * \brief The class constructor
     * \param pTempSens - the pointer to the temperature sensor
     * \param nGroup - Port Group of the fan control pin
     * \param nPin - Port Pin of the fan control pin
     * \param TempOnC0 - higher temperature threshold
     * \param TempOffC0 - lower temperature threshold
     */
    CFanControlSimple(std::shared_ptr<CSamTempSensor> &pTempSens, CSamPORT::group nGroup, CSamPORT::pin nPin,
                float TempOnC0=40.0f, float TempOffC0=35.0f);

    /*!
     * \brief The object state update method
     * \details Implements controlling algorythm
     *  Must be called from a "super loop" or from corresponding thread
     */
    void Update();
};
