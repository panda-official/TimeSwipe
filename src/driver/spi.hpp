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

#include <atomic>
#include <cstdint>
#include <iostream>

namespace panda::timeswipe::driver::detail {

class Bcm_spi final : public CSPI {
public:
  enum Spi_pins {
    spi0,
    aux
  };

  ~Bcm_spi()
  {
    if (is_spi_initialized_[Spi_pins::aux])
      bcm2835_aux_spi_end();

    if (is_spi_initialized_[Spi_pins::spi0])
      bcm2835_spi_end();

    if (is_initialized_)
      bcm2835_close();
  }

  Bcm_spi(const Spi_pins pins = Spi_pins::spi0)
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
        (pins_ == Spi_pins::spi0) ? bcm2835_spi_begin() : bcm2835_aux_spi_begin()))
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

  // ---------------------------------------------------------------------------
  // Communication
  // ---------------------------------------------------------------------------

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

  static std::string makeChCmd(const unsigned num, const char* const pSubDomain)
  {
    return std::string{"CH"}.append(std::to_string(num + 1))
      .append(".").append(pSubDomain);
  }

  void sendSetCommand(const std::string& variable, const std::string& value)
  {
    sendCommand(variable + "<" + value + "\n");
  }

  void sendGetCommand(const std::string& variable)
  {
    sendCommand(variable + ">\n");
  }

  bool receiveAnswer(std::string& ans)
  {
    CFIFO answer;
    if (receive(answer)) {
      ans = answer;
      if (trace_spi_)
        std::clog << "spi: received: \"" << ans << "\"" << std::endl;
      return true;
    }
    if (trace_spi_)
      std::cerr << "spi: receive failed" << std::endl;
    return false;
  }

  bool receiveAnswer(std::string& ans, std::string& error)
  {
    const auto ret = receiveAnswer(ans);
    if (ret && !ans.empty() && ans[0] == '!') {
      error = ans;
      ans.clear();
      return false;
    }
    return ret;
  }

  bool receiveStripAnswer(std::string& ans)
  {
    std::string error;
    return receiveStripAnswer(ans, error);
  }

  void sendSetSettingsCommand(const std::string& request)
  {
    sendCommand("js<" + request + "\n");
  }

  void sendGetSettingsCommand(const std::string& request)
  {
    sendCommand("js>" + request + "\n");
  }

  void sendEventsCommand()
  {
    sendCommand("je>\n");
  }

  template <class Number>
  bool sendSetCommandCheck(const std::string& variable, const Number value) {
    sendSetCommand(variable, std::to_string(value));
    // // FIXME: if sleep disable and trace_spi_=false receive() fails sometimes
    // std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    std::string answer;
    receiveStripAnswer(answer);
    Number num{};
    std::istringstream s{answer};
    s >> num;
    return num == value;
  }

  void SetTrace(const bool value)
  {
    trace_spi_ = value;
  }

private:
  inline static bool is_initialized_;
  inline static bool is_spi_initialized_[2];

  std::atomic_bool trace_spi_{};
  CSyncSerComFSM com_cntr_;
  Spi_pins pins_;
  CFIFO rec_fifo_;

  void set_phpol(bool /*bPhase*/, bool /*bPol*/) override
  {}

  void set_baud_div(unsigned char /*div*/) override
  {}

  void set_tprofile_divs(unsigned char /*CSminDel*/, unsigned char /*IntertransDel*/,
    unsigned char /*BeforeClockDel*/) override
  {}

  Character SpiTransfer(const Character ch)
  {
    if (pins_ != Spi_pins::spi0) {
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
    if (pins_ == Spi_pins::spi0)
      _bcm_spi_purge();
  }

  void SpiSetCs(const bool how)
  {
    if (pins_ != Spi_pins::spi0) {
      char t{};
      char r;
      _bcm_aux_spi_transfernb(&t, &r, 1, how);
    } else
      _bsm_spi_cs(how);
  }

  void SpiWaitDone()
  {
    if (pins_ == Spi_pins::spi0) while (!_bsm_spi_is_done()){}
  }

  void SpiSetSpeed(const std::uint32_t speed_hz)
  {
    if (pins_ == Spi_pins::spi0)
      bcm2835_spi_set_speed_hz(speed_hz);
    else
      bcm2835_aux_spi_setClockDivider(bcm2835_aux_spi_CalcClockDivider(speed_hz));
  }

  // ===========================================================================

  void sendCommand(const std::string& cmd)
  {
    CFIFO command;
    command += cmd;
    send(command);
    if (trace_spi_)
      std::clog << "spi: sent: \"" << command << "\"" << std::endl;
  }

  bool receiveStripAnswer(std::string& ans, std::string& error)
  {
    if (!receiveAnswer(ans, error))
      return false;

    // Strip.
    if (!ans.empty() && ans.back() == '\n')
      ans.pop_back();

    return true;
  }
};

} // namespace panda::timeswipe::driver::detail

#endif  // PANDA_TIMESWIPE_SPI_HPP
