/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#include "shiftreg.hpp"


void CShiftReg::SetShiftReg(typeRegister &RegValue, std::size_t BitsInUse)
{
    for(std::size_t i=0; i<BitsInUse; i++)
    {
        m_pDataPin->write(RegValue[i]);
        m_pClockPin->write(true);
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"); //200nS
        m_pClockPin->write(false);
        asm("nop; nop; nop; nop; nop; nop; nop; nop;"
            "nop; nop; nop; nop; nop; nop; nop; nop;"); //200nS
    }

    //strobe:
    m_pStrobePin->write(true);
    asm("nop; nop; nop; nop; nop; nop; nop; nop;"
        "nop; nop; nop; nop; nop; nop; nop; nop;"); //200nS
    m_pStrobePin->write(false);
}

std::shared_ptr<CShiftRegPin> CShiftReg::FactoryPin(std::size_t nBit)
{
    if(m_OccupiedBitsMask[nBit])
        return nullptr;

    m_OccupiedBitsMask[nBit]=true;
    return std::shared_ptr<CShiftRegPin>(new CShiftRegPin(shared_from_this(), nBit));
}
