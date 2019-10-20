/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include "SamI2CeepromMaster.h"

#include "sam.h"

Sercom *glob_GetSercomPtr(typeSamSercoms nSercom);
#define SELECT_SAMI2CM(nSercom) &(glob_GetSercomPtr(nSercom)->I2CM) //ptr to a master section


CSamI2CeepromMaster::CSamI2CeepromMaster() : CSamSercom(typeSamSercoms::Sercom6)
{

    //----------setup PINs: IOSET1 PD08, PD09, PD10, PD11----------------
    //PD08 -> group 3, even, function "C"(PAD1)=0x02: SDL
    PORT->Group[3].PMUX[4].bit.PMUXE=0x03;
    PORT->Group[3].PINCFG[8].bit.PMUXEN=1; //enable

    //PD09 -> group 3, odd, function "C"(PAD0)=0x02:  SDA
    PORT->Group[3].PMUX[4].bit.PMUXO=0x03;
    PORT->Group[3].PINCFG[9].bit.PMUXEN=1; //enable

    //PD10 -> grop 3, chip write protection
    PORT->Group[3].DIRSET.reg=(1L<<10); //set dir to output
    //-------------------------------------------------------------------

    //tune I2C master:
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);

    CSamSercom::EnableSercomBus(m_nSercom, true);

    m_pCLK=CSamCLK::Factory();

    //connect:
    ConnectGCLK(m_nSercom, m_pCLK->CLKind());
    m_pCLK->Enable(true);

    //setup I2C master mode:
    pI2Cm->CTRLA.bit.MODE=0x05;

    //setup:
    pI2Cm->CTRLB.bit.SMEN=1;        //smart mode is enabled
    pI2Cm->CTRLA.bit.INACTOUT=0x01; //55uS shoud be enough

    //enable:
    pI2Cm->CTRLA.bit.ENABLE=1;
}

bool CSamI2CeepromMaster::send(CFIFO &msg)  //blocking call
{
    m_pBuf=&msg;
    StartTranfer(true);
    while(FSM::halted!=m_MState || errTransfer!=m_MState){}
    m_pBuf=nullptr;
    return (FSM::halted==m_MState);
}
bool CSamI2CeepromMaster::receive(CFIFO &msg) //blocking call
{
    m_pBuf=&msg;
    StartTranfer(false);
    while(FSM::halted!=m_MState || errTransfer!=m_MState){}
    m_pBuf=nullptr;
    return (FSM::halted==m_MState);
}

//helpers:
void CSamI2CeepromMaster::StartTranfer(bool how)
{
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);

    m_IOdir=how;
    m_MState=FSM::start;
    rewindMemBuf();
    pI2Cm->ADDR.bit.ADDR=m_nDevAddr; //this will initiate a transfer sequence

}

//IRQ handling:
void CSamI2CeepromMaster::IRQhandler()
{
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);

    if(pI2Cm->INTFLAG.bit.MB) //master on bus
    {
        if(FSM::start==m_MState)
        {
            //set address HB:
            pI2Cm->DATA.bit.DATA=(m_nDevAddr>>8);
            m_MState=FSM::addrHb;
            return;
        }
        if(FSM::addrHb==m_MState)
        {
            pI2Cm->DATA.bit.DATA=(m_nDevAddr&0xff);
            m_MState=FSM::addrLb;
            return;
        }
        if(FSM::addrLb==m_MState)
        {
            //after setting the addres switch the direction: R or W
            if(m_IOdir) //write
            {
                //nothing to do, just continue writing:
                m_MState=FSM::write;
            }
            else
            {
                //initiating a repeated start for read:
                pI2Cm->ADDR.bit.ADDR=m_nDevAddr+1;
                m_MState=FSM::read;
            }
            return;
        }
        if(FSM::write==m_MState)
        {
            //write data until the end
            int val=readB();
            if(val>=0)
            {
                pI2Cm->DATA.bit.DATA=val;
            }
            else {
                //EOF:
                m_MState=FSM::halted;
            }
            return;
        }
        return;
    }
    if(pI2Cm->INTFLAG.bit.SB) //slave on bus
    {
        //read data untill the end
        if(writeB(pI2Cm->DATA.bit.DATA)<0) //EOF
        {
            m_MState=FSM::halted;
        }
        return;
    }
    if(pI2Cm->INTFLAG.bit.ERROR) //proc an error: LENERR, SEXTTOUT, MEXTTOUT, LOWTOUT, ARBLOST, and BUSERR
    {
        //proc an error:
        m_MState=errTransfer; //???
        return;
    }


}

void CSamI2CeepromMaster::OnIRQ0()  //#0
{
    IRQhandler();
}
void CSamI2CeepromMaster::OnIRQ1()  //#1
{
    IRQhandler();
}
void CSamI2CeepromMaster::OnIRQ2()  //#2
{
    IRQhandler();
}
void CSamI2CeepromMaster::OnIRQ3()  //#3
{
    IRQhandler();
}

void CSamI2CeepromMaster::EnableIRQs(bool how)
{
    //select ptr:
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);

    m_bIRQmode=how;
    if(how)
    {
        //master on bus, slave on bus, bus error
        pI2Cm->INTENSET.reg=(SERCOM_I2CM_INTENSET_MB|SERCOM_I2CM_INTENSET_SB|SERCOM_I2CM_INTENSET_ERROR);
    }
    else
    {
        //clear all:
        pI2Cm->INTENCLR.reg=SERCOM_I2CM_INTENSET_MASK;
    }

    //tune NVIC:
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ0, how);
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ1, how);
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ2, how);
    CSamSercom::EnableIRQ(typeSamSercomIRQs::IRQ3, how);
}

//+++new mem int:
void CSamI2CeepromMaster::rewindMemBuf()
{
    if(m_pBuf)
        m_pBuf->rewind();
}
int CSamI2CeepromMaster::readB()
{
    if(!m_pBuf)
        return -1;
    if(0==m_pBuf->in_avail())
        return -1;

    typeSChar ch;
    (*m_pBuf)>>ch;
    return ch;
}
int CSamI2CeepromMaster::writeB(int val)
{
    if(!m_pBuf)
        return -1;

    (*m_pBuf)<<val;
    return val;
}


/*int CSamI2CeepromMaster::readB()
{
    if(m_nMemCurInd>=m_nMemSize)
        return -1;

    return m_pMem[m_nMemCurInd++];
}
int CSamI2CeepromMaster::writeB(int val)
{
    if(m_nMemCurInd>=m_nMemSize)
        return -1;

    m_pMem[m_nMemCurInd++]=val;
    return val;
}*/
