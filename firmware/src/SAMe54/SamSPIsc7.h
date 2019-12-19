/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CSamSPIsc7
*/

#pragma once

#include "SamSPI.h"

/*!
 * \brief A hardware-dependent realization of SPI intercommunication bus for SERCOM7 pinouts
 */
class CSamSPIsc7 : public CSamSPI
{
public:
    /*!
      * \brief The class constructor
      * \param bMaster A Master mode
      * The constructor does the following:
      * 1) calls CSamSPI constructor
      * 2) setups corresponding PINs and its multiplexing
      * 3) performs final tuning and enables SERCOM7 in SPI mode
      */
    CSamSPIsc7(bool bMaster=false);
    virtual void chip_select(bool how);
};


