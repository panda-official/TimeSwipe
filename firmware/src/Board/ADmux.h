/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

/*!
*   \file
*   \brief A definition file for board's digital multiplexer
*   CADmux,
*   amplifier gains enum typeADgain and Analog Outputs mapping mode enum typeDACmode
*
*
*/


/*!
 * \brief A possible board's amplifier gain values
 */
enum class typeADgain: int
{
    gainX1=1,   //!<Bypass, no amplification
    gainX2,     //!<Gain X2
    gainX4,     //!<Gain X4
    gainX8      //!<Gain X8
};

/*!
 * \brief A possible DAC mode values
 */
enum class typeDACmode : int
{
    ExtDACs=0,      //!<map amplifier outputs onto Analog Outputs 1-4
    SamAndExtDACs   //!<map amplifier outputs onto Analog Outouts 1-2, map SAME54 DAC outputs onto Analog Outputs 3-4
};

#include "DAC.h"

/*!
 * \brief The board's digital multiplexer
 * \details implements hardware-dependent realization of setting gain, bridge voltage, enabling ADC measurements and so on
 *
 */
class CADmux
{
protected:

        /*!
         * \brief Set gain of board's amplifier
         */
        typeADgain m_CurGain;

        /*!
         * \brief Set DAC mapping mode
         */
        typeDACmode m_CurDACmode;

        /*!
         * \brief Bridge voltage: activated or not
         */
        bool       m_UBRVoltage;

        /*!
         * \brief ADC measurements: enabled or not
         */
        bool       m_bADmesEnabled;

        /*!
         * \brief The fan status: rotating or not
         */
        bool       m_bFanIsStarted;

public:

     /*!
     * \brief The class constructor: the implementation contains the PINs function setup
     */
	CADmux();
	
    /*!
     * \brief Enable or disable ADC measurements on the board
     * \param how true=enable, false=disable
     */
	void EnableADmes(bool how);

    /*!
     * \brief Read back the current enabled state of ADC measurements on the board
     * \return true=enabled, false=disabled
     */
    bool IsADmesEnabled(){return m_bADmesEnabled; }
	
    /*!
     * \brief Set the amplifier gain value
     * \param gain gain to set
     */
	void SetGain(typeADgain gain);

    /*!
     * \brief Read back the amplifier gain value
     * \return The amplifier gain value
     */
    typeADgain GetGain(){ return m_CurGain; }

    /*!
     * \brief Setup the DACs mapping mode
     * \param mode mode to set
     */
    void SetDACmode(typeDACmode mode);

    /*!
     * \brief Turn on/off the bridge voltage
     * \param how true=turn on, false=turn off
     */
    void SetUBRvoltage(bool how);

    /*!
     * \brief Read back set bridge voltage
     * \return
     */
    bool GetUBRVoltage(){ return m_UBRVoltage; }

    /*!
     * \brief Turn on/off the fan
     * \param how true=on, false=off
     * \deprecated
     */
    void StartFan(bool how);

    /*!
     * \brief Read back set fan state
     * \return true=on, false=off
     * \deprecated
     */
    bool IsFanStarted(){ return m_bFanIsStarted; }

    /*!
     * \brief Setup the DACs mapping mode from integer type
     * \param mode mode to set: 0=default mapping(ExtDACs), 1=SamAndExtDACs
     * \details this converter is required to form a communication "access point" of <int> type
     */
    void setDACsw(int mode){ SetDACmode(mode ? typeDACmode::SamAndExtDACs : typeDACmode::ExtDACs); }

    /*!
     * \brief Read back the DACs mapping mode as an integer value
     * \return 0=default mapping(ExtDACs), 1=SamAndExtDACs
     * \details this converter is required to form a communication "access point" of <int> type
     */
    int getDACsw(){ return static_cast<int>(m_CurDACmode); }
};

