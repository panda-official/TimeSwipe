/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "adcdac.hpp"
//#include "ADmux.h"
#include "../pin.hpp"
#include "../PWM.h"

#include <memory>

/// A PWM which's output is controlled by the DAC.
class CDacPWM final : public CPWM<CDacPWM> {
  friend CPWM;
private:
    /*!
     * \brief The pointer to the controlling DAC
     */
    std::shared_ptr<Dac_channel>    m_pDAC;

    /*!
     * \brief The pointer to the DAC mode switcher
     */
    std::shared_ptr<Pin>  m_pDACsw;

    /*!
     * \brief Called from the base class for additional specific actions for this class
     */
    void on_obtain_half_periods();

    /*!
     * \brief Implementation of the "Start" method: Starts or stops the generation
     * \param bHow true=start, flse=stop
     */
    void impl_Start(bool bHow);

    /*!
     * \brief Implementation of "LoadNextHalfPeriod" method: Sets corresponding output level via DAC.
     */
    void impl_LoadNextHalfPeriod();

public:
    /*!
     * \brief The class constructor
     * \param pDAC The pointer to the output controlling DAC
     * \param pMUX The pointer to the DAC mode switcher
     */
    CDacPWM(const std::shared_ptr<Dac_channel> &pDAC, const std::shared_ptr<Pin> &pDACsw);
};
