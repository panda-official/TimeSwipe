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

#include "../common/SPI.h"
#include "../common/SyncCom.h"

#include "../3rdparty/BCMsrc/bcm2835.h"

#include <iostream>

class BcmLib {
protected:
  inline static bool is_initialized_;
  inline static bool is_spi_initialized_[2];

  BcmLib()
  {
    if (is_initialized_)
      return;

    if (!bcm2835_init())
      return;

    is_initialized_ = true;
  }

  ~BcmLib()
  {
    if (is_spi_initialized_[iSPI::SPI0])
      bcm2835_spi_end();

    if (is_spi_initialized_[iSPI::SPI1])
      bcm2835_aux_spi_end();

    if (is_initialized_)
      bcm2835_close();
  }

public:
  enum iSPI {
    SPI0,
    SPI1
  };

  bool init_SPI(iSPI nSPI)
  {
    if (is_spi_initialized_[nSPI])
      return true;

    return is_spi_initialized_[nSPI] =
      (iSPI::SPI0 == nSPI) ? bcm2835_spi_begin() : bcm2835_aux_spi_begin();
  }

  Character SPItransfer(iSPI nSPI, Character ch)
  {
    if (iSPI::SPI0 != nSPI) {
      char t=ch;
      char r;
      _bcm_aux_spi_transfernb(&t, &r, 1, 1);
      return r;
    } else {
      _bcm_spi_send_char(ch);
      return _bcm_spi_rec_char();
    }
  }

  void SPI_purge(iSPI nSPI)
  {
    if (iSPI::SPI0 == nSPI)
      _bcm_spi_purge();
  }

  void SPI_setCS(iSPI nSPI, bool how)
  {
    if (iSPI::SPI0 != nSPI) {
      char t{};
      char r;
      _bcm_aux_spi_transfernb(&t, &r, 1, how ? 1:0);
    } else
      _bsm_spi_cs( how ? 1:0);
  }

  void SPI_waitDone(iSPI nSPI)
  {
    if (iSPI::SPI0 == nSPI) while (!_bsm_spi_is_done()){}
  }

  void SPI_set_speed_hz(iSPI nSPI, uint32_t speed_hz)
  {
    if (iSPI::SPI0 == nSPI)
      bcm2835_spi_set_speed_hz(speed_hz);
    else
      bcm2835_aux_spi_setClockDivider(bcm2835_aux_spi_CalcClockDivider(speed_hz));
  }
};

class CBcmSPI : public CSPI, public BcmLib {
protected:
  iSPI spi_;
  CFIFO rec_fifo_;

public:
  CSyncSerComFSM com_cntr_;

public:
  CBcmSPI(iSPI nSPI=iSPI::SPI0)
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
