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
#define SYNC_BUS(pBus) while(pBus->SYNCBUSY.bit.SYSOP){}


CSamI2CeepromMaster::CSamI2CeepromMaster() : CSamSercom(typeSamSercoms::Sercom6)
{
    CSamSercom::EnableSercomBus(m_nSercom, true);
    m_pCLK=CSamCLK::Factory();

    //connect:
    ConnectGCLK(m_nSercom, m_pCLK->CLKind());
    m_pCLK->Enable(true);

    setup_bus();
}

void CSamI2CeepromMaster::reset_chip_logic()
{
    //! disconnecting pins from I2C bus since we cannot use its interface

    PORT->Group[3].PINCFG[8].bit.PMUXEN=0;
    PORT->Group[3].PINCFG[9].bit.PMUXEN=0;

    //! performing a manual 10-period clock sequence - this will reset a chip

    PORT->Group[3].OUTCLR.reg=(1L<<8);
    for(int i=0; i<10; i++)
    {
        PORT->Group[3].DIRSET.reg=(1L<<8); //should go to 0
        os::wait(1);
        PORT->Group[3].DIRCLR.reg=(1L<<8); //back by pull up...
        os::wait(1);
    }
}
void CSamI2CeepromMaster::setup_bus()
{
    //SCL:
    PORT->Group[3].PMUX[4].bit.PMUXE=0x03;
    PORT->Group[3].PINCFG[8].bit.PMUXEN=1; //enable

    //SDA:
    PORT->Group[3].PMUX[4].bit.PMUXO=0x03;
    PORT->Group[3].PINCFG[9].bit.PMUXEN=1; //enable

    /*! "Violating the protocol may cause the I2C to hang. If this happens it is possible to recover from this
    *   state by a software Reset (CTRLA.SWRST='1')." page 1026
    */

    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);
    while(pI2Cm->SYNCBUSY.bit.SWRST){}
    pI2Cm->CTRLA.bit.SWRST=1;
    while(pI2Cm->CTRLA.bit.SWRST){}


    pI2Cm->CTRLA.bit.MODE=0x05;

    //setup:
    pI2Cm->CTRLA.bit.INACTOUT=1;  //55uS shoud be enough
    pI2Cm->CTRLB.bit.ACKACT=0;    //sending ACK after the bit is received (auto)
    pI2Cm->BAUD.bit.BAUD=0xff;

    //! if it was an IRQ mode don't forget to restart it:
    if(m_bIRQmode)
    {
        EnableIRQs(true);
    }

    //enable:
    pI2Cm->CTRLA.bit.ENABLE=1;

    while(0==pI2Cm->STATUS.bit.BUSSTATE)
    {
        SYNC_BUS(pI2Cm)
        pI2Cm->STATUS.bit.BUSSTATE=1;
    }
}

void CSamI2CeepromMaster::check_reset()
{
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);

    if(3==pI2Cm->STATUS.bit.BUSSTATE) //chip hangs...
    {
        reset_chip_logic();
        setup_bus();
    }
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

    check_reset(); //! check chip & bus states before transfer, reset if needed

    m_IOdir=how;
    m_MState=FSM::start;
    SYNC_BUS(pI2Cm)
    pI2Cm->CTRLB.bit.ACKACT=0;      //sets "ACK" action
    SYNC_BUS(pI2Cm)
    pI2Cm->ADDR.bit.ADDR=m_nDevAddr; //this will initiate a transfer sequence

}

//IRQ handling:
void CSamI2CeepromMaster::IRQhandler()
{
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);

    SYNC_BUS(pI2Cm)

    //proc line error first:
    if(pI2Cm->INTFLAG.bit.ERROR) //proc an error: LENERR, SEXTTOUT, MEXTTOUT, LOWTOUT, ARBLOST, and BUSERR
    {

        pI2Cm->STATUS.reg=0xff; //clear status ?
        pI2Cm->INTFLAG.bit.ERROR=1;

        m_MState=errTransfer;
        return;
    }


    if(pI2Cm->INTFLAG.bit.MB) //master on bus
    {
        if(pI2Cm->STATUS.bit.ARBLOST || pI2Cm->STATUS.bit.RXNACK)
        {
            //stop the communication:
            m_MState=errTransfer;
            pI2Cm->CTRLB.bit.CMD=0x3; //stop
            return;
            //how the flags will be reset? by a new start?
            //!"This bit is automatically cleared when writing to the ADDR register" (Manual)
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
                //nothing to do, stop:
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
            return;
        }
        //read data untill the end
        if(writeB(pI2Cm->DATA.bit.DATA)<0) //EOF
        {
            m_MState=FSM::halted;
            pI2Cm->CTRLB.bit.ACKACT=1; //setting "NACK" to the chip
            SYNC_BUS(pI2Cm)
            pI2Cm->CTRLB.bit.CMD=0x3; //stop
            return;
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

void CSamI2CeepromMaster::rewindMemBuf()
{
    if(m_pBuf)
        m_pBuf->rewind();
}
int CSamI2CeepromMaster::writeB(int val)
{
    if(!m_pBuf)
        return -1;

    if(m_pBuf->size()>=m_nReadDataCountLim) //memmory protection
        return -1;

    (*m_pBuf)<<val;
    return val;
}
