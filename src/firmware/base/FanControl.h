/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "PINPWM.h"
#include "../os.h"
#include "sam/SamTempSensor.h"

/**
 * @brief A control of fan in PWM mode with several fixed speeds.
 */
class CFanControl final {
private:
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
     * \brief The minimum PWM Duty Cycle of the controlling range
     */
    float m_MinDuty;

    /*!
     * \brief The maximum PWM Duty Cycle of the controlling range
     */
    float m_MaxDuty;

    /*!
     * \brief The PWM duty cycle range (=max-min)
     */
    float m_DutyRange;

    /*!
     * \brief The number of fan fixed speeds
     */
    unsigned int m_FanSpeeds=10;

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
    unsigned long m_upd_quant_mS=5000;


    bool m_enabled = true;

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
     * \param MinDuty   - the minimum Duty Cycle of the controlling range
     * \param MaxDuty   - the maximum Duty Cycle of the controlling range
     */
    CFanControl(std::shared_ptr<CSamTempSensor> &pTempSens, std::shared_ptr<CPinPWM> &pPWM,
                float MinTempC0=45.0f, float MaxTempC0=65.0f,
                float MinDuty=0.5, float MaxDuty=1.0);

    bool GetEnabled() const noexcept;
    void SetEnabled(bool enabled);

    /*!
     * \brief The object state update method
     * \details Implements controlling algorythm
     *  Must be called from a "super loop" or from corresponding thread
     */
    void Update();
};
