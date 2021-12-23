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

#ifndef PANDA_TIMESWIPE_FIRMWARE_CMD_HPP
#define PANDA_TIMESWIPE_FIRMWARE_CMD_HPP

#include "error.hpp"
#include "io_stream.hpp"

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

/**
 * @brief An uniform command request descriptor.
 *
 * The processing of all incoming commands is handled by an instance of class
 * CCmdDispatcher. The implementation of the current communication protocol
 * (simple text, binary/specific) transforms an incoming request from a protocol
 * depended form to an uniform request described by the class CCmdCallDescr,
 * where command name in a string format (if presents) denotes hash value,
 * pointers to input/output streams and other service information stored.
 * Implementation then calls method CCmdDispatcher::Call with an object of type
 * CCmdCallDescr as a parameter. CCmdDispatcher::Call handles it by finding
 * specific command handler in its internal map and invokes that handler. The
 * result of call is stored to the passed parameter and hence its further
 * accesible by a port. By adding number of different port classes communication
 * protocols can be implemented at once.
 */
struct CCmdCallDescr final {
  /// The command in a string format.
  std::string m_strCommand;

  /// A hash value of the command string.
  int m_hashCommand;

  /// A zero based index of the command.
  unsigned int m_nCmdIndex;

  /// Input stream: to fetch function/methode input arguments.
  Io_stream    *m_pIn=nullptr;

  /// Output stream: to store function/methodes output arguments or return value.
  Io_stream    *m_pOut=nullptr;

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

/// A command dispatcher class.
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
   * @brief Finds a corresponding command handler by given call parameters
   * and calls it.
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
  using Getter = std::function<Getter_value()>;

  /// Generic setter.
  using Setter = std::function<Error(Setter_value)>;

  /// Basic setter.
  template<typename R>
  using Basic_setter = std::function<R(Setter_value)>;

  /// Member getter.
  template<typename T>
  using Member_getter = Getter_value(T::*)()const;

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
  template<typename R = void>
  explicit CCmdSGHandler(Getter get, Basic_setter<R> set = {})
    : get_{std::move(get)}
    , set_{
        [set = std::move(set)](auto value)
        {
          if constexpr (std::is_same_v<R, void>) {
            set(std::forward<decltype(value)>(value));
            return Error{};
          } else
            return set(std::forward<decltype(value)>(value));
        }
      }
  {}

  /// @overload
  template<typename R = void>
  explicit CCmdSGHandler(Getter_value(*get)(), R(*set)(Setter_value) = {})
    : CCmdSGHandler{Getter{get}, Basic_setter<R>{set}}
  {}

  /**
   * @brief The constructor.
   *
   * @param instance An instance.
   * @param get A pointer to the class getter.
   * @param set A pointer to the class setter.
   */
  template<class T, class GetterClass, class SetterClass = GetterClass, class R = void>
  CCmdSGHandler(const std::shared_ptr<T>& instance,
    Member_getter<GetterClass> get, Member_setter<SetterClass, R> set = {})
    : CCmdSGHandler{
        // get_
        instance && get ?
        [instance, get]{return (instance.get()->*get)();} : Getter{},
        // set_
        instance && set ?
        [instance, set](Setter_value v)
        {
          if constexpr (std::is_same_v<R, void>) {
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
   */
  typeCRes Call(CCmdCallDescr& d) override
  {
    if (d.m_ctype & CCmdCallDescr::ctype::ctSet) {
      if (set_) {
        Setter_value val{};
        *d.m_pIn >> val;
        if (d.m_pIn->is_good()) {
          if (set_(val)) {
            if (get_)
              *d.m_pOut << get_(); // Done.
          } else
            return typeCRes::generic;
        } else
          return typeCRes::parse_err;
      } else
        return typeCRes::fset_not_supported;
    }
    if (d.m_ctype & CCmdCallDescr::ctype::ctGet) {
      if (get_)
        *d.m_pOut << get_(); // Done.
      else
        return typeCRes::fget_not_supported;
    }
    return typeCRes::OK;
  }

private:
  Getter get_{};
  Setter set_{};
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_CMD_HPP
