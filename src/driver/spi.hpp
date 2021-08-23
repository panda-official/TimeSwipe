// -*- C++ -*-

// PANDA TimeSwipe Project
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

#ifndef PANDA_TIMESWIPE_SPI_HPP
#define PANDA_TIMESWIPE_SPI_HPP

#include "bcm.hpp"
#include "../common/SPI.h"

#include <iostream>

class BcmSpi : public CSPI, public BcmLib {
protected:
  BcmLib::iSPI spi_;
  CFIFO rec_fifo_;

public:
  CSyncSerComFSM com_cntr_;

public:
  BcmSpi(BcmLib::iSPI nSPI = BcmLib::iSPI::SPI0)
  {
    spi_ = nSPI;
    if (!init_SPI(nSPI))
      return;

    //set default rate:
    //SPI_set_speed_hz(100000);
    SPI_set_speed_hz(50000);
  }

  bool is_initialzed()
  {
    return is_spi_initialized_[spi_];
  }

  Character SPItransfer(Character ch)
  {
    return BcmLib::SPItransfer(spi_, ch);
  }

  void SPI_purge()
  {
    BcmLib::SPI_purge(spi_);
  }

  void SPI_setCS(bool how)
  {
    BcmLib::SPI_setCS(spi_, how);
  }

  void SPI_waitDone()
  {
    BcmLib::SPI_waitDone(spi_);
  }

  void SPI_set_speed_hz(uint32_t speed_hz)
  {
    BcmLib::SPI_set_speed_hz(spi_, speed_hz);
  }

  bool send(CFIFO &msg) override
  {
    if (!is_initialzed())
      return false;

    SPI_purge();
    SPI_setCS(true);
    rec_fifo_.reset();

    // A delay CS to fall required.
    bcm2835_delay(20); //corresponds to 50KHz

    // Flow control.
    Character ch=0;
    com_cntr_.start(CSyncSerComFSM::FSM::sendLengthMSB);
    while (com_cntr_.proc(ch, msg))
      SPItransfer(ch);
    if (com_cntr_.bad())
      return false;

    // Provide add clock.
    SPItransfer(0);

    // Wait for a "done" state.
    SPI_waitDone();

    com_cntr_.start(CSyncSerComFSM::FSM::recSilenceFrame);
    do
      ch = SPItransfer(0); //provide a clock
    while (com_cntr_.proc(ch, rec_fifo_));

    SPI_setCS(false);

    // A delay for CS to rise required.
    bcm2835_delay(20); // corresponds to 50KHz

    return true;
  }

  bool receive(CFIFO& msg) override
  {
    if (!is_initialzed())
      return false;

    msg = rec_fifo_;
    return com_cntr_.get_state() == CSyncSerComFSM::FSM::recOK;
  }

  virtual bool send(Character /*ch*/)
  {
    return false;
  }

  virtual bool receive(Character& /*ch*/)
  {
    return false;
  }

  void set_phpol(bool /*bPhase*/, bool /*bPol*/) override
  {}

  void set_baud_div(unsigned char /*div*/) override
  {}

  void set_tprofile_divs(unsigned char /*CSminDel*/, unsigned char /*IntertransDel*/,
    unsigned char /*BeforeClockDel*/) override
  {}
};

#endif  // PANDA_TIMESWIPE_SPI_HPP
