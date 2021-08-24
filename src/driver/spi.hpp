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

#include <cstdint>
#include <iostream>

class BcmSpi final : public CSPI {
public:
  enum SpiPins {
    kSpi0,
    kAux
  };

  ~BcmSpi()
  {
    if (is_spi_initialized_[SpiPins::kAux])
      bcm2835_aux_spi_end();

    if (is_spi_initialized_[SpiPins::kSpi0])
      bcm2835_spi_end();

    if (is_initialized_)
      bcm2835_close();
  }

  BcmSpi(const SpiPins pins = SpiPins::kSpi0)
    : pins_{pins}
  {
    // FIXME: throw exceptions on errors instead of just return.

    /// Initialize BCM.
    if (is_initialized_)
      return;
    else if (!bcm2835_init())
      return;
    is_initialized_ = true;

    /// Initialize SPI.
    if (is_spi_initialized_[pins_])
      return;
    else if ( !(is_spi_initialized_[pins_] =
        (pins_ == SpiPins::kSpi0) ? bcm2835_spi_begin() : bcm2835_aux_spi_begin()))
      return;

    //set default rate:
    //SpiSetSpeed(100000);
    SpiSetSpeed(50000);
  }

  bool IsInitialized() const noexcept
  {
    return is_spi_initialized_[pins_];
  }

  CSyncSerComFSM::FSM GetFsmState() const noexcept
  {
    return com_cntr_.get_state();
  }

  bool send(CFIFO& msg) override
  {
    if (!IsInitialized())
      return false;

    SpiPurge();
    SpiSetCs(true);
    rec_fifo_.reset();

    // A delay CS to fall required.
    bcm2835_delay(20); //corresponds to 50KHz

    // Flow control.
    Character ch=0;
    com_cntr_.start(CSyncSerComFSM::FSM::sendLengthMSB);
    while (com_cntr_.proc(ch, msg))
      SpiTransfer(ch);
    if (com_cntr_.bad())
      return false;

    // Provide add clock.
    SpiTransfer(0);

    // Wait for a "done" state.
    SpiWaitDone();

    com_cntr_.start(CSyncSerComFSM::FSM::recSilenceFrame);
    do
      ch = SpiTransfer(0); //provide a clock
    while (com_cntr_.proc(ch, rec_fifo_));

    SpiSetCs(false);

    // A delay for CS to rise required.
    bcm2835_delay(20); // corresponds to 50KHz

    return true;
  }

  bool receive(CFIFO& msg) override
  {
    if (!IsInitialized())
      return false;

    msg = rec_fifo_;
    return com_cntr_.get_state() == CSyncSerComFSM::FSM::recOK;
  }

private:
  inline static bool is_initialized_;
  inline static bool is_spi_initialized_[2];

  CSyncSerComFSM com_cntr_;
  SpiPins pins_;
  CFIFO rec_fifo_;

  bool send(Character /*ch*/)
  {
    return false;
  }

  bool receive(Character& /*ch*/)
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

  Character SpiTransfer(const Character ch)
  {
    if (pins_ != SpiPins::kSpi0) {
      char t = ch;
      char r{};
      _bcm_aux_spi_transfernb(&t, &r, 1, 1);
      return r;
    } else {
      _bcm_spi_send_char(ch);
      return _bcm_spi_rec_char();
    }
  }

  void SpiPurge()
  {
    if (pins_ == SpiPins::kSpi0)
      _bcm_spi_purge();
  }

  void SpiSetCs(const bool how)
  {
    if (pins_ != SpiPins::kSpi0) {
      char t{};
      char r;
      _bcm_aux_spi_transfernb(&t, &r, 1, how);
    } else
      _bsm_spi_cs(how);
  }

  void SpiWaitDone()
  {
    if (pins_ == SpiPins::kSpi0) while (!_bsm_spi_is_done()){}
  }

  void SpiSetSpeed(const std::uint32_t speed_hz)
  {
    if (pins_ == SpiPins::kSpi0)
      bcm2835_spi_set_speed_hz(speed_hz);
    else
      bcm2835_aux_spi_setClockDivider(bcm2835_aux_spi_CalcClockDivider(speed_hz));
  }
};

#endif  // PANDA_TIMESWIPE_SPI_HPP
