/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CSamTempSensor
*/

#pragma once

/*!
 * \brief The CSamTempSensor class: implements SAME54 temperature sensor
 */

#include "SamADCcntr.h"
class CSamTempSensor
{
protected:

    /*!
     * \brief the ADC channels of two temperature sensors:
     *
     * \details "The device provides two temperature sensors (TSENSP and TSENSC, respectively)
     * at different locations in the die, controlled by the SUPC - Supply Controller.
     *  The output voltages from the sensors, VTP and VTC, can be sampled by the ADC." manual, page 1618
     */
    CSamADCchan m_VTP;
    CSamADCchan m_VTC;

    /*!
     * \brief a cashed value of measured temperatue in degrees Celsius
     */

    float m_MeasuredTempCD;

    /*!
     *  how the temperature is actually measured: "Using the two conversion results, TP and TC,
     *  and the temperature calibration parameters found in the
     *  NVM Software Calibration Area, the die temperature T can be calculated:
ܶ     *  ( TL*VPH*TC - VPL*TH*TC - TL*VCH*TP + TH*VCL*TP)/
     *  ( VCL*TP - VCH*TP - VPL*TC + VPH*TC )
     *  Here, TL and TH are decimal numbers composed of their respective integer part (TLI, THI) and decimal
     *  parts (TLD and THD) from the NVM Software Calibration Area.
     *
     *  simplify the expression:
     *  T= ( TC(TL*VPH - TH*VPL) + TP(TH*VCL - TL*VCH) )/( TP(VCL-VCH) + TC(VPH-VPL) )
     *  thats why we need the following cashed values:
     *  TLVPH_THVPL, THVCL_TLVCH, VCL_VCH, VPH_VPL
     */

     float m_TLVPH_THVPL, m_THVCL_TLVCH, m_VCL_VCH, m_VPH_VPL;


public:

    /*!
     * \brief CSamTempSensor: class constructor
     *
     * \param pCont - a pointer to one of the SAM's ADC
     *
     */

    CSamTempSensor(std::shared_ptr<CSamADCcntr> &pCont);

    /*!
     * \brief GetTempCD measured temperature in degrees Celsius
     * \return measured temperature.
     *
     * \details returns a cashed value of measured temperature in degrees Celsius.
     * to obtain and refresh the value Update() method should be used
     */

    float GetTempCD(){ return m_MeasuredTempCD; }

    /*!
     * \brief Update: by this method we receive the CPU time to perform internal updation
     *
     * \details how the temperature is actually measured: "Using the two conversion results, TP and TC,
     *  and the temperature calibration parameters found in the
        NVM Software Calibration Area, the die temperature T can be calculated:
ܶ        ( TL*VPH*TC - VPL*TH*TC - TL*VCH*TP + TH*VCL*TP)/
        ( VCL*TP - VCH*TP - VPL*TC + VPH*TC )
        Here, TL and TH are decimal numbers composed of their respective integer part (TLI, THI) and decimal
        parts (TLD and THD) from the NVM Software Calibration Area.
     */
    void Update();
};
