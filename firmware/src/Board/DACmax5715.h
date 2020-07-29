/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

/*!
*   \file
*   \brief A definition file for MAX5715 DAC's channel
*   CDac5715sa
*
*/

/*!
 * \brief The typeDac5715chan enum enumeration of presented DAC channels
 */

enum class typeDac5715chan : int
{
    DACA=0, //!<channel A, #0
    DACB,   //!<channel B, #1
    DACC,   //!<channel C, #2
    DACD    //!<channel D, #3
};

#include "SPI.h"
#include "DAC.h"
#include "Pin.h"

/*!
 * \brief The CDac5715sa class implements MAX5715 DAC's channel functionality
 *
 * \details the class is derived from a CDac base class. It requires a separate object per each MAX5715 channel
 * To control the MAX5715 channel output please use DAC functions
 * void SetVal(float val);
 * void SetRawOutput(int val);
 * They invoke DriverSetVal which is overriden in this class and directly set the MAX5715 output via SPI bus
 *
 * Current version: only DAC channel output functionality is presented,
 * configuration functions are not implemented.
 *
 */

class CDac5715sa : public CDac //dac chan 5715 stand-alone version
{
protected:

    /*!
     * \brief m_pBus a pointer to SPI bus.
     */
	CSPI *m_pBus;

    std::shared_ptr<IPin> m_pCS;

    /*!
     * \brief m_chan what channel of MAX5715 is controlled (A or B or C or D)
     */
	typeDac5715chan m_chan;

    /*!
     * \brief DriverSetVal the main driver function to set DAC channel output value.
     * The function should be overridden in each CDac derived class to implement functionality of current DAC chip (currently MAX5715)
     * \param val channel output value in user defined units (Volts, Amperes, e.t.c) for models that can handle
     *  the format(some PCI boards for example, currently not used)
     * \param out_bin output value in raw binary format - used directly by MAX5715
     */
	virtual void DriverSetVal(float val, int out_bin);
	
public:
    /*!
     * \brief CDac5715sa class constructor
     * \param pBus      -a pointer to a SPI bus
     * \param nChan     -a DAC channel to control
     * \param RangeMin  -a channel range minimum value in user defined units (Volts, Amperes, e.t.c)
     * \param RangeMax  -a channel range maximum value in user defined units (Volts, Amperes, e.t.c)
     */
    CDac5715sa(CSPI *pBus, std::shared_ptr<IPin> pCS, typeDac5715chan nChan, float RangeMin, float RangeMax);
     //   virtual ~CDac5715sa(){} //just to keep polymorphic behaviour, should be never called
};
