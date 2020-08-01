/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for CMesChannel class aka CIEPEchannel
*   CDac5715sa
*
*/

/*!
*   \file
*   \brief A definition file for CDMSchannel
*/


#pragma once

#include "BaseMesChannel.h"
#include "ShiftReg.h"
#include "PGA280.h"


/*!
 * \brief The implementation of the DMS measurement channel
 */
class CDMSchannel : public CMesChannel
{
protected:

    /*!
     * \brief The pointer to the IEPE switch pin
     */
    std::shared_ptr<CPin> m_pIEPEswitch;

    /*!
     * \brief The pointer to the PGA280 amplifier control instance
     */
    std::shared_ptr<CPGA280> m_pPGA;

    /*!
     * \brief Turns IEPEmode on/off
     * \param bHow - true=IEPE mode ON, false=IEPE mode off
     */
    virtual void IEPEon(bool bHow){

        m_bIEPEon=bHow;
        m_pIEPEswitch->Set(bHow);
    }

    /*!
     * \brief Sets the measurement mode (Voltage or Current)
     * \param nMode - the measuremnt mode to be set
     */
    virtual void SetMesMode(mes_mode nMode){

        m_MesMode=nMode;
        m_pPGA->SetMode( static_cast<CPGA280::mode>(nMode) );

    }

    /*!
     * \brief Sets the channel amplification gain
     * \param GainValue - the Gain value to be set
     */
    virtual void SetAmpGain(float GainValue);

public:

    /*!
     * \brief The class constructor
     * \param pADC - the pointer to the channel's ADC
     * \param pDAC - the pointer to channel's offset control DAC
     * \param nCh  - the visualization index of the channel
     * \param pIEPEswitch - the pointer to the IEPE switch pin
     * \param pPGA - the pointer to the PGA280 amplifier control instance
     */
    CDMSchannel(const std::shared_ptr<CAdc> &pADC,  const std::shared_ptr<CDac> &pDAC,  CView::vischan nCh,
                const std::shared_ptr<CPin> &pIEPEswitch, const std::shared_ptr<CPGA280> &pPGA) :
        CMesChannel(pADC,  pDAC, nCh)
    {
        m_pIEPEswitch=pIEPEswitch;
        m_pPGA=pPGA;
    }
};

