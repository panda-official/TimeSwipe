/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include "SamI2Cmem.h"
#include "sam.h"

Sercom *glob_GetSercomPtr(typeSamSercoms nSercom);
#define SELECT_SAMI2C(nSercom) &(glob_GetSercomPtr(nSercom)->I2CS)

CSamI2Cmem::CSamI2Cmem() : CSamSercom(typeSamSercoms::Sercom7)
{
    //----------setup PINs: IOSET1 PD08, PD09, PD10, PD11----------------
 /*   //PD08 -> group 3, even, function "C"(PAD0)=0x02: SDA
    PORT->Group[3].PMUX[4].bit.PMUXE=0x02;
    PORT->Group[3].PINCFG[8].bit.PMUXEN=1; //enable

    //PD09 -> group 3, odd, function "C"(PAD1)=0x02:  SDL
    PORT->Group[3].PMUX[4].bit.PMUXO=0x02;
    PORT->Group[3].PINCFG[9].bit.PMUXEN=1; //enable*/
    //-------------------------------------------------------------------

    //----------setup PINs: Version2: PA16,PA17----------------
    PORT->Group[0].PMUX[8].bit.PMUXE=0x02; //(PAD0)
    PORT->Group[0].PINCFG[16].bit.PMUXEN=1; //enable

    PORT->Group[0].PMUX[8].bit.PMUXO=0x02; //(PAD1)
    PORT->Group[0].PINCFG[17].bit.PMUXEN=1; //enable
    //---------------------------------------------------------



    SercomI2cs *pI2C=SELECT_SAMI2C(m_nSercom);

    CSamSercom::EnableSercomBus(m_nSercom, true);

    m_pCLK=CSamCLK::Factory();

    //connect:
    ConnectGCLK(m_nSercom, m_pCLK->CLKind());
    m_pCLK->Enable(true);

    //setup slave mode:
    pI2C->CTRLA.bit.MODE=0x04;

    //SDA Hold Time value:
    //pI2C->CTRLA.bit.SDAHOLD=

    //stretching control:
    //pI2C->CTRLA.bit.SCLSM=1;

    //the minimum slave setup time for the SDA:
    //pI2C->CTRLC.bit.SDASETUP=

    //enable smart operation:
    pI2C->CTRLB.bit.SMEN=1; //auto ack by DATA read

    //enable SCL low time-out:
    //pI2C->CTRLA.bit.LOWTOUT=

    //Configure the address match configuration:
    pI2C->CTRLB.bit.AMODE=0; //default
   // pI2C->CTRLB.bit.AACKEN=1; //automatic ACK if an adress match, don't need corresponding IRQ!!!

    pI2C->ADDR.bit.ADDR=0x50;
    pI2C->ADDR.bit.ADDRMASK=0;

    //enabling:
    pI2C->CTRLA.bit.ENABLE=1;
}
bool CSamI2Cmem::send(CFIFO &msg){ return false;}
 bool CSamI2Cmem::receive(CFIFO &msg){return false;}
 bool CSamI2Cmem::send(typeSChar ch){return false;}
 bool CSamI2Cmem::receive(typeSChar &ch){return false;}

//IRQ handling:
void CSamI2Cmem::IRQhandler()
{
    SercomI2cs *pI2C=SELECT_SAMI2C(m_nSercom);
    if(pI2C->INTFLAG.bit.AMATCH) //adress match
    {
        if(pI2C->STATUS.bit.DIR) //read
        {
            m_MState=FSM::read;
        }
        else                 //write
        {
            m_MState=FSM::addrHb;
        }
        pI2C->CTRLB.bit.CMD=3;
        //pI2C->INTFLAG.bit.AMATCH=1;
 //       pI2C->INTFLAG.bit.DRDY=1;
        return;
    }
    if(pI2C->INTFLAG.bit.DRDY)  //data ready
    {
     /*   if(!m_MState)
        {
            //int data=pI2C->DATA.reg;
            //return;
            m_MState=pI2C->STATUS.bit.DIR ? FSM::read :FSM::addrHb;
        }*/

      /*  if(pI2C->INTFLAG.bit.AMATCH) //adress match
        {
            if(pI2C->STATUS.bit.DIR) //read
            {
                m_MState=FSM::read;
            }
            else                 //write
            {
                m_MState=FSM::addrHb;
            }
         }*/

        if(FSM::read==m_MState)
        {
            pI2C->DATA.reg=readB(); return;
        }
        if(FSM::write==m_MState)
        {
            writeB(pI2C->DATA.reg); return;
            return;
        }
        if(FSM::addrHb==m_MState)
        {
            set_addr_H(pI2C->DATA.reg);
            m_MState=FSM::addrLb;
            return;
        }
        //else //if(FSM::addrLb==m_MState)
        {
            set_addr_L(pI2C->DATA.reg);
            m_MState=FSM::write;
        }

    }
    if(pI2C->INTFLAG.bit.ERROR)
    {
        m_MState=FSM::halted;
        pI2C->INTFLAG.bit.ERROR=1;
    }
    if(pI2C->INTFLAG.bit.PREC)  //stop condition
    {
        m_MState=FSM::halted;
        pI2C->INTFLAG.bit.PREC=1;
    }
}

void CSamI2Cmem::OnIRQ0()  //#0
{
    IRQhandler();
}
void CSamI2Cmem::OnIRQ1()  //#1
{
    IRQhandler();
}
void CSamI2Cmem::OnIRQ2()  //#2
{
    IRQhandler();
}
void CSamI2Cmem::OnIRQ3()  //#3
{
    IRQhandler();
}

void CSamI2Cmem::EnableIRQs(bool how)
{
    //select ptr:
    SercomI2cs *pI2C=SELECT_SAMI2C(m_nSercom);

    m_bIRQmode=how;
    if(how)
    {
        pI2C->INTENSET.reg=(SERCOM_I2CS_INTENSET_PREC|SERCOM_I2CS_INTENSET_AMATCH|SERCOM_I2CS_INTENSET_DRDY|SERCOM_I2CS_INTENSET_ERROR);
        //pI2C->INTENSET.reg=(SERCOM_I2CS_INTENSET_PREC|SERCOM_I2CS_INTENSET_DRDY|SERCOM_I2CS_INTENSET_ERROR);
    }
    else
    {
        //clear all:
        pI2C->INTENCLR.reg=SERCOM_I2CS_INTENSET_MASK;
    }

    //tune NVIC:
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ0, how);
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ1, how);
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ2, how);
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ3, how);
}

//mem interface:
int CSamI2Cmem::readB()
{
    if(m_nMemCurInd>=m_nMemSize)
        return -1;

    return m_pMem[m_nMemCurInd++];
}
int CSamI2Cmem::writeB(int val) //forbidden
{
    if(m_nMemCurInd>=m_nMemSize)
        return -1;

  //  m_pMem[m_nMemCurInd++]=val;
    return 0;
}
void CSamI2Cmem::set_addr_H(int addr)
{
    m_nMemCurInd=(addr<<8);
}
void CSamI2Cmem::set_addr_L(int addr)
{
    m_nMemCurInd+=addr;
}

