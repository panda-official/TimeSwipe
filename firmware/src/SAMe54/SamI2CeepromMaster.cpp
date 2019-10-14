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
    //......

    //enable:
    pI2Cm->CTRLA.bit.ENABLE=1;
}

//IRQ handling:
void CSamI2CeepromMaster::IRQhandler()
{
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(m_nSercom);

    if(pI2Cm->INTFLAG.bit.MB) //master on bus
    {
        //write data:
        if(FSM::addrHb==m_MState)
        {
            //set_addr_H(pI2C->DATA.reg); write addr HB
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
            }
            return;
        }
        if(FSM::write==m_MState)
        {
            //write data until the end
            //........................
            return;
        }
        return;
    }
    if(pI2Cm->INTFLAG.bit.SB) //slave on bus
    {
        //read data untill the end
        //.........
        return;
    }
    if(pI2Cm->INTFLAG.bit.ERROR) //proc an error: LENERR, SEXTTOUT, MEXTTOUT, LOWTOUT, ARBLOST, and BUSERR
    {
        //proc an error:
        //.............
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

