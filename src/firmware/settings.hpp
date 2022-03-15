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

#include "../debug.hpp"
#include "../serial.hpp"
#include "error.hpp"
#include "json.hpp"
using namespace panda::timeswipe; // FIXME: REMOVEME

#include <cctype>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// -----------------------------------------------------------------------------
// Setting_request
// -----------------------------------------------------------------------------

/// Setting request type.
enum class Setting_request_type {
  /// Read access.
  read,
  /// Write access.
  write
};

/**
 * @brief Setting request.
 *
 * @see Setting_parser.
 */
struct Setting_request final {
  /// The setting name.
  const std::string& name;

  /// Access type.
  const Setting_request_type type{Setting_request_type::read};

  /// Input value view.
  const Json_value_view input;

  /// Output value view.
  Json_value_view output;
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
   * @brief Handles the setting request.
   *
   * @param[in,out] request The request.
   */
  virtual Error handle(Setting_request& request) = 0;
};

// -----------------------------------------------------------------------------
// Setting_dispatcher
// -----------------------------------------------------------------------------

/// The setting requests dispatcher.
class Setting_dispatcher final {
public:
  /// @returns `true` if `name` is a special setting name.
  static constexpr bool is_name_special(const std::string_view name) noexcept
  {
    return name == "all" || name == "basic";
  }

  /**
   * @brief Registers a new request handler.
   *
   * @par Requires
   * `!is_name_reserved(name) && handler`.
   */
  void add(const std::string& name, const std::shared_ptr<Setting_handler>& handler)
  {
    PANDA_TIMESWIPE_ASSERT(!is_name_special(name) && handler);
    table_[name] = handler;
  }

  /**
   * @brief Searches an associated handler by the given request and invokes it
   * if found.
   *
   * @param[in,out] request The request.
   */
  Error handle(Setting_request& request)
  {
    using rapidjson::Value;
    auto& result = request.output.value_ref();
    auto& alloc = request.output.alloc_ref();
    result.SetObject();
    if (request.name == "all" || request.name == "basic") {
      const auto is_should_be_skipped = [is_basic = request.name == "basic"]
        (const std::string& name) noexcept
      {
        return is_basic && (name == "calibrationData" ||
          name == "calibrationDataApplyError" ||
          name == "calibrationDataEepromError");
      };

      switch (request.type) {
      case Setting_request_type::read:
        for (const auto& handler : table_) {
          if (is_should_be_skipped(handler.first))
            continue;

          Value res;
          Setting_request req{handler.first, Setting_request_type::read,
            request.input, {&res, &alloc}};
          if (const auto err = handler.second->handle(req))
            set_error(res, err, alloc);
          result.AddMember(Value{req.name, alloc}, std::move(res), alloc);
        }
        break;
      case Setting_request_type::write: {
        auto& input = request.input.value_ref();
        if (!input.IsObject())
          return Error{Errc::board_settings_invalid, "value is not object"};
        for (const auto& member : input.GetObject()) {
          const std::string name{member.name.GetString(), member.name.GetStringLength()};
          if (is_should_be_skipped(name))
            continue;

          if (!is_name_special(name)) {
            Value res;
            Value in{member.value, alloc};
            Setting_request req{name, Setting_request_type::write,
              {&in}, {&res, &alloc}};
            if (const auto err = handle(req))
              return err;
            else
              result.AddMember(Value{name, alloc}, std::move(res), alloc);
          } else
            return Error{Errc::board_settings_invalid, "special name requested"};
        }
        break;
      }
      }
    } else if (!request.name.empty()) {
      if (const auto handler = table_.find(request.name); handler != table_.end()) {
        Value res;
        Setting_request req{request.name, request.type, request.input, {&res, &alloc}};
        const auto err = handler->second->handle(req);
        if (!err)
          result.AddMember(Value{request.name, alloc}, std::move(res), alloc);
        return err;
      } else
        return Errc::board_settings_unknown;
    } else
      return Errc::bug;

    return Errc::ok;
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
        set ? [set = std::move(set)](auto value)
        {
          using std::is_same_v;
          static_assert(is_same_v<SetterResult, void> ||
            is_same_v<SetterResult, Error>);
          if constexpr (is_same_v<SetterResult, void>) {
            set(std::forward<decltype(value)>(value));
            return Error{};
          } else
            return set(std::forward<decltype(value)>(value));
        } : Setter{}
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
   * @param request[in,out] The request.
   */
  Error handle(Setting_request& request) override
  {
    if (request.type == Setting_request_type::write) {
      if (set_) {
        Setter_value val{};
        const auto err = get(request.input, val);
        if (!err) {
          if (const auto err = set_(val); !err) {
            if (get_) { // read back
              if (const auto [err, res] = get_(); err)
                return err;
              else
                return set(request.output, res); // Done.
            }
          } else
            return err;
        } else
          return Error{Errc::board_settings_invalid, err.what()};
      } else
        return Errc::board_settings_write_forbidden;
    } else if (request.type == Setting_request_type::read) {
      if (get_) {
        if (const auto [err, res] = get_(); err)
          return err;
        else
          return set(request.output, res); // Done.
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
 * @brief Parser of simple text protocol described in firmware-api.md.
 *
 * @details This class is responsible to parse the setting requests and to pass
 * them to a Setting_dispatcher. The syntax of requests is described in
 * firmware-api.md.
 *
 * @see Setting_dispatcher.
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
  {}

  /// @see Serial_event_handler::handle_receive().
  void handle_receive(const Character ch) override
  {
    constexpr const Character term_char{'\n'};

    // Process the request if terminal character has been received.
    if (ch == term_char) {
      // Always respond in JSON.
      rapidjson::Document response{rapidjson::kObjectType};
      auto& alloc = response.GetAllocator();

      // Invoke setting handler.
      if (state_ == State::input) {
        rapidjson::Document input;
        rapidjson::ParseResult pr;
        if (!request_input_.empty())
          pr = input.Parse(request_input_.data(), request_input_.size());
        if (pr) {
          rapidjson::Value result;
          Setting_request request{request_name_, request_type_,
            {&input}, {&result, &alloc}};
          if (const auto err = setting_dispatcher_->handle(request))
            set_error(response, err, alloc);
          else
            set_result(response, std::move(request.output.value_ref()), alloc);
        } else
          set_error(response, Errc::board_settings_invalid, alloc);
      } else
        set_error(response, Errc::board_settings_invalid, alloc);

      // Respond and return.
      CFIFO response_fifo{to_text(response)};
      response_fifo << term_char;
      serial_bus_->send(response_fifo);
      reset();
      return;
    }

  parse:
    switch (state_) {
    case State::name:
      if (ch == '<' || ch == '>') {
        state_ = State::type;
        goto parse;
      } else if (std::isalnum(ch))
        request_name_ += ch;
      else
        state_ = State::error;
      break;
    case State::type:
      if (ch == '>') {
        request_type_ = Setting_request_type::read;
        state_ = State::input;
      } else if (ch == '<') {
        request_type_ = Setting_request_type::write;
        state_ = State::input;
      } else
        state_ = State::error;
      break;
    case State::input:
      request_input_ += ch;
      break;
    case State::error:
      break;
    }
  }

private:
  /// Setting request parser state.
  enum class State {
    /// Processing a setting name.
    name,
    /// Processing a request type.
    type,
    /// Processing a request input.
    input,
    /// Protocol error.
    error
  };

  std::shared_ptr<CSerial> serial_bus_;
  std::shared_ptr<Setting_dispatcher> setting_dispatcher_;
  State state_{State::name};
  std::string request_name_;
  Setting_request_type request_type_;
  std::string request_input_;

  /// Resets the state.
  void reset()
  {
    state_ = State::name;
    request_name_.clear();
    request_type_ = Setting_request_type::read;
    request_input_.clear();
  }
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SETTINGS_HPP
