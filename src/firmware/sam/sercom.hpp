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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SAM_SERCOM_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SAM_SERCOM_HPP

/*!
 * \brief An enumeration of possible SAME54 Sercom devices
 */
enum class typeSamSercoms : int {Sercom0=0, Sercom1, Sercom2, Sercom3, Sercom4, Sercom5, Sercom6, Sercom7};

/*!
 * \brief An enumeration of Sercom IRQ channels
 */
enum class typeSamSercomIRQs : int {IRQ0=0, IRQ1, IRQ2, IRQ3};

#include "../../serial.hpp"
#include "SamCLK.h"

extern "C"{
void SERCOM0_0_Handler(void);
void SERCOM0_1_Handler(void);
void SERCOM0_2_Handler(void);
void SERCOM0_3_Handler(void);
void SERCOM1_0_Handler(void);
void SERCOM1_1_Handler(void);
void SERCOM1_2_Handler(void);
void SERCOM1_3_Handler(void);
void SERCOM2_0_Handler(void);
void SERCOM2_1_Handler(void);
void SERCOM2_2_Handler(void);
void SERCOM2_3_Handler(void);
void SERCOM3_0_Handler(void);
void SERCOM3_1_Handler(void);
void SERCOM3_2_Handler(void);
void SERCOM3_3_Handler(void);
void SERCOM4_0_Handler(void);
void SERCOM4_1_Handler(void);
void SERCOM4_2_Handler(void);
void SERCOM4_3_Handler(void);
void SERCOM5_0_Handler(void);
void SERCOM5_1_Handler(void);
void SERCOM5_2_Handler(void);
void SERCOM5_3_Handler(void);
void SERCOM6_0_Handler(void);
void SERCOM6_1_Handler(void);
void SERCOM6_2_Handler(void);
void SERCOM6_3_Handler(void);
void SERCOM7_0_Handler(void);
void SERCOM7_1_Handler(void);
void SERCOM7_2_Handler(void);
void SERCOM7_3_Handler(void);
}

/*!
 * \brief An implementation of SAME54's basic Serial Communication Interface.
 * \details Depending on settings it can be turned into USART, SPI, I2C-master or I2C-slave.
 * The provides basic functionality of SERCOM mainly dealing with interrupt processing, enabling and connecting corresponding
 * CSamCLK (Generic Clock controller)
 */
class CSamSercom : virtual public CSerial
{
protected:

    /*!
     * \brief The SERCOM ID
     */
    typeSamSercoms m_nSercom;

protected:

    /*!
     * \brief The class constructor
     * \param nSercom The SERCOM ID
     * \details The constructor does the following:
     * connects SERCOM object to the corresponding slot allowing to handle corresponding Cortex M/SAME54
     */
    CSamSercom(typeSamSercoms nSercom);

    /*!
     * \brief Virtual destructor
     */
    virtual ~CSamSercom();

    /*!
     * \brief The handler of the 1st IRQ line
     */
    virtual void OnIRQ0(){}
    /*!
     * \brief The handler of the 2nd IRQ line
     */
    virtual void OnIRQ1(){}
    /*!
     * \brief The handler of the 3rd IRQ line
     */
    virtual void OnIRQ2(){}
    /*!
     * \brief The handler of the 4th IRQ line
     */
    virtual void OnIRQ3(){}

    /*!
     * \brief Enable corresponding IRQ line
     * \param nLine An IRQ line ID
     * \param how true=enable, false=disable
     */
    void EnableIRQ(typeSamSercomIRQs nLine, bool how);

    /*!
     * \brief Enables internal communication bus with SERCOM
     * \param nSercom A SERCOM ID
     * \param how treu=enable, false=disable
     */
    static void EnableSercomBus(typeSamSercoms nSercom, bool how);

    /*!
     * \brief Connects a clock generator to SERCOM device
     * \param nSercom A SERCOM ID
     * \param nCLK A clock generator ID
     */
    static void ConnectGCLK(typeSamSercoms nSercom, typeSamCLK nCLK);

friend void SERCOM0_0_Handler(void);
friend void SERCOM0_1_Handler(void);
friend void SERCOM0_2_Handler(void);
friend void SERCOM0_3_Handler(void);

friend void SERCOM1_0_Handler(void);
friend void SERCOM1_1_Handler(void);
friend void SERCOM1_2_Handler(void);
friend void SERCOM1_3_Handler(void);

friend void SERCOM2_0_Handler(void);
friend void SERCOM2_1_Handler(void);
friend void SERCOM2_2_Handler(void);
friend void SERCOM2_3_Handler(void);

friend void SERCOM3_0_Handler(void);
friend void SERCOM3_1_Handler(void);
friend void SERCOM3_2_Handler(void);
friend void SERCOM3_3_Handler(void);

friend void SERCOM4_0_Handler(void);
friend void SERCOM4_1_Handler(void);
friend void SERCOM4_2_Handler(void);
friend void SERCOM4_3_Handler(void);

friend void SERCOM5_0_Handler(void);
friend void SERCOM5_1_Handler(void);
friend void SERCOM5_2_Handler(void);
friend void SERCOM5_3_Handler(void);

friend void SERCOM6_0_Handler(void);
friend void SERCOM6_1_Handler(void);
friend void SERCOM6_2_Handler(void);
friend void SERCOM6_3_Handler(void);

friend void SERCOM7_0_Handler(void);
friend void SERCOM7_1_Handler(void);
friend void SERCOM7_2_Handler(void);
friend void SERCOM7_3_Handler(void);
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SAM_SERCOM_HPP
