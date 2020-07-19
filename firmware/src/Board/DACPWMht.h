/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CDacPWMht
*/


#pragma once

#include <stdint.h>
#include <memory>
#include "Pin.h"
#include "DAC.h"
#include "PWM.h"
#include "SamCLK.h"
#include "SamTC.h"
#include "SamDMAC.h"

/*!
 * \brief The class implements a PWM which output is controlled by the DAC with DMA or timer IRQ support
 * \details The class is designed to generate PWM without using of CPU time
 */
class CDacPWMht : public CPWM<CDacPWMht>, public CSamTC
{
friend class CPWM;
public:

    /*!
     * \brief The PWM instance index (PWM1 or PWM2)
     */
    enum PWM{

        PWM1,
        PWM2
    };

    /*!
     * \brief The generation mode
     */
    enum mode{

        DMA,    //!<DMA mode: CortexM4 core is not involved in generation, only peripherals are working
        IRQ     //!<IRQ mode: CortexM4 core timer interrupt handlers are used to set DAC output
    };

protected:

    /*!
     * \brief The PWM index of the instance
     */
    PWM m_nPWM;

    /*!
     * \brief The pointer to the DAC mode switcher
     */
    std::shared_ptr<CPin>  m_pDACsw;

    /*!
     * \brief The 16-bit variable holding the HighLevel of PWM output to be mapped onto the DAC by the DMA
     */
    uint16_t m_prmHighLevel16;

    /*!
     * \brief The 16-bit variable holding the LowLevel of PWM output to be mapped onto the DAC by the DMA
     */
    uint16_t m_prmLowLevel16;

    /*!
     * \brief The DMA channel used to map m_prmHighLevel16 onto the DAC
     */
    std::shared_ptr<CSamDMAChannel> m_pHLevDMAch;

    /*!
     * \brief The DMA channel used to map m_prmLowLevel16 onto the DAC
     */
    std::shared_ptr<CSamDMAChannel> m_pLLevDMAch;

    /*!
     * \brief The PWM periods counter. Used to stop generation if "repeats"!=0
     */
    CSamTC m_PeriodsCounter;


    /*!
     * \brief An associated clock generator
     */
    static std::shared_ptr<CSamCLK> m_pCLK;

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

    void synced_DAC_set(unsigned int nLevel);


public:
    CDacPWMht(PWM nPWM, const std::shared_ptr<CPin> &pDACsw, CDacPWMht::mode nOpMode=CDacPWMht::mode::DMA);
};

