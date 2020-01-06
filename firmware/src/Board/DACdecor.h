/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

/*!
*   \file
*   \brief A definition file for Pseudo-Decorator class for DAC
*   CDACdecor
*
*/

/*!
*  @brief Pseudo-Decorator class for a DAC: changing DAC behavior depending on CADmux's DACmode
*
*  @details A pseudo-decorator class (just a simple wrapper class without a super-class) that contains
*           pointers to the corresponding DACs and CADmux class and decorates 4 of DAC's methodes:
*           SetVal, GetRealVal, SetRawOutput, GetRawBinVal which are required for a command processor
*
*  @todo    Create a real decorator with an abstract interface(? isn't too much overhead for this?)
*
*/

#include <memory>
#include "ADmux.h"

class  CDACdecor //: public CDac
{
protected:
    std::shared_ptr<CDac> m_pExtDAC;
    std::shared_ptr<CDac> m_pSamDAC;
    std::shared_ptr<CADmux> m_pADmux;

    bool m_bInverted;   //!inverted behaviour (required for "cold" outputs)

    /*!
    *  @brief  Helper: returns a pointer to a decorated DAC depending on typeDACmode (DACsw)
    *
    */
    std::shared_ptr<CDac> &GetCurDacPtr()
    {
        bool bChoise=(typeDACmode::ExtDACs==static_cast<typeDACmode>(m_pADmux->getDACsw()));
        if(m_bInverted)
             bChoise=!bChoise;

        if(bChoise)
            return m_pExtDAC; //default
        else
            return m_pSamDAC;
    }

public:
    /*!
    *  @brief   A class constructor
    *
    *  @param   pExtDAC - pointer to an external DAC (currently DACmax5715 chip, 4 DAC channels)
    *  @param   pSamDAC - pointer to an internal SAME54 DAC (2 channels)
    *  @param   pADmux  - pointer to the board Analog/Digital multiplexor control class
    *
    */
    CDACdecor(std::shared_ptr<CDac> pExtDAC, std::shared_ptr<CDac> pSamDAC, std::shared_ptr<CADmux> pADmux, bool bInverted=false)
    {
        m_pExtDAC=pExtDAC;
        m_pSamDAC=pSamDAC;
        m_pADmux=pADmux;
        m_bInverted=bInverted;
    }

    float GetRealVal(){ return GetCurDacPtr()->GetRealVal(); }
    int GetRawBinVal(){ return GetCurDacPtr()->GetRawBinVal(); }
    void SetVal(float val){ GetCurDacPtr()->SetVal(val); }
    void SetRawOutput(int val){ GetCurDacPtr()->SetRawOutput(val); }

};
