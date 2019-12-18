/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include <math.h>
#include "ADpointSearch.h"

int CADpointSearch::m_TargErrTolerance;

void CADpointSearch::Update()
{
    if(typePTsrcState::searching!=m_State)
        return;

    int CurPoint=m_pADC->DirectMeasure();
    int CurSetPoint=m_pDAC->GetRawBinVal();
    int err=(m_TargPoint-CurPoint);
    bool  bErrSign=std::signbit(err);

    //set test bit:
    if(m_ProcBits>0)
    {
        CurSetPoint|=1L<<(m_ProcBits-1);
    }

    //remove bit:
    if(bErrSign)
    {
        CurSetPoint&=~(1L<<m_ProcBits);
    }

    m_pDAC->SetRawOutput(CurSetPoint);
    if(0==m_ProcBits--)
    {
        m_State=(std::abs(err) < m_TargErrTolerance) ? typePTsrcState::found : typePTsrcState::error;
    }
}
