/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "SPIcomm.h"
#include "sam.h"

Sercom *glob_GetSercomPtr(typeSamSercoms nSercom);
#define SELECT_SAMSPI(nSercom) &(glob_GetSercomPtr(nSercom)->SPI)

void CSPIcomm::IRQhandler()
{
    SercomSpi *pSPI=SELECT_SAMSPI(m_nSercom);
    if(pSPI->INTFLAG.bit.RXC)
    {
        typeSChar ch=pSPI->DATA.bit.DATA;
        m_ComCntr.proc(ch, m_recFIFO);
        return;
    }
    if(pSPI->INTFLAG.bit.SSL) //start of frame
    {
     //  m_bCSactive=true;
       m_recFIFO.reset();
       m_ComCntr.start(CSyncSerComFSM::FSM::recLengthMSB);
       pSPI->INTFLAG.bit.SSL=1;
       return;
    }
    if(pSPI->INTFLAG.bit.ERROR)
    {
        pSPI->INTFLAG.bit.ERROR=1;
    }
    if(pSPI->INTFLAG.bit.TXC)
    {
        pSPI->INTFLAG.bit.TXC=1;
    }
}

void CSPIcomm::OnIRQ0()
{
    IRQhandler();
}
void CSPIcomm::OnIRQ1()
{
    IRQhandler();
}
void CSPIcomm::OnIRQ2()
{
    IRQhandler();
}
void CSPIcomm::OnIRQ3()
{
    IRQhandler();
}

bool CSPIcomm::send(CFIFO &msg)
{
    //blocking mode:
    typeSChar ch;
    CSyncSerComFSM cntr;
    cntr.start(CSyncSerComFSM::FSM::sendSilenceFrame);
    while(cntr.proc(ch, msg))
    {
       if(!send_char(ch))
           return false;
    }
    return true;
}

void CSPIcomm::Update()
{
    if(!isIRQmode()) //if not IRQ mode, poll
    {
        IRQhandler();
    }

    //check: thread-safe
    bool bProc=false;
    __disable_irq();
        if(m_ComCntr.get_state()==CSyncSerComFSM::FSM::recOK)
        {
            m_recFIFO.dumpres(m_recFIFOhold);
            m_ComCntr.start(CSyncSerComFSM::FSM::halted);
            bProc=true;
        }
    __enable_irq();

    if(bProc)
    {
        while(m_recFIFOhold.in_avail())
        {
            typeSChar ch;
            m_recFIFOhold>>ch;
            Fire_on_rec_char(ch);
        }
    }
}

