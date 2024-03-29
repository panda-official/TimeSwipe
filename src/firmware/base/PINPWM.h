/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "../PWM.h"
#include "sam/clock_generator.hpp"
#include "sam/SamTC.h"
#include "sam/SamDMAC.h"
#include "sam/pin.hpp"

#include <cstdint>
#include <memory>

/**
 * @brief A PWM which output is controlled by the PIN with DMA support.
 *
 * @details The class is designed to generate PWM without using of CPU time.
 */
class CPinPWM final : public CPWM<CPinPWM>, public CSamTC {
public:
    /*!
     * \brief The class constructor
     * \param nGroup - Port Group of the fan control pin
     * \param nPin - Port Pin of the fan control pin
     */
    CPinPWM(Sam_pin::Group nGroup, Sam_pin::Number nPin);

private:
    friend CPWM;

    /*!
     * \brief The 32-bit variable holding the PORT output mask
     */
    uint32_t m_prmPortMask;

    /*!
     * \brief Port Group of the fan control pin
     */
    Sam_pin::Group m_prmPortGroup;

    /*!
     * \brief The DMA channel used to map m_prmHighLevel16 onto the DAC
     */
    std::shared_ptr<CSamDMAChannel> m_pHLevDMAch;

    /*!
     * \brief The DMA channel used to map m_prmLowLevel16 onto the DAC
     */
    std::shared_ptr<CSamDMAChannel> m_pLLevDMAch;

    /*!
     * \brief An associated clock generator
     */
    std::shared_ptr<Sam_clock_generator> m_pCLK;

    /*!
     * \brief Called from the base class for additional specific actions for this class
     */
    void on_obtain_half_periods();

    /*!
     * \brief Called from the base class to apply changed settings
     */
    void on_settings_changed();

    /*!
     * \brief Implementation of the "Start" method: Starts or stops the generation
     * \param bHow true=start, flse=stop
     */
    void impl_Start(bool bHow);

    /*!
     * \brief Implementation of "LoadNextHalfPeriod" method: Sets corresponding output level via DAC.
     */
    void impl_LoadNextHalfPeriod();

    /*!
     * \brief Turns the fan control pin ON/OFF
     * \param how - ON=true, OFF=false
     */
    void set_pin(bool how);
};
