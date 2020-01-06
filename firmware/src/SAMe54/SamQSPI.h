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
    CSamQSPI();
	
	virtual bool send(CFIFO &msg);
	virtual bool receive(CFIFO &msg);
    virtual bool send(typeSChar ch);
    virtual bool receive(typeSChar &ch);
	
	void set_phpol(bool bPhase, bool bPol);
	void set_baud_div(unsigned char div);
	void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel);

    //! The virtual destructor of the class
    virtual ~CSamQSPI(){}
};
