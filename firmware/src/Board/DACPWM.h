/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include <memory>
#include "DAC.h"
#include "ADmux.h"
#include "PWM.h"

class CDacPWM : public CPWM<CDacPWM>
{
friend class CPWM;
protected:
    std::shared_ptr<CDac>    m_pDAC;
    std::shared_ptr<CADmux>  m_pMUX;

    void on_obtain_half_periods();
    void impl_Start(bool bHow);
    void impl_LoadNextHalfPeriod();

public:
    CDacPWM(const std::shared_ptr<CDac> &pDAC, const std::shared_ptr<CADmux>  &pMUX);
};
