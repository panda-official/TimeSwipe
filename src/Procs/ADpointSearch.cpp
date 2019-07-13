/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/


#include <math.h>
#include "ADpointSearch.h"

void CADpointSearch::Update()
{
    if(typePTsrcState::searching!=m_State)
        return;

    //int CurPoint=m_pADC->GetRawBinVal();
    int CurPoint=m_pADC->DirectMeasure(); //direct mes instead... without chan switching

    int CurSetPoint=m_pDAC->GetRawBinVal();
    int err=(m_TargPoint-CurPoint);
    bool  bErrSign=std::signbit(err);


    //28.05.2019:
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



   /* m_pDAC->SetRawOutput(  bErrSign ? (CurSetPoint & ~(1L<<m_ProcBits)) : (CurSetPoint | (1L<<(m_ProcBits-1))) );
    if(0==(--m_ProcBits))
    {
         m_State=(std::abs(err) < m_TargErrTolerance) ? typePTsrcState::found : typePTsrcState::error;
    }*/
}
