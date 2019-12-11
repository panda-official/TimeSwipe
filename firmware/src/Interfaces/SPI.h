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
 * \brief   The basic class for SPI devices
 * \details This pure virtual class declares some specific SPI interfaces for setting phase and polarity set_phpol() ,
 * baud rate divisor set_baud_div(), setting time profile for transfer operation set_tprofile_divs()
 * \todo seems to be deprecated,too complex...
 *
 */
class CSPI : public virtual ISerial{
	
public:
    /*!
     * \brief send a serial message to this class object
     * \param msg  a message to send (output parameter)
     * \return operation result: true if successful otherwise - false
     */
    virtual bool send(CFIFO &msg)=0;

    /*!
     * \brief receive a serial message from this class object
     * \param msg a message to receive (input parameter)
     * \return operation result: true if successful otherwise - false
     */
    virtual bool receive(CFIFO &msg)=0;

    /*!
     * \brief send a single character to this class object
     * \param ch a character to send
     * \return operation result: true if successful otherwise - false
     * \details deprecated
     */

    virtual bool send(typeSChar ch)=0;

    /*!
     * \brief receive a single character from this class object
     * \param ch a character to receive
     * \return operation result: true if successful otherwise - false
     * \details deprecated
     */

    virtual bool receive(typeSChar &ch)=0;
	
    /*!
     * \brief setup phase & polarity
     * \param bPhase phase to set: true(1)-shifted, false(0) - not shifted
     * \param bPol polarity to set: true - bus idle state=HIGH, false - bus idle state=LOW
     */
	virtual void set_phpol(bool bPhase, bool bPol)=0;

    /*!
     * \brief  setup baudrate divisor
     * \param div divisor value: baudrate=clock_speed/div
     */
	virtual void set_baud_div(unsigned char div)=0;

    /*!
     * \brief setup the bus timing profile ---minimal time to HOLD CS HIGH---___delay in between transfers___---delay before SCK is continued---
     * \param CSminDel minimal time to HOLD CS HIGH
     * \param IntertransDel delay in between transfers
     * \param BeforeClockDel delay before SCK is continued
     */
	virtual void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel)=0;

    //! virtual destructor
    virtual ~CSPI()=default;
};
