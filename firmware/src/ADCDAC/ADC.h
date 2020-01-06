/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   @file
*   @brief A definition file for ADC (Analog-to-Digital-Converter) channel abstract class
*   CAdc
*
*/

#pragma once
#include "ADchan.h"

/*!
 * \brief An ADC channel class: uses only ADC functionality from CADchan
 */
class CAdc : public CADchan{

public:
    /*!
     * \brief Force direct measurement for this channel on ADC device without queuing
     * \return Immediately measured analog value in raw-binary format
     */
    virtual int DirectMeasure(){return CADchan::GetRawBinVal(); }
    //virtual ~CAdc(){}  //just to keep polymorphic behaviour, should be never called

};
