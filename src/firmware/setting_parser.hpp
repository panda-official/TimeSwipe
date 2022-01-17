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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SETTING_PARSER_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SETTING_PARSER_HPP

#include "cmd.h"
#include "fifo_stream.hpp"

/**
 * @brief Parser of simple text protocol described in CommunicationProtocol.md.
 *
 * @details All the settings and value are presented in a text format. Both
 * request and response are terminated with `\n` character.
 */
class Setting_parser final : public Serial_event_handler {
public:
  /**
   * @brief The constructor.
   *
   * @param setting_dispatcher A setting dispatcher.
   * @param serial_bus A serial device that is used for communication: provides
   * the ways to send responses and to listen incoming data.
   */
  Setting_parser(const std::shared_ptr<CCmdDispatcher>& setting_dispatcher,
    const std::shared_ptr<CSerial>& serial_bus)
    : serial_bus_{serial_bus}
    , setting_dispatcher_{setting_dispatcher}
  {
    in_fifo_.reserve(1024);
    out_fifo_.reserve(1024);
  }

  /// Termination character used (default is `\n`).
  static constexpr Character term_char{'\n'};

  /// @see Serial_event_handler::handle_receive().
  void handle_receive(const Character ch) override
  {
    if(is_trimming_) {
      if(' '==ch)
        return;

      is_trimming_=false;
    }

    if(term_char==ch) {
      //preparing streams:
      Fifo_stream in{&in_fifo_};
      Fifo_stream out{&out_fifo_};

      try {
        if(Input_state::value!=in_state_)
          throw std::runtime_error{"protocol_error!"};

        //call:
        setting_descriptor_.m_pIn=&in;
        setting_descriptor_.m_pOut=&out;
        setting_descriptor_.m_bThrowExcptOnErr=true;
        setting_dispatcher_->Call(setting_descriptor_);

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
      out_fifo_<<term_char;
      serial_bus_->send(out_fifo_);

      //reset:
      reset();
      return; //!!!!
    }

    switch(in_state_) {
    case Input_state::setting:
      if(' '==ch || '<'==ch || '>'==ch) {
        in_state_ = Input_state::oper;
        is_trimming_=true;
        handle_receive(ch);
        return;
      }
      setting_descriptor_.m_strCommand+=ch;
      return;

    case Input_state::oper:
      if('>'==ch) { //get
        setting_descriptor_.m_ctype=CCmdCallDescr::ctype::ctGet;
        in_state_ = Input_state::value;
        is_trimming_=true;
      } else if('<'==ch) { //set
        setting_descriptor_.m_ctype=CCmdCallDescr::ctype::ctSet;
        in_state_ = Input_state::value;
        is_trimming_=true;
      } else {
        //format error: no function
        //handle_receive(term_char);
        in_state_ = Input_state::error;
      }
      return;

    case Input_state::value:
      in_fifo_<<ch;
      return;

    case Input_state::error:
      return;
    }
  }

private:
  /// Input parsing state.
  enum class Input_state {
    /// Processing a setting name.
    setting,
    /// Processing operator: `<` - *set*, `>` - *get*.
    oper,
    /// Processing a setting value (JSON).
    value,
    /// Protocol error.
    error
  };

  std::shared_ptr<CSerial> serial_bus_;
  std::shared_ptr<CCmdDispatcher> setting_dispatcher_;
  CCmdCallDescr setting_descriptor_;
  CFIFO in_fifo_;
  CFIFO out_fifo_;
  bool is_trimming_{true}; // for automatic spaces skipping
  Input_state in_state_{Input_state::setting};

  /// Resets the state.
  void reset()
  {
    is_trimming_ = true;
    in_state_ = Input_state::setting;
    setting_descriptor_.m_strCommand.clear();
    in_fifo_.reset();
    out_fifo_.reset();
  }
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SETTING_PARSER_HPP
