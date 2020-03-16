/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include <memory>
#include "DAC.h"
#include "PWM.h"
#include "ADmux.h"
#include "SamCLK.h"
#include "SamTC.h"


class CDacPWMht : public CPWM<CDacPWMht>, public CSamTC
{
friend class CPWM;
public:
    enum PWM{

        PWM1,
        PWM2
    };

protected:
    PWM m_nPWM;
    std::shared_ptr<CADmux>  m_pMUX;

    /*!
     * \brief An associated clock generator
     */
    static std::shared_ptr<CSamCLK> m_pCLK;

    /*!
     * \brief Called from the base class for additional specific actions for this class
     */
    void on_obtain_half_periods();

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

    void synced_DAC_set(unsigned int nLevel);


public:
    CDacPWMht(PWM nPWM, const std::shared_ptr<CADmux>  &pMUX);
};

