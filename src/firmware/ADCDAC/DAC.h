/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   @file
*   @brief A definition file for DAC (Digital-to-Analog-Converter) channel abstract class
*   CDac
*
*/


#pragma once

#include "ADchan.h"

/*!
 * \brief A DAC channel class: uses only DAC functionality from CADchan
 */
class CDac : public CADchan{
	
protected:
    /*!
     * \brief Sets the output value to the real DAC device
     * \param val A value to set in a real-unit format for devices that can accept it(some PCI boards for example)
     * \param out_bin A value to set in a raw-binary format - most common format for DAC devices
     * \details The function is used to transfer a control value from the abstract DAC channel to the real DAC device
     *  and must be re-implemented in the real device control class
     */
    virtual void DriverSetVal(float val, int out_bin)=0;

public:
    /*!
     * \brief Set control value in a real unit format for this channel
     * \param val A value in a real unit format
     */
	void SetVal(float val);

    /*!
     * \brief Set control value in a raw binary format for this channel
     * \param val A value in a raw binary format
     */
    void SetRawOutput(int val);
    //virtual ~CDac(){}  //just to keep polymorphic behaviour, should be never called
};
