// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/**
 * @file
 * Common stuff of all Serial Peripheral Interface (SPI) devices.
 */

#ifndef PANDA_TIMESWIPE_SPI_HPP
#define PANDA_TIMESWIPE_SPI_HPP

#include "serial.hpp"

/**
 * @brief An SPI device.
 *
 * @details Adds specific SPI bus functionality to a `CSerial` device. In real
 * hardware devices a data (series of bytes) are normally stored in FIFO buffer.
 *
 *
 * @details A specific SPI interface for setting phase and polarity set_phpol() ,
 * baud rate divisor set_baud_div(), setting time profile for transfer operation set_tprofile_divs()
 * \todo seems to be deprecated,too complex...
 *
 * @see CFIFO.
 */
class CSPI : public virtual ISerial {
public:
    /*!
     * \brief Sends a serial message to this class object
     * \param msg  - the message to send (output parameter)
     * \return the operation result: true if successful otherwise - false
     */
    virtual bool send(CFIFO& msg) override = 0;

    /*!
     * \brief Receives a serial message from this class object
     * \param msg - the message to receive (input parameter)
     * \return the operation result: true if successful otherwise - false
     */
    virtual bool receive(CFIFO& msg) override = 0;


    /*!
     * \brief Performs a SPI transfer operation: send output message, receive input message of the same length
     * \param out_msg - the message to send (output parameter)
     * \param in_msg - the message to receive (input parameter)
     * \return the operation result: true if successful otherwise - false
     */
    virtual bool transfer(CFIFO& /*out_msg*/, CFIFO& /*in_msg*/)
    {
      return false; // dbg
    }


    /*!
     * \brief Setups phase & polarity
     * \param bPhase A phase to set: true(1)-shifted, false(0) - not shifted
     * \param bPol A polarity to set: true - bus idle state=HIGH, false - bus idle state=LOW
     */
    virtual void set_phpol(bool bPhase, bool bPol) = 0;

    /*!
     * \brief  Setups baudrate divisor
     * \param div A divisor value: baudrate=clock_speed/div
     */
    virtual void set_baud_div(unsigned char div) = 0;

    /*!
     * \brief Setups the bus timing profile ---minimal time to HOLD CS HIGH---___delay in between transfers___---delay before SCK is continued---
     * \param CSminDel A minimal time to HOLD CS HIGH
     * \param IntertransDel A delay in between transfers
     * \param BeforeClockDel A delay before SCK is continued
     */
    virtual void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel) = 0;

    //! virtual destructor
    virtual ~CSPI() = default;
};

#endif  // PANDA_TIMESWIPE_SPI_HPP
