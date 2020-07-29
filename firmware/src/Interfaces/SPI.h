/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

/*!
*   @file
*   @brief A definition file for SPI(Serial Peripheral Interface) CSPI class - basic class for all SPI devices
*
*/

#include "Serial.h"

/*!
 * \brief   A basic class for SPI devices
 * \details This pure virtual class declares some specific SPI interfaces for setting phase and polarity set_phpol() ,
 * baud rate divisor set_baud_div(), setting time profile for transfer operation set_tprofile_divs()
 * \todo seems to be deprecated,too complex...
 *
 */
class CSPI : public virtual ISerial{
	
public:
    /*!
     * \brief Sends a serial message to this class object
     * \param msg  - the message to send (output parameter)
     * \return the operation result: true if successful otherwise - false
     */
    virtual bool send(CFIFO &msg)=0;

    /*!
     * \brief Receives a serial message from this class object
     * \param msg - the message to receive (input parameter)
     * \return the operation result: true if successful otherwise - false
     */
    virtual bool receive(CFIFO &msg)=0;


    /*!
     * \brief Performs a SPI transfer operation: send output message, receive input message of the same length
     * \param out_msg - the message to send (output parameter)
     * \param in_msg - the message to receive (input parameter)
     * \return the operation result: true if successful otherwise - false
     */
    virtual bool transfer(CFIFO &out_msg, CFIFO &in_msg){return false;} //dbg

	
    /*!
     * \brief Setups phase & polarity
     * \param bPhase A phase to set: true(1)-shifted, false(0) - not shifted
     * \param bPol A polarity to set: true - bus idle state=HIGH, false - bus idle state=LOW
     */
	virtual void set_phpol(bool bPhase, bool bPol)=0;

    /*!
     * \brief  Setups baudrate divisor
     * \param div A divisor value: baudrate=clock_speed/div
     */
	virtual void set_baud_div(unsigned char div)=0;

    /*!
     * \brief Setups the bus timing profile ---minimal time to HOLD CS HIGH---___delay in between transfers___---delay before SCK is continued---
     * \param CSminDel A minimal time to HOLD CS HIGH
     * \param IntertransDel A delay in between transfers
     * \param BeforeClockDel A delay before SCK is continued
     */
	virtual void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel)=0;

    //! virtual destructor
    virtual ~CSPI()=default;
};
