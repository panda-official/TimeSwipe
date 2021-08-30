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
    //set_speed(100000);
    set_speed(50000);
  }

  bool is_initialized() const noexcept
  {
    return is_spi_initialized_[pins_];
  }

  CSyncSerComFSM::FSM fsm_state() const noexcept
  {
    return com_cntr_.get_state();
  }

  // ---------------------------------------------------------------------------
  // Communication
  // ---------------------------------------------------------------------------

  bool send(CFIFO& msg) override
  {
    if (!is_initialized())
      return false;

    purge();
    set_cs(true);
    rec_fifo_.reset();

    // A delay CS to fall required.
    bcm2835_delay(20); //corresponds to 50KHz

    // Flow control.
    Character ch=0;
    com_cntr_.start(CSyncSerComFSM::FSM::sendLengthMSB);
    while (com_cntr_.proc(ch, msg))
      transfer(ch);
    if (com_cntr_.bad())
      return false;

    // Provide add clock.
    transfer(0);

    // Wait for a "done" state.
    wait_done();

    com_cntr_.start(CSyncSerComFSM::FSM::recSilenceFrame);
    do
      ch = transfer(0); //provide a clock
    while (com_cntr_.proc(ch, rec_fifo_));

    set_cs(false);

    // A delay for CS to rise required.
    bcm2835_delay(20); // corresponds to 50KHz

    return true;
  }

  bool receive(CFIFO& msg) override
  {
    if (!is_initialized())
      return false;

    msg = rec_fifo_;
    return com_cntr_.get_state() == CSyncSerComFSM::FSM::recOK;
  }

  static std::string make_channel_command(const int num,
    const char* const pSubDomain)
  {
    return std::string{"CH"}.append(std::to_string(num + 1))
      .append(".").append(pSubDomain);
  }

  void send_set_command(const std::string& variable, const std::string& value)
  {
    send_command(variable + "<" + value + "\n");
  }

  void send_get_command(const std::string& variable)
  {
    send_command(variable + ">\n");
  }

  bool receive_answer(std::string& ans)
  {
    CFIFO answer;
    if (receive(answer)) {
      ans = answer;
      if (is_tracing_enabled_)
        std::clog << "spi: received: \"" << ans << "\"" << std::endl;
      return true;
    }
    if (is_tracing_enabled_)
      std::cerr << "spi: receive failed" << std::endl;
    return false;
  }

  bool receive_answer(std::string& ans, std::string& error)
  {
    const auto ret = receive_answer(ans);
    if (ret && !ans.empty() && ans[0] == '!') {
      error = ans;
      ans.clear();
      return false;
    }
    return ret;
  }

  bool receive_strip_answer(std::string& ans)
  {
    std::string error;
    return receive_strip_answer(ans, error);
  }

  void send_set_settings_command(const std::string& request)
  {
    send_command("js<" + request + "\n");
  }

  void send_get_settings_command(const std::string& request)
  {
    send_command("js>" + request + "\n");
  }

  void send_events_command()
  {
    send_command("je>\n");
  }

  template <class Number>
  bool send_set_command_check(const std::string& variable, const Number value) {
    send_set_command(variable, std::to_string(value));
    // // FIXME: if sleep disable and is_tracing_enabled__=false receive() fails sometimes
    // std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    std::string answer;
    receive_strip_answer(answer);
    Number num{};
    std::istringstream s{answer};
    s >> num;
    return num == value;
  }

  void enable_tracing(const bool value)
  {
    is_tracing_enabled_ = value;
  }

  bool is_tracing_enabled() const noexcept
  {
    return is_tracing_enabled_;
  }

private:
  inline static bool is_initialized_;
  inline static bool is_spi_initialized_[2];

  std::atomic_bool is_tracing_enabled_{};
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

  Character transfer(const Character ch)
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

  void purge()
  {
    if (pins_ == Spi_pins::spi0)
      _bcm_spi_purge();
  }

  void set_cs(const bool how)
  {
    if (pins_ != Spi_pins::spi0) {
      char t{};
      char r;
      _bcm_aux_spi_transfernb(&t, &r, 1, how);
    } else
      _bsm_spi_cs(how);
  }

  void wait_done()
  {
    if (pins_ == Spi_pins::spi0) while (!_bsm_spi_is_done()){}
  }

  void set_speed(const std::uint32_t speed_hz)
  {
    if (pins_ == Spi_pins::spi0)
      bcm2835_spi_set_speed_hz(speed_hz);
    else
      bcm2835_aux_spi_setClockDivider(bcm2835_aux_spi_CalcClockDivider(speed_hz));
  }

  // ===========================================================================

  void send_command(const std::string& cmd)
  {
    CFIFO command;
    command += cmd;
    send(command);
    if (is_tracing_enabled_)
      std::clog << "spi: sent: \"" << command << "\"" << std::endl;
  }

  bool receive_strip_answer(std::string& ans, std::string& error)
  {
    if (!receive_answer(ans, error))
      return false;

    // Strip.
    if (!ans.empty() && ans.back() == '\n')
      ans.pop_back();

    return true;
  }
};

} // namespace panda::timeswipe::driver::detail

#endif  // PANDA_TIMESWIPE_SPI_HPP
