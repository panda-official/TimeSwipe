/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include "os.h"
#include "SamI2CeepromMaster.h"

#include "sam.h"

//#define EEPROM_8BIT_ADDR

Sercom *glob_GetSercomPtr(typeSamSercoms nSercom);
#define SELECT_SAMI2CM(nSercom) &(glob_GetSercomPtr(nSercom)->I2CM) //ptr to a master section


CSamI2CeepromMaster::CSamI2CeepromMaster() : CSamSercom(typeSamSercoms::Sercom6)
{

    //----------setup PINs: IOSET1 PD08, PD09, PD10----------------
    //PD08 -> group 3, even, function "C"(PAD1)=0x02: SDL
    PORT->Group[3].PMUX[4].bit.PMUXE=0x03;
    PORT->Group[3].PINCFG[8].bit.PMUXEN=1; //enable

    //PD09 -> group 3, odd, function "C"(PAD0)=0x02:  SDA
    PORT->Group[3].PMUX[4].bit.PMUXO=0x03;
    PORT->Group[3].PINCFG[9].bit.PMUXEN=1; //enable


    //tune I2C master:
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);

    CSamSercom::EnableSercomBus(m_nSercom, true);

    //perform a soft reset before use:
    pI2Cm->CTRLA.bit.SWRST=1;
    while(pI2Cm->SYNCBUSY.bit.SWRST){}
    while(pI2Cm->CTRLA.bit.SWRST){}

    m_pCLK=CSamCLK::Factory();

    //connect:
    ConnectGCLK(m_nSercom, m_pCLK->CLKind());
    m_pCLK->Enable(true);

    //setup I2C master mode:
    pI2Cm->CTRLA.bit.MODE=0x05;
    //pI2Cm->CTRLA.bit.SCLSM=0; //def

    //setup:
    //pI2Cm->CTRLB.bit.SMEN=1;        //smart mode is enabled
    pI2Cm->CTRLA.bit.SCLSM=1;
    pI2Cm->CTRLA.bit.INACTOUT=1; //0x01; //55uS shoud be enough
    pI2Cm->CTRLB.bit.ACKACT=0; //0x01;   //sending ACK after the bit is received (auto)
    pI2Cm->BAUD.bit.BAUD=0xff;
    pI2Cm->BAUD.bit.BAUDLOW=0 ; //0xFF;

    //enable:
    pI2Cm->CTRLA.bit.ENABLE=1;

    //pI2Cm->STATUS.bit.BUSSTATE=1;
    while(0==pI2Cm->STATUS.bit.BUSSTATE)
    {
        pI2Cm->STATUS.bit.BUSSTATE=1;
    }
}

bool CSamI2CeepromMaster::send(CFIFO &msg)  //blocking call
{
    return false;
}
bool CSamI2CeepromMaster::receive(CFIFO &msg) //blocking call
{
    m_nCurMemAddr=m_nMemAddr;
    m_pBuf=&msg;
    StartTranfer(false);
    unsigned long StartWaitTime=os::get_tick_mS();
    while(FSM::halted!=m_MState && errTransfer!=m_MState)
    {
        if( (os::get_tick_mS()-StartWaitTime)>m_OpTmt_mS )
            break;
        os::wait(50);
    }
    m_pBuf=nullptr;
    return (FSM::halted==m_MState);
}

//helpers:
void CSamI2CeepromMaster::StartTranfer(bool how)
{
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);

    m_IOdir=how;
    m_MState=FSM::start;
    pI2Cm->ADDR.bit.ADDR=m_nDevAddr; //this will initiate a transfer sequence
}

//IRQ handling:
void CSamI2CeepromMaster::IRQhandler()
{
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);

    //proc line error first:
    if(pI2Cm->INTFLAG.bit.ERROR) //proc an error: LENERR, SEXTTOUT, MEXTTOUT, LOWTOUT, ARBLOST, and BUSERR
    {

        pI2Cm->STATUS.reg=0xff; //clear status ?
        pI2Cm->INTFLAG.bit.ERROR=1;

        m_MState=errTransfer; //???
        return;
    }


    if(pI2Cm->INTFLAG.bit.MB) //master on bus
    {
        if(pI2Cm->STATUS.bit.ARBLOST || pI2Cm->STATUS.bit.RXNACK)
        {
            //stop the communication:
            m_MState=errTransfer;
            pI2Cm->CTRLB.bit.CMD=0x3; //stop
            //pI2Cm->INTFLAG.bit.MB=1;
            return;
            //how the flags will be reset? by a new start?
            //"This bit is automatically cleared when writing to the ADDR register" (Manual)
        }

        if(FSM::start==m_MState)
        {
            //set address HB:
#ifdef EEPROM_8BIT_ADDR
            m_MState=FSM::addrLb;
            pI2Cm->DATA.bit.DATA=(m_nCurMemAddr/m_nPageSize);
#else
            m_MState=FSM::addrHb;
            pI2Cm->DATA.bit.DATA=(m_nCurMemAddr>>8);
#endif
            return;
        }
        if(FSM::addrHb==m_MState)
        {
            m_MState=FSM::addrLb;
            pI2Cm->DATA.bit.DATA=(m_nCurMemAddr&0xff);
            return;
        }
        if(FSM::addrLb==m_MState)
        {
            //after setting the addres switch the direction: R or W
            if(m_IOdir) //write
            {
                //nothing to do
                m_MState=FSM::halted;
                pI2Cm->CTRLB.bit.CMD=0x3; //stop
            }
            else
            {
                //initiating a repeated start for read:
                m_MState=FSM::read;
                pI2Cm->ADDR.bit.ADDR=m_nDevAddr+1;
            }
            return;
        }
        pI2Cm->INTFLAG.bit.MB=1;
        return;
    }
    if(pI2Cm->INTFLAG.bit.SB) //slave on bus
    {
        //if data is "ended" at the slave (setting NACK)
        if(pI2Cm->STATUS.bit.RXNACK)
        {
            m_MState=FSM::halted;
            pI2Cm->CTRLB.bit.CMD=0x3; //stop
        }
        //read data untill the end
        else if(writeB(pI2Cm->DATA.bit.DATA)<0) //EOF
        {
            m_MState=FSM::halted;
            pI2Cm->CTRLB.bit.CMD=0x3; //stop
        }
        pI2Cm->CTRLB.bit.CMD=0x2;
        pI2Cm->INTFLAG.bit.SB=1;
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
int CSamI2CeepromMaster::writeB(int val)
{
    if(!m_pBuf)
        return -1;

    if(m_pBuf->in_avail()>=m_nReadDataCountLim) //memmory protection
        return -1;

    (*m_pBuf)<<val;
    return val;
}
