/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "BaseMesChannel.h"
#include "ShiftReg.h"
#include "PGA280.h"

class CDMSchannel : public CMesChannel
{
protected:
    std::shared_ptr<CPin> m_pIEPEswitch;
    std::shared_ptr<CPGA280> m_pPGA;

    virtual void IEPEon(bool bHow){

        m_bIEPEon=bHow;
        m_pIEPEswitch->Set(bHow);
    }
    virtual void SetMesMode(mes_mode nMode){

        m_MesMode=nMode;
        m_pPGA->SetMode( static_cast<CPGA280::mode>(nMode) );

    }
    virtual void SetAmpGain(float GainValue); /*{

        m_ActualAmpGain=GainValue;
        m_pPGA->SetIGain( static_cast<CPGA280::igain>(GainValue) ); //dbg only
    }*/

public:
    CDMSchannel(const std::shared_ptr<CAdc> &pADC,  const std::shared_ptr<CDac> &pDAC,  CView::vischan nCh,
                const std::shared_ptr<CPin> &pIEPEswitch, const std::shared_ptr<CPGA280> &pPGA) :
        CMesChannel(pADC,  pDAC, nCh)
    {
        m_pIEPEswitch=pIEPEswitch;
        m_pPGA=pPGA;
    }

};
