/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
    int            m_MesCnt;

    float m_TargPoint;

    //targ conditions:
    float m_TargErrTolerance;
    float m_MinAmpVal;

    float m_LastPoint;
    float m_LastSetPoint;
    float m_LastAmp;
    bool  m_bLastErrSign;

    float m_k;
    float m_def_k;
    float m_k_limit;
    float m_Q;

    //control:
    std::shared_ptr<CAdc> m_pADC;
    std::shared_ptr<CDac> m_pDAC;

public:
    typePTsrcState state(){return m_State;}
    CADpointSearch(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CDac> &pDAC)
    {
        m_State=typePTsrcState::idle;

        m_pADC=pADC;
        m_pDAC=pDAC;

        float AdcRangeMin, AdcRangeMax;
        float DacRangeMin, DacRangeMax;

        pADC->GetRange(AdcRangeMin, AdcRangeMax);
        pDAC->GetRange(DacRangeMin, DacRangeMax);

        m_k=(DacRangeMax-DacRangeMin)/(AdcRangeMax-AdcRangeMin); //!!!
        m_def_k=m_k;
        m_k_limit=4.0f*m_k; //???

        m_TargErrTolerance=0.005f;    //???V
        m_MinAmpVal=0.01f;         //???V
    }
    typePTsrcState Search(float val)
    {
        m_TargPoint=val;
        m_State=typePTsrcState::searching;
        m_k=m_def_k;
        m_Q=1.0f;
        m_MesCnt=0;

        //02.05.2019: set intial value: "sensor detection"
        m_pDAC->SetVal(val-3.0); //+++db only!!!

        return m_State;
    }
    void StopReset()
    {
        m_State=typePTsrcState::idle;
    }

    void Update();
};

#include "nodeLED.h"
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
