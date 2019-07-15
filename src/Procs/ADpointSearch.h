/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

#include <memory>
#include <vector>
#include "ADC.h"
#include "DAC.h"

enum class typePTsrcState{idle, searching, found, error};

class CADpointSearch
{
protected: //data

    typePTsrcState m_State;
    int            m_ProcBits;

    int m_TargPoint;
    static int m_TargErrTolerance; //15.07.2019 make the value static (one for all?)

    //control:
    std::shared_ptr<CAdc> m_pADC;
    std::shared_ptr<CDac> m_pDAC;

public:
    //m_TargErrTolerance setter/getter 15.07.2019
    static int  GetTargErrTol() {return m_TargErrTolerance; }
    static void SetTargErrTol(int val){ if(val<1) val=1; m_TargErrTolerance=val;}

    typePTsrcState state(){return m_State;}
    CADpointSearch(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CDac> &pDAC)
    {
        m_State=typePTsrcState::idle;

        m_pADC=pADC;
        m_pDAC=pDAC;
        m_TargErrTolerance=25;  //15.07.2019 25 bits
    }
    typePTsrcState Search(int val)
    {
        m_TargPoint=val;
        m_State=typePTsrcState::searching;
        m_ProcBits=12;
        m_pDAC->SetRawBinVal(0); //set initial value

        return m_State;
    }
    void StopReset()
    {
        m_State=typePTsrcState::idle;
    }
    void Update();
};
