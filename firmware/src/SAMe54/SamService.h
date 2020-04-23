/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CSamService
*/

#pragma once

#include <array>
#include <stdint.h>
#include <string>

/*!
 * \brief An implementation of some SAM's service functions
 * \details Various service functions of the SAM like fetching its UUID and so on.
 */

class CSamService
{
protected:
    static std::string m_SerialString;

public:

    /*!
     * \brief Fetches the UUID of SAM CPU.
     * \return An array containing UUID in a form of four 32-bit words
     */
    static std::array<uint32_t, 4> GetSerial();

    /*!
     * \brief Fetches the UUID of SAM CPU as the string
     * \return A string containing UUID in a form of "XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX"
     */
    static std::string GetSerialString();
};

