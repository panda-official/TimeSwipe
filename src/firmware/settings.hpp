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

#include "../serial.hpp"
#include "error.hpp"
#include "json.hpp"
using namespace panda::timeswipe; // FIXME: REMOVEME

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// -----------------------------------------------------------------------------
// Setting_descriptor
// -----------------------------------------------------------------------------

/// Setting request type.
enum class Setting_access_type {
  /// Read access.
  read,
  /// Write access.
  write
};

/**
 * @brief Setting request descriptor.
 *
 * @remarks If both `name` and `index` are set the `name` is higher priority
 * than `index`.
 *
 * @see Setting_parser.
 */
struct Setting_descriptor final {
  /// The setting name. (Higher priority than `index`.)
  std::string name;

  /// A zero based index of the setting. (Lower priority than `name`.)
  int index{-1};

  /// Input value view. (For both read and write requests.)
  Json_value_view in_value;

  /// Output value view. (For either read or write requests.)
  Json_value_view out_value;

  /// Access type.
  Setting_access_type access_type{Setting_access_type::read};

  void reset()
  {
    name.clear();
    index = -1;
    in_value = {};
    out_value = {};
    access_type = Setting_access_type::read;
  }
};

// -----------------------------------------------------------------------------
// Setting_handler
// -----------------------------------------------------------------------------

/// A setting access handler.
class Setting_handler {
public:
  /// The destructor.
  virtual ~Setting_handler() = default;

  /**
   * @brief Handles the setting access request.
   *
   * @param[in,out] descriptor The descriptor with the input value and to hold
   * the result value of the successful operation.
   */
  virtual Error handle(Setting_descriptor& descriptor) = 0;
};

// -----------------------------------------------------------------------------
// Setting_dispatcher
// -----------------------------------------------------------------------------

/// The dispatcher of all the setting accesses.
class Setting_dispatcher final {
public:
  /// Registers a new handler.
  void add(const std::string& name, const std::shared_ptr<Setting_handler>& handler)
  {
    table_[name] = handler;
  }

  /**
   * @brief Finds an associated handler by the given descriptor and invokes it
   * if found.
   *
   * @details The result of call is stored into the given descriptor.
   *
   * @param[in,out] descriptor The descriptor to find an associated handler and
   * to hold the result value of the successful operation.
   */
  Error handle(Setting_descriptor& descriptor)
  {
    auto handler = table_.end();
    if (!descriptor.name.empty()) {
      handler = table_.find(descriptor.name);
    } else if (descriptor.index >= 0) {
      if (static_cast<unsigned>(descriptor.index) < table_.size()) {
        handler = table_.begin();
        std::advance(handler, descriptor.index);
        descriptor.name = handler->first;
      }
    }
    if (handler != table_.end())
      return handler->second->handle(descriptor);
    else
      return Errc::board_settings_unknown;
  }

private:
  std::map<std::string, std::shared_ptr<Setting_handler>> table_;
};

// -----------------------------------------------------------------------------
// Setting_generic_handler
// -----------------------------------------------------------------------------

/**
 * @brief A setting generic handler for handling either read or write requests.
 *
 * @tparam GetterValue A type of value returned by getter.
 * @tparam SetterValue A type of argument of setter.
 */
template<typename GetterValue, typename SetterValue = GetterValue>
class Setting_generic_handler final : public Setting_handler {
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
  Setting_generic_handler() = default;

  /**
   * @brief The constructor.
   *
   * @param get A generic getter.
   * @param set A generic setter.
   */
  template<typename GetterResult, typename SetterResult = void>
  explicit Setting_generic_handler(Basic_getter<GetterResult> get,
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
  explicit Setting_generic_handler(GetterResult(*get)(),
    SetterResult(*set)(Setter_value) = {})
    : Setting_generic_handler{Basic_getter<GetterResult>{get},
      Basic_setter<SetterResult>{set}}
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
  Setting_generic_handler(const std::shared_ptr<T>& instance,
    Member_getter<GetterClass, GetterResult> get,
    Member_setter<SetterClass, SetterResult> set = {})
    : Setting_generic_handler{
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
   * @brief Handles a request.
   *
   * @param descriptor Call descriptor in protocol-independent format.
   */
  Error handle(Setting_descriptor& descriptor) override
  {
    if (descriptor.access_type == Setting_access_type::write) {
      if (set_) {
        Setter_value val{};
        const auto err = get(descriptor.in_value, val);
        if (!err) {
          if (const auto err = set_(val); !err) {
            if (get_) { // read back
              if (const auto [err, res] = get_(); err)
                return err;
              else
                return set(descriptor.out_value, res); // Done.
            }
          } else
            return err;
        } else
          return Error{Errc::board_settings_invalid, err.what()};
      } else
        return Errc::board_settings_write_forbidden;
    } else if (descriptor.access_type == Setting_access_type::read) {
      if (get_) {
        if (const auto [err, res] = get_(); err)
          return err;
        else
          return set(descriptor.out_value, res); // Done.
      } else
        return Errc::board_settings_read_forbidden;
    }
    return Errc::bug;
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
 * the instances of class Setting_descriptor and to pass them to an instance of
 * Setting_dispatcher.
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
  Setting_parser(const std::shared_ptr<Setting_dispatcher>& setting_dispatcher,
    const std::shared_ptr<CSerial>& serial_bus)
    : serial_bus_{serial_bus}
    , setting_dispatcher_{setting_dispatcher}
  {
    input_value_string_.reserve(1024);
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
      // Always respond in JSON.
      rapidjson::Document response{rapidjson::kObjectType};

      // Invoke setting handler.
      if (in_state_ == Input_state::value) {
        rapidjson::Document in_value;
        const rapidjson::ParseResult pr{in_value.Parse(input_value_string_.data(),
            input_value_string_.size())};
        if (pr) {
          const Json_value_view in_view{&in_value};
          rapidjson::Value out_value;
          auto& alloc = response.GetAllocator();
          Json_value_view out_view{&out_value, &alloc};
          setting_descriptor_.in_value = in_view;
          setting_descriptor_.out_value = out_view;
          if (const auto err = setting_dispatcher_->handle(setting_descriptor_))
            set_error(response, response, err);
          else
            set_result(response, response, std::move(out_value));
        } else
          set_error(response, response, Errc::board_settings_invalid);
      } else
        set_error(response, response, Errc::board_settings_invalid);

      // Respond and return.
      CFIFO output{to_text(response)};
      output << term_char;
      serial_bus_->send(output);
      reset();
      return;
    }

    switch (in_state_) {
    case Input_state::setting:
      if (ch == ' ' || ch == '<' || ch == '>') {
        in_state_ = Input_state::oper;
        is_trimming_ = true;
        handle_receive(ch);
      } else
        setting_descriptor_.name += ch;
      break;
    case Input_state::oper:
      if (ch == '>') {
        setting_descriptor_.access_type = Setting_access_type::read;
        in_state_ = Input_state::value;
        is_trimming_ = true;
      } else if (ch == '<') {
        setting_descriptor_.access_type = Setting_access_type::write;
        in_state_ = Input_state::value;
        is_trimming_ = true;
      } else
        in_state_ = Input_state::error;
      break;
    case Input_state::value:
      input_value_string_ += ch;
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
  std::shared_ptr<Setting_dispatcher> setting_dispatcher_;
  Setting_descriptor setting_descriptor_;
  std::string input_value_string_;
  bool is_trimming_{true}; // for automatic spaces skipping
  Input_state in_state_{Input_state::setting};

  /// Resets the state.
  void reset()
  {
    is_trimming_ = true;
    in_state_ = Input_state::setting;
    setting_descriptor_.reset();
    input_value_string_.clear();
  }
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SETTINGS_HPP
