#pragma once

#include <memory>
#include <vector>
#include "ADC.h"
#include "DAC.h"



#include "nodeLED.h"
#include "ADpointSearch.h"
class CCalMan
{
protected:
    std::vector<CADpointSearch>             m_ChanCal;
    std::vector< std::shared_ptr<CLED> >    m_pLED;
    std::vector<typePTsrcState>             m_State;

    //quatation:
    unsigned long m_LastTimeUpd;

public:
    void Add(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CDac> &pDAC, const std::shared_ptr<CLED> &pLED)
    {
        m_ChanCal.emplace_back(pADC, pDAC);
        m_pLED.emplace_back(pLED);
        m_State.emplace_back(typePTsrcState::idle);
    }
    void Start();
    void StopReset();
    void Update();
};
