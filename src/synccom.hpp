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

/**
 * @file
 * A software implementation of flow-control for SPI bus
 */

#ifndef PANDA_TIMESWIPE_SYNCCOM_HPP
#define PANDA_TIMESWIPE_SYNCCOM_HPP

#include "serial.hpp"

/**
 * @brief A software implementation of flow-control for SPI bus.
 *
 * Since the clock frequency is provided by a SPI master device to push bytes
 * through the bus, at least one problem occurs: how many clocks are required to
 * fetch responce message from the slave if the message has variable lenght? This
 * problem is solved by using simple software flow-control: each transaction is
 * started with a "silence frame" (a sequence of zeros) that is used as marker of
 * transfer begin. Then a number of bytes that should be received is transmitted
 * in two bytes in MSB-LSB order. Where MSB is marked with 0x80 flag. This works
 * symmetric both for master and slave: first, the master sends a silence frame
 * followed by the length of the message, so the slave knows the number of bytes
 * to receive, then the master fetches the same message header from the slave and
 * knows how many clocks should be provided to fetch the whole message from the
 * slave.
 */
class CSyncSerComFSM {
public:
  /// A state to control the communication flow.
  enum class State {
    /// Inactive state, no operation performed.
    halted,

    /// @name Sending
    /// @{

    /// Send a silence frame (a sequence of zeros) to a destination device.
    sendSilenceFrame,

    /// Send a most significant byte of a message length to a destination device.
    sendLengthMSB,

    /// Send a least significant byte of a message length to a destination device.
    sendLengthLSB,

    /// Send message of given length to a destination device.
    sendBody,

    /// Sending operation was successfully finished.
    sendOK,
    /// @}

    /// @name Receiving
    /// @{

    /// Receive a silence frame (a sequence of zeros) from a destination device.
    recSilenceFrame,

    /// Receive a most significant byte of a message length from a destination device.
    recLengthMSB,

    /// Receive a least significant byte of a message length from a destination device.
    recLengthLSB,

    /// Receive message of given length from a destination device.
    recBody,

    /// Receiving operation was successfully finished.
    recOK,

    /// @}

    /// @name Errors
    /// @{

    /**
     * A silence frame was disrupted (the byte other than zero was received
     * during the silence frame).
     */
    errLine,

    /// A message length bytes were not received after the silence frame.
    errTimeout

    /// @}
  };

protected:
  /// Holding the current FSM state.
  State m_PState=State::halted;

  /// Current bytes count in a silence frame.
  int m_FrameCnt=0;

  /// Obtained message length
  unsigned m_TargetLength=0;

public:
  /**
   * @brief Turn FSM to sending (sendSilenceFrame) or receiving
   * (recSilenceFrame) mode or stop it by setting halted state.
   *
   * @param nState A state to set
   */
  void start(State nState)
  {
    m_FrameCnt=0;
    m_PState=nState;
  }

  /// @returns true if an error happened during transaction.
  bool bad()
  {
    return m_PState >= State::errLine;
  }

  /// @returns Actual FSM state.
  State state() const noexcept
  {
    return m_PState;
  }

  /**
   * @brief Force execution of SPI flow-control.
   *
   * @param[inout] ch In case of sending message operation: a character to send
   * to SPI bus generated on flow-control logic and message to send (msg). In
   * case of receiving message operation: a received character from SPI bus
   * that is processed according flow-control logic and received message will
   * be saved to `msg`.
   * @param[out] msg A buffer for send/receive message.
   *
   * @returns FIXME.
   */
  template <typename FIFO>
  bool proc(Character& ch, FIFO& msg)
  {
    switch(m_PState) {
      //sending:
    case State::sendSilenceFrame:
      ch=0;
      if(m_FrameCnt++>3) {
        m_FrameCnt=0;
        m_PState=State::sendLengthMSB;
      }
      return true;
    case State::sendLengthMSB:
      ch=(msg.in_avail()>>8)|0x80;
      m_PState=State::sendLengthLSB;
      return true;
    case State::sendLengthLSB:
      ch=(msg.in_avail())&0xff;
      m_PState=State::sendBody;
      return true;
    case State::sendBody:
      if(0==msg.in_avail())
        {
          m_PState=State::sendOK;
          return false;
        }
      msg>>ch;
      return true;

      //receiving:
    case State::recSilenceFrame:
      //collision check:
      if(0!=ch) {
        //line error:
        m_PState=State::errLine;
        return false;
      }
      if(m_FrameCnt++>3) {
        m_FrameCnt=0;
        m_PState=State::recLengthMSB;
      }
      return true;
    case State::recLengthMSB:
      if(0!=ch)
        {
          m_TargetLength=((unsigned)(ch&0x7f))<<8;
          m_PState = State::recLengthLSB;
          return true;
        }
      if(m_FrameCnt++>100000)
        {
          m_PState=State::errTimeout;
          return false;
        }
      return true;
    case State::recLengthLSB:
      m_TargetLength|=ch;
      m_PState=State::recBody;
      return true;
    case State::recBody:
      msg<<ch;
      if(msg.in_avail()>=m_TargetLength) {
        m_PState=State::recOK;
        return false;
      }
      return true;

    case State::halted:
    case State::sendOK:
    case State::recOK:
    case State::errLine:
    case State::errTimeout:
      break;
    }
    return false;
  }
};

#endif  // PANDA_TIMESWIPE_SYNCCOM_HPP
