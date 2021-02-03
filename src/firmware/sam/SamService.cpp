/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include <stdio.h>
#include "SamService.h"

std::string CSamService::m_SerialString;

std::array<uint32_t, 4> CSamService::GetSerial()
{
    return {
        *((uint32_t   *) 0x008061FCUL),
        *((uint32_t   *) 0x00806010UL),
        *((uint32_t   *) 0x00806014UL),
        *((uint32_t   *) 0x00806018UL)
    };
}

std::string CSamService::GetSerialString()
{
    //generate string on demand:
    if(m_SerialString.empty())
    {
        char tbuf[128];
        std::array<uint32_t, 4> serial=GetSerial();
        sprintf(tbuf, "%X-%X-%X-%X", serial[0], serial[1], serial[2], serial[3]);
        m_SerialString=tbuf;
    }
    return m_SerialString;
}
