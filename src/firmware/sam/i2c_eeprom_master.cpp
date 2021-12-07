// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "../error.hpp"
#include "../os.h"
#include "i2c_eeprom_master.hpp"

#include <sam.h>

//#define EEPROM_8BIT_ADDR

Sercom *glob_GetSercomPtr(Sam_sercom::Id nSercom);
#define SELECT_SAMI2CM(nSercom) &(glob_GetSercomPtr(nSercom)->I2CM) //ptr to a master section

Sam_i2c_eeprom_master::Sam_i2c_eeprom_master()
  : Sam_sercom{Id::sercom6}
{
  PORT->Group[3].DIRSET.reg = (1L<<10);
  SetWriteProtection(true);

  enable_internal_bus(true);
  m_pCLK = Sam_clock_generator::make();
  PANDA_TIMESWIPE_FIRMWARE_ASSERT(m_pCLK);

  connect_clock_generator(m_pCLK->id());
  m_pCLK->enable(true);

  setup_bus();
}

void Sam_i2c_eeprom_master::reset_chip_logic()
{
    //! disconnecting pins from I2C bus since we cannot use its interface

    PORT->Group[3].PINCFG[8].bit.PMUXEN=0;
    PORT->Group[3].PINCFG[9].bit.PMUXEN=0;

    //! performing a manual 10-period clock sequence - this will reset the chip

    PORT->Group[3].OUTCLR.reg=(1L<<8);
    for(int i=0; i<10; i++)
    {
        PORT->Group[3].DIRSET.reg=(1L<<8); //should go to 0
        os::wait(1);
        PORT->Group[3].DIRCLR.reg=(1L<<8); //back by pull up...
        os::wait(1);
    }
}

#define SYNC_BUS(pBus) while(pBus->SYNCBUSY.bit.SYSOP){}
void Sam_i2c_eeprom_master::setup_bus()
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

    SercomI2cm *pI2Cm=SELECT_SAMI2CM(id());
    while(pI2Cm->SYNCBUSY.bit.SWRST){}
    pI2Cm->CTRLA.bit.SWRST=1;
    while(pI2Cm->CTRLA.bit.SWRST){}


    pI2Cm->CTRLA.bit.MODE=0x05;

    //setup:
    //pI2Cm->CTRLB.bit.SMEN=1;    //smart mode is enabled
    //pI2Cm->CTRLA.bit.SCLSM=1;
    pI2Cm->CTRLA.bit.INACTOUT=1;  //55uS shoud be enough
    pI2Cm->CTRLB.bit.ACKACT=0;    //sending ACK after the bit is received (auto)
    pI2Cm->BAUD.bit.BAUD=0xff;
   // pI2Cm->BAUD.bit.BAUDLOW=0 ; //0xFF;

    //! if it was an IRQ mode don't forget to restart it:

    if (is_irq_enabled_)
      enable_irq(true);

    //enable:
    pI2Cm->CTRLA.bit.ENABLE=1;

    while(0==pI2Cm->STATUS.bit.BUSSTATE)
    {
        SYNC_BUS(pI2Cm)
        pI2Cm->STATUS.bit.BUSSTATE=1;
    }
}

void Sam_i2c_eeprom_master::check_reset()
{
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(id());

    if(3==pI2Cm->STATUS.bit.BUSSTATE) //chip is hanging...
    {
        reset_chip_logic();
        setup_bus();
    }
}



void Sam_i2c_eeprom_master::SetWriteProtection(const bool activate)
{
    if(activate)
    {
        PORT->Group[3].OUTSET.reg=(1L<<10);
        os::uwait(100);                      //wait till real voltage level rise of fall
    }
    else
    {
        os::uwait(100);                      //wait till real voltage level rise of fall
        PORT->Group[3].OUTCLR.reg=(1L<<10);
    }
}

bool Sam_i2c_eeprom_master::write_next_page() //since only 1 page can be written at once
{
    int pbl=page_size() - m_nCurMemAddr%page_size();
    m_nPageBytesLeft=pbl;
    StartTransfer(Io_direction::write);
    unsigned long StartWaitTime=os::get_tick_mS();
    while(State::halted!=state_ && State::errTransfer!=state_)
    {
        if( (os::get_tick_mS()-StartWaitTime)>operation_timeout() )
            return false;
        //os::wait(1); //regulate the cpu load here
    }
    if(State::halted==state_)
    {
        m_nCurMemAddr+=pbl;
        return true;
    }
    return false;
}

bool Sam_i2c_eeprom_master::__send(CFIFO &msg)
{
    m_nCurMemAddr=m_nMemAddr; //rewind mem addr
    m_pBuf=&msg;
    bool bPageWriteResult;
    unsigned long StartWaitTime=os::get_tick_mS();
    do{

        bPageWriteResult=write_next_page();
        if(bPageWriteResult)
        {
            StartWaitTime=os::get_tick_mS();
        }

    }
    while(msg.in_avail() && (os::get_tick_mS()-StartWaitTime)<operation_timeout());
    m_pBuf=nullptr;
    return bPageWriteResult;
}

bool Sam_i2c_eeprom_master::__sendRB(CFIFO &msg)
{
    m_nCurMemAddr=m_nMemAddr;
    m_pBuf=&msg;
    m_bCmpReadMode=true;
    StartTransfer(Io_direction::read);
    unsigned long StartWaitTime=os::get_tick_mS();
    while(State::halted!=state_ && State::errTransfer!=state_)
    {
        if( (os::get_tick_mS()-StartWaitTime)>operation_timeout() )
            break;
        os::wait(1);
    }
    m_pBuf=nullptr;
    return (State::halted==state_);
}

bool Sam_i2c_eeprom_master::send(CFIFO& msg)
{
    constexpr int write_retries{3};
    bool bPageWriteResult=false;
SetWriteProtection(false);
    for(int i{}; i < write_retries; i++){

        msg.rewind();
        if(__send(msg))
        {
            //some delay is required:
            os::wait(10);

            msg.rewind();
            if(__sendRB(msg))
            {
                bPageWriteResult=true;
                break;
            }
        }
     }
SetWriteProtection(true);
    return bPageWriteResult;
}
bool Sam_i2c_eeprom_master::receive(CFIFO& msg)
{
    m_nCurMemAddr=m_nMemAddr;
    m_pBuf=&msg;
    m_bCmpReadMode=false;
    StartTransfer(Io_direction::read);
    unsigned long StartWaitTime=os::get_tick_mS();
    while(State::halted!=state_ && State::errTransfer!=state_)
    {
        if( (os::get_tick_mS()-StartWaitTime)>operation_timeout() )
            break;
        os::wait(1);
    }
    m_pBuf=nullptr;
    return (State::halted==state_);
}

//helpers:
void Sam_i2c_eeprom_master::StartTransfer(const Io_direction dir)
{
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(id());

    check_reset(); //! check chip & bus states before transfer, reset if needed

    io_direction_ = dir;
    state_ = State::start;
    SYNC_BUS(pI2Cm)
    pI2Cm->CTRLB.bit.ACKACT = 0;      //sets "ACK" action
    SYNC_BUS(pI2Cm)
    pI2Cm->ADDR.bit.ADDR = m_nDevAddr; //this will initiate a transfer sequence

}

void Sam_i2c_eeprom_master::run_self_test(bool)
{
  /*
   * @brief Tests selected EEPROM area.
   *
   * @param pattern A pattern to test with.
   * @param offset A start address of EEPROM area.
   *
   * @returns `true` on success.
   */
  const auto is_mem_area_ok = [this](CFIFO& pattern, const int offset)
  {
    CFIFO buf;

    pattern.rewind();
    const auto pattern_size = static_cast<std::size_t>(pattern.in_avail());
    buf.reserve(pattern_size);

    const int nPrevAddr{m_nMemAddr};
    m_nMemAddr=offset;

    if (!__send(pattern)) {
      m_nMemAddr = nPrevAddr;
      return false;
    }

    // Some delay is required.
    os::wait(10);

    const bool rb{receive(buf)};
    m_nMemAddr = nPrevAddr;
    if (!rb)
      return false;

    // Compare.
    for (int k{}; k < pattern_size; ++k)
      if (buf[k] != pattern[k])
        return false;

    return true;
  };

  // Perform self-test.
  SetWriteProtection(false);
  self_test_result_ = [&]
  {
    CFIFO pattern;
    for (int i{}; i < page_size(); ++i)
      pattern << 0xA5;

    // Test first page.
    if (!is_mem_area_ok(pattern, 0))
      return false;

    // Test last page.
    if (!is_mem_area_ok(pattern, m_nReadDataCountLim - page_size()))
      return false;

    return true;
  }();
  SetWriteProtection(true);
}

//IRQ handling:
void Sam_i2c_eeprom_master::handle_irq()
{
    SercomI2cm *pI2Cm=SELECT_SAMI2CM(id());

    SYNC_BUS(pI2Cm)

    //proc line error first:
    if(pI2Cm->INTFLAG.bit.ERROR) //proc an error: LENERR, SEXTTOUT, MEXTTOUT, LOWTOUT, ARBLOST, and BUSERR
    {

        pI2Cm->STATUS.reg=0xff; //clear status ?
        pI2Cm->INTFLAG.bit.ERROR=1;

        state_=State::errTransfer; //???
        return;
    }


    if(pI2Cm->INTFLAG.bit.MB) //master on bus
    {
        if(pI2Cm->STATUS.bit.ARBLOST || pI2Cm->STATUS.bit.RXNACK)
        {
            //stop the communication:
            state_=State::errTransfer;
            pI2Cm->CTRLB.bit.CMD=0x3; //stop
            //pI2Cm->INTFLAG.bit.MB=1;
            return;
            //how the flags will be reset? by a new start?
            //"This bit is automatically cleared when writing to the ADDR register" (Manual)
        }

        if(State::start==state_)
        {
            //set address HB:
#ifdef EEPROM_8BIT_ADDR
            state_=State::addrLb;
            pI2Cm->DATA.bit.DATA=(m_nCurMemAddr/page_size());
#else
            state_=State::addrHb;
            pI2Cm->DATA.bit.DATA=(m_nCurMemAddr>>8);
#endif
            return;
        }
        if(State::addrHb==state_)
        {
            state_=State::addrLb;
            pI2Cm->DATA.bit.DATA=(m_nCurMemAddr&0xff);
            return;
        }
        if(State::addrLb==state_)
        {
          // After setting the addres switch the IO direction.
          if (io_direction_ == Io_direction::write) {
                //nothing to do, just continue writing:
                state_=State::write;
            }
            else
            {
                //initiating a repeated start for read:
                state_=State::read;
                pI2Cm->ADDR.bit.ADDR=m_nDevAddr+1;
            }
            return;
        }
        if(State::write==state_)
        {
            //write data until the end
            const int val = read_byte();
            if (val >= 0)
            {
                pI2Cm->DATA.bit.DATA=val;
            }
            else {
                //EOF:
                state_=State::halted;
                pI2Cm->CTRLB.bit.CMD=0x3; //stop
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
            state_=State::halted;
            pI2Cm->CTRLB.bit.CMD=0x3; //stop
            return;
        }
        //read data untill the end
        if(writeB(pI2Cm->DATA.bit.DATA)<0) //EOF
        {
            if(State::errCmp!=state_){
                state_=State::halted;
            }
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

void Sam_i2c_eeprom_master::handle_irq0()
{
  handle_irq();
}

void Sam_i2c_eeprom_master::handle_irq1()
{
  handle_irq();
}

void Sam_i2c_eeprom_master::handle_irq2()
{
  handle_irq();
}

void Sam_i2c_eeprom_master::handle_irq3()
{
  handle_irq();
}

void Sam_i2c_eeprom_master::enable_irq(const bool enabled)
{
  is_irq_enabled_ = enabled;
  SercomI2cm* const i2cm = SELECT_SAMI2CM(id());
  PANDA_TIMESWIPE_FIRMWARE_ASSERT(i2cm);
  if (is_irq_enabled_)
    i2cm->INTENSET.reg =
      SERCOM_I2CM_INTENSET_MB |   // master on bus
      SERCOM_I2CM_INTENSET_SB |   // slave on bus
      SERCOM_I2CM_INTENSET_ERROR; // bus error
  else
    // Clear all.
    i2cm->INTENCLR.reg = SERCOM_I2CM_INTENSET_MASK;

  // Tune NVIC.
  for (const auto irq : {Irq::irq0, Irq::irq1, Irq::irq2, Irq::irq3})
    Sam_sercom::enable_irq(irq, is_irq_enabled_);
}

//+++new mem int:
void Sam_i2c_eeprom_master::rewindMemBuf()
{
    if(m_pBuf)
        m_pBuf->rewind();
}
int Sam_i2c_eeprom_master::read_byte()
{
    if(!m_pBuf)
        return -1;
    if((m_nPageBytesLeft--)<=0)
        return -1;
    if(0==m_pBuf->in_avail())
        return -1;

    Character ch;
    (*m_pBuf)>>ch;
    return ch;
}
int Sam_i2c_eeprom_master::writeB(int val)
{
    if(!m_pBuf)
        return -1;

    if(m_bCmpReadMode) {
        if(0==m_pBuf->in_avail())
            return -1;

        Character ch;
        (*m_pBuf)>>ch;
        if(ch!=val)
        {
            state_=State::errCmp;
            return -1;
        }
    } else {
        if(m_pBuf->size()>=m_nReadDataCountLim) //memmory protection
            return -1;

        (*m_pBuf)<<val;
    }
    return val;
}
