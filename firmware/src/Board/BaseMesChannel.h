/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include <memory>
#include "DAC.h"
#include "Pin.h"
#include "DataVis.h"

class nodeControl;
class CMesChannel
{
friend class nodeControl;
public:

    enum mes_mode{

        Voltage=0,
        Current
    };

    //interface:
    inline int GetADCmesRawVal(){

        return m_pADC->GetRawBinVal();
    }
    virtual void IEPEon(bool bHow){

        m_bIEPEon=bHow;
    }
    inline bool IsIEPEon(){

        return  m_bIEPEon;
    }
    virtual void SetMesMode(mes_mode nMode){

        m_MesMode=nMode;
    }
    virtual void SetAmpGain(float GainValue){

        m_ActualAmpGain=GainValue;
    }
    inline float GetActualAmpGain()
    {
        return m_ActualAmpGain;
    }
    inline unsigned int CmGetMesMode(){

        return static_cast<int>(m_MesMode);
    }
    inline void CmSetMesMode(unsigned int nMode){

        if(nMode>mes_mode::Current)
            nMode=mes_mode::Current;

        SetMesMode( static_cast<mes_mode>(nMode) );
    }

protected:
    nodeControl *m_pCont=nullptr;

   // bool m_bEnabled=false;
    bool m_bIEPEon=false;
    mes_mode m_MesMode=mes_mode::Voltage;
    float m_ActualAmpGain=1.0f;

    std::shared_ptr<CAdc> m_pADC;
    std::shared_ptr<CDac> m_pDAC;
    CDataVis m_VisChan;

    void Update(){

        m_VisChan.Update( m_pADC->GetRawBinVal() );
    }

public:
    CMesChannel(const std::shared_ptr<CAdc> &pADC,  const std::shared_ptr<CDac> &pDAC,  CView::vischan nCh) : m_VisChan(nCh){

        m_pADC=pADC;
        m_pDAC=pDAC;
    }
};

typedef CMesChannel CIEPEchannel;


