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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SETTINGS_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SETTINGS_HPP

#include "error.hpp"
#include "fifo_stream.hpp"
#include "io_stream.hpp"

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

/**
 * @brief Setting access descriptor.
 *
 * @see Setting_parser.
 */
struct CCmdCallDescr final {
  /// The command in a string format.
  std::string m_strCommand;

  /// A hash value of the command string.
  int m_hashCommand;

  /// A zero based index of the command.
  unsigned int m_nCmdIndex{};

  /// Input stream: to fetch function/methode input arguments.
  Io_stream *m_pIn{};

  /// Output stream: to store function/methodes output arguments or return value.
  Io_stream *m_pOut{};

  /**
   * @brief Command handler invocation result ("call result"=cres).
   *
   * @todo FIXME: remove this enum by merging some of it's members into Errc.
   */
  enum cres {
    OK=0,               //!<successful invocation
    generic,            //!<generic error (temporary until merging into Errc)
    obj_not_found,      //!<requested command(object) was not found
    fget_not_supported, //!<"get" property is not supported by a handler
    fset_not_supported, //!<"set" property is not supported by a handler
    parse_err,          //!<an error occurred while parsing arguments from the input stream
    disabled            //!<handler is disabled for some reasons
  };

  /// Invocation type ("call type"=ctype).
  enum ctype {
    /// "get" property
    ctGet = 1,
    /// "set" property
    ctSet = 2
  } m_ctype{ctGet};

  /// How to dispatch an invocation? by a command in a string format, its hash value or index?
  enum cmethod {
    byCmdName=1,        //!<by a command in a string format (using m_strCommand)
    byCmdHash=2,        //!<by a command's hash value  (using m_hashCommand)
    byCmdIndex=4        //!<by a command's zero-based index (using m_nCmdIndex)
  } m_cmethod{byCmdName};

  /// If true, throw `std::runtime_error` instead of returning cres.
  bool m_bThrowExcptOnErr=false;
};

typedef  CCmdCallDescr::cres typeCRes;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/// A basic class for command handler.
class CCmdCallHandler {
public:
  /// The destructor.
  virtual ~CCmdCallHandler() = default;

  /**
   * @brief A method for handling a concrete command.
   *
   * @param d Call descriptor in protocol-independent format.
   */
  virtual typeCRes Call(CCmdCallDescr& d) = 0;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/**
 * @brief The dispatcher of all the setting accesses.
 */
class CCmdDispatcher final {
public:
  /**
   * @brief Adding a new command handler to the dispatching table
   *
   * @param pCmdName Command in a string format.
   * @param pHandler A pointer to the command handler object.
   */
  void Add(const std::string& pCmdName, const std::shared_ptr<CCmdCallHandler>& pHandler)
  {
    table_[pCmdName] = pHandler;
  }

  /**
   * @brief Finds an associated handler by the given descriptor and invokes it
   * if found.
   *
   * @details The result of call is stored to the given descriptor.
   *
   * @param d Call parameters
   * @returns The result of call.
   * @throws `std::runtime_error` on error if
   * `(CCmdCallDescr::m_bThrowExcptOnErr == true)`.
   */
  typeCRes Call(CCmdCallDescr& d)
  {
    const typeCRes cres = __Call(d);
    if (d.m_bThrowExcptOnErr) {
      const char* const what = [cres]() -> const char*
      {
        switch (cres) {
        case typeCRes::OK: break;
        case typeCRes::generic: return "generic!";
        case typeCRes::obj_not_found: return "obj_not_found!";
        case typeCRes::fget_not_supported: return ">_not_supported!";
        case typeCRes::fset_not_supported: return "<_not_supported!";
        case typeCRes::parse_err: return "parse_err!";
        case typeCRes::disabled: return "disabled!";
        }
        return nullptr;
      }();
      if (what) throw std::runtime_error{what};
    }
    return cres;
  }

private:
  std::map<std::string, std::shared_ptr<CCmdCallHandler>> table_;

  typeCRes __Call(CCmdCallDescr& d)
  {
    if (d.m_cmethod == CCmdCallDescr::cmethod::byCmdName) {
      const auto cmd = table_.find(d.m_strCommand);
      return cmd != table_.end() ? cmd->second->Call(d) : typeCRes::obj_not_found;
    } else if (d.m_cmethod == CCmdCallDescr::cmethod::byCmdIndex) {
      if (d.m_nCmdIndex < static_cast<unsigned int>(table_.size())) {
        auto cmd = table_.begin();
        std::advance(cmd, d.m_nCmdIndex);
        d.m_strCommand = cmd->first;
        return cmd->second->Call(d);
      }
    }
    return typeCRes::obj_not_found;
  }
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/**
 * @brief A command dispatcher handler for handling an access point `get` and
 * `set` requests via binding to the methods with a corresponding signature of
 * an arbitrary class.
 *
 * @tparam GetterValue A type of value returned by getter.
 * @tparam SetterValue A type of argument of setter.
 */
template<typename GetterValue, typename SetterValue = GetterValue>
class CCmdSGHandler final : public CCmdCallHandler {
public:
  /// A type of value returned by getter.
  using Getter_value = GetterValue;

  /// A type of argument of setter.
  using Setter_value = SetterValue;

  /// Generic getter.
  using Getter = std::function<Error_or<Getter_value>()>;

  /// Generic setter.
  using Setter = std::function<Error(Setter_value)>;

  /// Basic getter.
  template<typename R>
  using Basic_getter = std::function<R()>;

  /// Basic setter.
  template<typename R>
  using Basic_setter = std::function<R(Setter_value)>;

  /// Member getter.
  template<typename T, typename R>
  using Member_getter = R(T::*)()const;

  /// Member setter.
  template<typename T, typename R>
  using Member_setter = R(T::*)(Setter_value);

  /**
   * @brief The default constructor.
   *
   * @details Constructs an instance which supports neither get nor set.
   */
  CCmdSGHandler() = default;

  /**
   * @brief The constructor.
   *
   * @param get A generic getter.
   * @param set A generic setter.
   */
  template<typename GetterResult, typename SetterResult = void>
  explicit CCmdSGHandler(Basic_getter<GetterResult> get,
    Basic_setter<SetterResult> set = {})
    : get_{std::move(get)}
    , set_{
        [set = std::move(set)](auto value)
        {
          using std::is_same_v;
          static_assert(is_same_v<SetterResult, void> ||
            is_same_v<SetterResult, Error>);
          if constexpr (is_same_v<SetterResult, void>) {
            set(std::forward<decltype(value)>(value));
            return Error{};
          } else
            return set(std::forward<decltype(value)>(value));
        }
      }
  {}

  /// @overload
  template<typename GetterResult, typename SetterResult = void>
  explicit CCmdSGHandler(GetterResult(*get)(), SetterResult(*set)(Setter_value) = {})
    : CCmdSGHandler{Basic_getter<GetterResult>{get}, Basic_setter<SetterResult>{set}}
  {}

  /**
   * @brief The constructor.
   *
   * @param instance An instance.
   * @param get A pointer to the class getter.
   * @param set A pointer to the class setter.
   */
  template<class T, class GetterClass, class GetterResult,
    class SetterClass = GetterClass, class SetterResult = void>
  CCmdSGHandler(const std::shared_ptr<T>& instance,
    Member_getter<GetterClass, GetterResult> get,
    Member_setter<SetterClass, SetterResult> set = {})
    : CCmdSGHandler{
        // get_
        instance && get ?
        [instance, get]
        {
          return (instance.get()->*get)();
        } : Getter{},
        // set_
        instance && set ?
        [instance, set](Setter_value v)
        {
          using std::is_same_v;
          static_assert(is_same_v<SetterResult, void> ||
            is_same_v<SetterResult, Error>);
          if constexpr (is_same_v<SetterResult, void>) {
            (instance.get()->*set)(std::move(v));
            return Error{};
          } else
            return (instance.get()->*set)(std::move(v));
        } : Setter{}
      }
  {}

  /**
   * @brief A method for handling a concrete command.
   *
   * @param d Call descriptor in protocol-independent format.
   *
   * @todo: FIXME: return Errc
   */
  typeCRes Call(CCmdCallDescr& d) override
  {
    if (d.m_ctype & CCmdCallDescr::ctype::ctSet) {
      if (set_) {
        Setter_value val{};
        *d.m_pIn >> val;
        if (d.m_pIn->is_good()) {
          if (const auto err = set_(val); !err) {
            if (get_) {
              if (const auto [err, res] = get_(); err)
                return typeCRes::generic; // FIXME: return err
              else
                *d.m_pOut << res; // Done.
            }
          } else
            return typeCRes::generic;
        } else
          return typeCRes::parse_err;
      } else
        return typeCRes::fset_not_supported;
    }
    if (d.m_ctype & CCmdCallDescr::ctype::ctGet) {
      if (get_) {
        if (const auto [err, res] = get_(); err)
          return typeCRes::generic; // FIXME: return err
        else
          *d.m_pOut << res; // Done.
      } else
        return typeCRes::fget_not_supported;
    }
    return typeCRes::OK;
  }

private:
  Getter get_{};
  Setter set_{};
};

// -----------------------------------------------------------------------------
// Setting_parser
// -----------------------------------------------------------------------------

/**
 * @brief Parser of simple text protocol described in CommunicationProtocol.md.
 *
 * @details This class is responsible to parse the setting access requests to
 * the instances of class CCmdCallDescr and to pass them to an instance of
 * CCmdDispatcher.
 * All the settings and their's values are represented in text format. Each
 * request and response are always terminated with the `\n` character.
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
    if (is_trimming_) {
      if (ch == ' ')
        return;

      is_trimming_ = false;
    }

    if (ch == term_char) {
      Fifo_stream in{&in_fifo_};
      Fifo_stream out{&out_fifo_};

      try {
        if (in_state_ != Input_state::value)
          throw std::runtime_error{"protocol_error!"};

        // Invoke setting handler.
        setting_descriptor_.m_pIn = &in;
        setting_descriptor_.m_pOut = &out;
        setting_descriptor_.m_bThrowExcptOnErr = true;
        setting_dispatcher_->Call(setting_descriptor_);
      } catch(const std::exception& ex) {
        out << "!" << ex.what();
      }

      out_fifo_ << term_char;
      serial_bus_->send(out_fifo_);

      reset();
      return; // done
    }

    switch (in_state_) {
    case Input_state::setting:
      if (ch == ' ' || ch == '<' || ch == '>') {
        in_state_ = Input_state::oper;
        is_trimming_ = true;
        handle_receive(ch);
      } else
        setting_descriptor_.m_strCommand += ch;
      break;
    case Input_state::oper:
      if (ch == '>') {
        setting_descriptor_.m_ctype = CCmdCallDescr::ctype::ctGet;
        in_state_ = Input_state::value;
        is_trimming_ = true;
      } else if (ch == '<') {
        setting_descriptor_.m_ctype = CCmdCallDescr::ctype::ctSet;
        in_state_ = Input_state::value;
        is_trimming_ = true;
      } else
        in_state_ = Input_state::error;
      break;
    case Input_state::value:
      in_fifo_ << ch;
      break;
    case Input_state::error:
      break;
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

#endif  // PANDA_TIMESWIPE_FIRMWARE_SETTINGS_HPP
