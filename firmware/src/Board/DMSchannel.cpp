/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "DMSchannel.h"

void CDMSchannel::SetAmpGain(float GainValue)
{
#define OGAIN_FACTOR 1.375f
    static constexpr float GainTab[]={

        (1.0f/8.0f),
        (1.0f/8.0f)*OGAIN_FACTOR,
        (1.0f/4.0f),
        (1.0f/4.0f)*OGAIN_FACTOR,
        (1.0f/2.0f),
        (1.0f/2.0f)*OGAIN_FACTOR,
        1.0f,
        OGAIN_FACTOR,
        2.0f,
        2.0f*OGAIN_FACTOR,
        4.0f,
        4.0f*OGAIN_FACTOR,
        8.0f,
        8.0f*OGAIN_FACTOR,
        16.0f,
        16.0f*OGAIN_FACTOR,
        32.0f,
        32.0f*OGAIN_FACTOR,
        64.0f,
        64.0f*OGAIN_FACTOR,
        128.0f,
        128.0f*OGAIN_FACTOR
    };

    constexpr size_t tsize=sizeof(GainTab)/sizeof(float);
    size_t el;
    for(el=0; el<tsize; el++)
    {
        if(GainValue<GainTab[el])
            break;
    }
    if(el>0)
        el--;


    if(m_pPGA->SetGains( static_cast<CPGA280::igain>(el/2), static_cast<CPGA280::ogain>(el%2) ))
        m_ActualAmpGain=GainTab[el];
}
