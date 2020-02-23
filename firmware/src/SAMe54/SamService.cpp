/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "SamService.h"

std::array<uint32_t, 4> CSamService::GetSerial()
{
    return {
        *((uint32_t   *) 0x008061FCUL),
        *((uint32_t   *) 0x00806010UL),
        *((uint32_t   *) 0x00806014UL),
        *((uint32_t   *) 0x00806018UL)
    };
}
