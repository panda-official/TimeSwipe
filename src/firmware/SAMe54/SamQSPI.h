/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CSamQSPI
*/

#pragma once
#include "SPI.h"
#include "Pin.h"

/*!
 * \brief An implementation of SAM E54 QSPI bus
 * \details "The QSPI can be used in “SPI mode” to interface serial peripherals, such as ADCs, DACs, LCD
 *   controllers and sensors, or in “Serial Memory Mode” to interface serial Flash memories." manual, page 1087
 */
class CSamQSPI : public CSPI
{
public:
    /*!
     * \brief The class constructor
     * \details The constructor does the following:
     * setups corresponding PINs and its multiplexing
     */
    CSamQSPI(bool bAutoCS=false);
	
    /*!
     * \brief Sends a serial message to this class object
     * \param msg  - the message to send (output parameter)
     * \return the operation result: true if successful otherwise - false
     */
	virtual bool send(CFIFO &msg);

    /*!
     * \brief Receives a serial message from this class object
     * \param msg - the message to receive (input parameter)
     * \return the operation result: true if successful otherwise - false
     */
	virtual bool receive(CFIFO &msg);
	
    /*!
     * \brief Setups phase & polarity
     * \param bPhase A phase to set: true(1)-shifted, false(0) - not shifted
     * \param bPol A polarity to set: true - bus idle state=HIGH, false - bus idle state=LOW
     */
	void set_phpol(bool bPhase, bool bPol);

    /*!
     * \brief  Setups baudrate divisor
     * \param div A divisor value: baudrate=clock_speed/div
     */
	void set_baud_div(unsigned char div);

    /*!
     * \brief Setups the bus timing profile ---minimal time to HOLD CS HIGH---___delay in between transfers___---delay before SCK is continued---
     * \param CSminDel A minimal time to HOLD CS HIGH
     * \param IntertransDel A delay in between transfers
     * \param BeforeClockDel A delay before SCK is continued
     */
	void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel);

    //! The virtual destructor of the class
    virtual ~CSamQSPI(){}
};
