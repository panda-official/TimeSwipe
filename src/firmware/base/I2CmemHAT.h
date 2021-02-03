/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#pragma once

/*!
*   \file
*   \brief A definition file for realization of CAT2430 EEPROM chip emulation for HAT's outputs
*   CSamI2CmemHAT
*
*/

#include "sam/SamI2Cmem.h"

/*!
 * \brief A hardware-dependent realization of CAT2430 EEPROM chip emulation for HAT's outputs
 */
class CSamI2CmemHAT : public CSamI2Cmem
{
public:
    /*!
     * \brief The class constructor. The implementation contains the PINs function setup
     */
    CSamI2CmemHAT();
};
