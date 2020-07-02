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
    std::shared_ptr<CPin> m_pEnableSwitch;
    CPGA280 m_PGA;

    virtual void Enable(bool bHow){

        m_bEnabled=bHow;
        m_pEnableSwitch->Set(bHow);
    }
    virtual void SetMesMode(mes_mode nMode){

        m_MesMode=nMode;

    }
    virtual void SetAmpGain(float GainValue){

        m_ActualAmpGain=GainValue;
    }

};
