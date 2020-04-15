/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


/*!
*   \file
*   \brief A definition file for
*   CFanControl
*/

#pragma once

#include "SamTempSensor.h"
#include "PINPWM.h"
#include "os.h"

/*!
 * \brief The class implements control of fan in PWM mode with several fixed speeds
 */
class CFanControl
{
protected:
    /*!
     * \brief The minimum temperature of the range
     */
    float m_TempMinC0;

    /*!
     * \brief The maximum temperature of the range
     */
    float m_TempMaxC0;

    /*!
     * \brief The temperature range (=max-min)
     */
    float m_TempRangeC0;

    /*!
     * \brief The minimum PWM frequency of the controlling range
     */
    unsigned int m_MinFreqHz;

    /*!
     * \brief The maximum PWM frequency of the controlling range
     */
    unsigned int m_MaxFreqHz;

    /*!
     * \brief The PWM frequency range (=max-min)
     */
    unsigned int m_FreqRangeHz;

    /*!
     * \brief The number of fan fixed speeds
     */
    unsigned int m_FanSpeeds=4;

    /*!
     * \brief The actuall set fan speed
     */
    unsigned int m_CurSpeed=0;

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

    /*!
     * \brief The pointer to the PWM-controlled fan pin
     */
    std::shared_ptr<CPinPWM>        m_pPWM;


public:
    /*!
     * \brief The class constructor
     * \param pTempSens - the pointer to the temperature sensor
     * \param pPWM      - the pointer to the PWM controlled fan pin
     * \param MinTempC0 - the minimum temperature of the range
     * \param MaxTempC0 - the maximum temperature of the range
     * \param MinFreqHz - the minimum PWM frequency of the controlling range
     * \param MaxFreqHz - the maximum PWM frequency of the controlling range
     */
    CFanControl(std::shared_ptr<CSamTempSensor> &pTempSens, std::shared_ptr<CPinPWM> &pPWM,
                float MinTempC0=20.0f, float MaxTempC0=60.0f,
                unsigned int MinFreqHz=1000, unsigned int MaxFreqHz=9000);

    /*!
     * \brief The object state update method
     * \details Implements controlling algorythm
     *  Must be called from a "super loop" or from corresponding thread
     */
    void Update();
};
