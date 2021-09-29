// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH / Dmitry Igrishin

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

#ifndef PANDA_TIMESWIPE_BCMSPI_HPP
#define PANDA_TIMESWIPE_BCMSPI_HPP

#include "error_detail.hpp"
#include "spi.hpp"
#include "synccom.hpp"
#include "3rdparty/BCMsrc/bcm2835.h"

#include <atomic>
#include <cstdint>
#ifdef PANDA_TIMESWIPE_TRACE_SPI
#include <iostream>
#endif
#include <thread>

namespace panda::timeswipe::detail {

/// Implementation of SPI for BCM.
class Bcm_spi final : public CSPI {
public:
  /// BCM pins.
  enum Pins {
    spi0,
    aux
  };

  /// The destructor.
  ~Bcm_spi()
  {
    if (is_spi_initialized_[Pins::aux])
      bcm2835_aux_spi_end();

    if (is_spi_initialized_[Pins::spi0])
      bcm2835_spi_end();

    if (is_initialized_)
      bcm2835_close();
  }

  /// The constructor.
  explicit Bcm_spi(const Pins pins = Pins::spi0)
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
        (pins_ == Pins::spi0) ? bcm2835_spi_begin() : bcm2835_aux_spi_begin()))
      return;

    // Set default rate
    set_speed(50000);
  }

  /// @returns `true` if SPI initialized for the specific pins.
  bool is_initialized() const noexcept
  {
    return is_spi_initialized_[pins_];
  }

  /// @returns The state of FSM.
  CSyncSerComFSM::State fsm_state() const noexcept
  {
    return com_cntr_.state();
  }

  // ---------------------------------------------------------------------------
  // Requests execution (high-level API)
  // ---------------------------------------------------------------------------

  /**
   * Executes the SPI `request`.
   *
   * @throws An instance of Exception with either `Errc::spi_send` or
   * `Errc::spi_receive`.
   */
  std::string execute(const std::string& request)
  {
    send_throw(request);
    return receive_throw();
  }

  /**
   * Executes the SPI set request.
   *
   * @throws See execute().
   */
  std::string execute_set_one(const std::string& name, const std::string& value)
  {
    return execute(name + "<" + value + "\n");
  }

  /**
   * Executes the SPI get request.
   *
   * @throws See execute().
   */
  std::string execute_get_one(const std::string& name)
  {
    return execute(name + ">\n");
  }

  /**
   * Executes the SPI "set many" request.
   *
   * @throws See execute().
   */
  std::string execute_set_many(const std::string& json_object)
  {
    return execute("js<" + json_object + "\n");
  }

  /**
   * Executes the SPI "set many" request.
   *
   * @throws See execute().
   */
  std::string execute_get_many(const std::string& json_object)
  {
    return execute("js>" + json_object + "\n");
  }

  // ---------------------------------------------------------------------------
  // CSPI overridings
  // ---------------------------------------------------------------------------

  /// See CSPI::send().
  bool send(CFIFO& msg) override
  {
    if (!is_initialized())
      return false;

    purge();
    set_transfer_active(true);
    rec_fifo_.reset();

    // A delay CS to fall required.
    bcm2835_delay(20); // corresponds to 50KHz

    // Flow control.
    {
      Character ch{};
      com_cntr_.start(CSyncSerComFSM::State::sendLengthMSB);
      while (com_cntr_.proc(ch, msg))
        transfer(ch);
      if (com_cntr_.bad())
        return false;
    }

    // Provide add clock.
    transfer(0);

    // Wait for a "done" state.
    wait_done();

    {
      Character ch{};
      com_cntr_.start(CSyncSerComFSM::State::recSilenceFrame);
      do
        ch = transfer(0); // provide a clock
      while (com_cntr_.proc(ch, rec_fifo_));
    }

    set_transfer_active(false);

    // A delay for CS to rise required.
    bcm2835_delay(20); // corresponds to 50KHz

    return true;
  }

  /// See CSPI::receive().
  bool receive(CFIFO& msg) override
  {
    if (!is_initialized())
      return false;

    msg = rec_fifo_;
    return com_cntr_.state() == CSyncSerComFSM::State::recOK;
  }

  /**
   * Sends the SPI "set one" request.
   *
   * @throws See send_throw().
   */
  void send_set_one(const std::string& name, const std::string& value)
  {
    send_throw(name + "<" + value + "\n");
  }

  /**
   * Sends the SPI "get one" request.
   *
   * @throws See send_throw().
   */
  void send_get_one(const std::string& name)
  {
    send_throw(name + ">\n");
  }

  /**
   * Sends the SPI "set many" request.
   *
   * @throws See send_throw().
   */
  void send_set_many(const std::string& json_object)
  {
    send_throw("js<" + json_object + "\n");
  }

  /**
   * Sends the SPI "get many" request.
   *
   * @throws See send_throw().
   */
  void send_get_many(const std::string& json_array)
  {
    send_throw("js>" + json_array + "\n");
  }

  /**
   * Sends the SPI `request`.
   *
   * @throws An instance of Exception with either `Errc::spi_send`.
   */
  void send_throw(const std::string& request)
  {
    CFIFO fifo;
    fifo += request;
    const auto res = send(fifo);
#ifdef PANDA_TIMESWIPE_TRACE_SPI
    std::clog << "spi: sent: \"" << request << "\"" << std::endl;
#endif
    if (!res)
      throw Generic_exception{Errc::spi_send,
        std::string{"cannot send SPI request "}.append(request)};
  }

  /**
   * Receives the SPI response.
   *
   * @throws An instance of Exception with either `Errc::spi_receive`.
   */
  std::string receive_throw()
  {
    std::string result;
    CFIFO fifo;
    if (receive(fifo)) {
      result = fifo;
      if (!result.empty() && result[0] == '!') {
        // Error returned, so throw exception.
        const auto errc = [&result]
        {
          if (result == "!protocol_error!")
            return Errc::com_proto_request_invalid;
          else if (result == "!Line_err!")
            return Errc::com_proto_bus;
          else if (result == "!Timeout_err!")
            return Errc::com_proto_timeout;
          else if (result == "!obj_not_found!")
            return Errc::com_proto_object_not_found;
          else if (result == "!>_not_supported!")
            return Errc::com_proto_get_unsupported;
          else if (result == "!<_not_supported!")
            return Errc::com_proto_set_unsupported;
          else if (result == "!disabled!")
            return Errc::com_proto_access_point_disabled;
          else
            return Errc::com_proto;
        }();
        throw Generic_exception{errc, "communication protocol error"};
      }

      // Strip result.
      if (!result.empty() && result.back() == '\n')
        result.pop_back();

#ifdef PANDA_TIMESWIPE_TRACE_SPI
      std::clog << "spi: received: \"" << result << "\"" << std::endl;
#endif
      return result;
    }
#ifdef PANDA_TIMESWIPE_TRACE_SPI
    std::clog << "spi: receive error" << std::endl;
#endif
    throw Generic_exception{Errc::spi_receive, "cannot receive SPI response"};
  }

private:
  inline static bool is_initialized_;
  inline static bool is_spi_initialized_[2];

  CSyncSerComFSM com_cntr_;
  Pins pins_;
  CFIFO rec_fifo_;

  void set_phpol(bool /*bPhase*/, bool /*bPol*/) override
  {}

  void set_baud_div(unsigned char /*div*/) override
  {}

  void set_tprofile_divs(unsigned char /*CSminDel*/, unsigned char /*IntertransDel*/,
    unsigned char /*BeforeClockDel*/) override
  {}

  using CSPI::transfer;

  Character transfer(const Character ch)
  {
    if (pins_ != Pins::spi0) {
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
    if (pins_ == Pins::spi0)
      _bcm_spi_purge();
  }

  void set_transfer_active(const bool how)
  {
    if (pins_ != Pins::spi0) {
      char t{};
      char r;
      _bcm_aux_spi_transfernb(&t, &r, 1, how);
    } else
      _bsm_spi_cs(how);
  }

  void wait_done()
  {
    if (pins_ == Pins::spi0) while (!_bsm_spi_is_done());
  }

  void set_speed(const std::uint32_t speed_hz)
  {
    if (pins_ == Pins::spi0)
      bcm2835_spi_set_speed_hz(speed_hz);
    else
      bcm2835_aux_spi_setClockDivider(bcm2835_aux_spi_CalcClockDivider(speed_hz));
  }
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_BCMSPI_HPP
