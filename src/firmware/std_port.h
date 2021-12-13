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
 * A basic Port that implements simple text protocol described in
 * CommunicationProtocol.md.
 */

#ifndef PANDA_TIMESWIPE_FIRMWARE_STD_PORT_HPP
#define PANDA_TIMESWIPE_FIRMWARE_STD_PORT_HPP

#include "cmd.h"
#include "fifo_stream.hpp"

/**
 * @brief An implementation of simple text protocol described in
 * CommunicationProtocol.md.
 *
 * All the commands and data are presented as text in a human-readable format.
 * The message must be ended with a termination character (`\n` by default).
 */
class CStdPort : public ISerialEvent {
protected:
  /// A pointer to a  serial device used for communication
  std::shared_ptr<CSerial> m_pBus;

  /// A pointer to a command dispatcher object.
  std::shared_ptr<CCmdDispatcher> m_pDisp;

  /// An information about command call and its parameters in protocol-independent form.
  CCmdCallDescr m_CallDescr;

  /// A FIFO buffer to receive incoming request message.
  CFIFO m_In;

  /// A FIFO buffer to form an output message.
  CFIFO m_Out;

  /// Shall we automatically remove spaces from the input stream? This variable controlled automatically.
  bool m_bTrimming=true;

  /// A Finite State Machine (FSM) used to parse incoming stream.
  enum FSM {
    proc_cmd,           //!<processing a command
    proc_function,      //!<waiting for a function type character: '<'="set", '>'="get"
    proc_args,          //!<processing command arguments
    err_protocol        //!<an error happened during processing an incoming request
  };

  /// Holds a current state of the parser FSM.
  FSM m_PState=FSM::proc_cmd;

  /**
   * @brief Main parser function, called from on_rec_char
   *
   * @param ch Incoming character.
   */
  void parser(Character ch)
  {
    if(m_bTrimming) {
      if(' '==ch)
        return;

      m_bTrimming=false;
    }

    if(TERM_CHAR==ch) {
      //preparing streams:
      Fifo_stream in{&m_In};
      Fifo_stream out{&m_Out};

      try {
        if(proc_args!=m_PState)
          throw CCmdException("protocol_error!");

        //call:
        m_CallDescr.m_pIn=&in;
        m_CallDescr.m_pOut=&out;
        m_CallDescr.m_bThrowExcptOnErr=true;
        m_pDisp->Call(m_CallDescr);

      } catch(const std::exception& ex) {
        out<<"!"<<ex.what();
      }

      //send data:
      /*if(typeCRes::OK!=cres) switch(cres){

        case typeCRes::parse_err:           out<<"!parse_err!";       break;
        case typeCRes::obj_not_found:       out<<"!obj_not_found!";   break;
        case typeCRes::fget_not_supported:  out<<"!>_not_supported!";  break;
        case typeCRes::fset_not_supported:  out<<"!<_not_supported!";  break;
        }*/
      m_Out<<TERM_CHAR;
      m_pBus->send(m_Out);

      //reset:
      reset();
      return; //!!!!
    }

    switch(m_PState) {
    case proc_cmd:
      if(' '==ch || '<'==ch || '>'==ch) {
        m_PState=proc_function;
        m_bTrimming=true;
        parser(ch);
        return;
      }
      m_CallDescr.m_strCommand+=ch;
      return;

    case proc_function:
      if('>'==ch) { //get
        m_CallDescr.m_ctype=CCmdCallDescr::ctype::ctGet;
        m_PState=proc_args;
        m_bTrimming=true;
      } else if('<'==ch) { //set
        m_CallDescr.m_ctype=CCmdCallDescr::ctype::ctSet;
        m_PState=proc_args;
        m_bTrimming=true;
      } else {
        //format error: no function
        //parser(TERM_CHAR);
        m_PState=err_protocol;
      }
      return;

    case proc_args:
      m_In<<ch;
      return;

    case err_protocol:
      return;
    }
  }

  /// Resets the port: buffers, FSM and m_CallDescr.
  void reset()
  {
    m_bTrimming=true;
    m_PState=FSM::proc_cmd;
    m_CallDescr.m_strCommand.clear();
    m_In.reset();
    m_Out.reset();
  }

public:
  /// Termination character used (default is `\n`).
  static const int TERM_CHAR='\n';

  /**
   * @brief Callback that is called when a new character has been received in
   * a FIFO buffer of a serial device.
   *
   * @param ch Character received.
   */
  void on_rec_char(Character ch) override
  {
    parser(ch);
  }

public:
  /**
   * @brief The constructor.
   *
   * @param pDisp A pointer to a command dispatcher.
   * @param pBus A pointer to a serial device that provides ISerial for sending
   * response messages and ISerialEvent callback interface for listening incoming
   * characters.
   */
  CStdPort(const std::shared_ptr<CCmdDispatcher> &pDisp, const std::shared_ptr<CSerial> &pBus)
  {
    m_pDisp=pDisp;
    m_pBus=pBus;

    m_In.reserve(1024);
    m_Out.reserve(1024);
  }
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_STD_PORT_HPP
