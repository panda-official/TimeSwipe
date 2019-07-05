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
    int m_TargErrTolerance;

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
        m_TargErrTolerance=3;
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
