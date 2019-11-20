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



#include "nodeLED.h"
#include "ADpointSearch.h"
#include "json_evsys.h" //15.07.2019 adding events

class CCalMan : public CJSONEvCP
{
protected:
    std::vector<CADpointSearch>             m_ChanCal;
    std::vector< std::shared_ptr<CLED> >    m_pLED;
    std::vector<typePTsrcState>             m_State;

    //quatation:
    unsigned long m_LastTimeUpd;
    unsigned long m_UpdSpan=100;    //variable span 15.07.2019

    enum    FSM{
        halted,

        running,
        delay
    };
    FSM m_PState=FSM::halted;

public:
    void Add(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CDac> &pDAC, const std::shared_ptr<CLED> &pLED)
    {
        m_ChanCal.emplace_back(pADC, pDAC);
        m_pLED.emplace_back(pLED);
        m_State.emplace_back(typePTsrcState::idle);
    }
    bool IsStarted(){ return (FSM::halted!=m_PState); }
    void Start();
    void StopReset();
    void Update();
};
