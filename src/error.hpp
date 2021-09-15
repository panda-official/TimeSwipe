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

#ifndef PANDA_TIMESWIPE_ERROR_HPP
#define PANDA_TIMESWIPE_ERROR_HPP

#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// Errc
// -----------------------------------------------------------------------------

/// Error codes.
enum class Errc {
  ok = 0,

  /** Generic section **/

  generic = 1,
  out_of_range = 2,
  invalid_argument = 3,

  /** PID file section **/

  pid_file = 1001,
  pid_file_lock_failed = 1002,

  /** Board section **/

  board = 2001,
  board_invalid_setting = 2002,
  board_invalid_state = 2003,
  board_measurement_started = 2004,

  /** Driver section **/

  driver = 3001,
  driver_invalid_setting = 3002,

  /** Drift compensation section **/

  drift_comp = 4001,
  drift_comp_invalid_reference = 4002,
  drift_comp_no_references = 4003,
  drift_comp_insufficient_references = 4004,
  drift_comp_excessive_references = 4005,

  /** Calibration data section **/

  calib_data = 5001,
  calib_data_invalid = 5002,
  calib_data_invalid_atom_type = 5003,

  /** SPI section **/

  spi = 6001,
  spi_send = 6002,
  spi_receive = 6003,

  /** Communication protocol section **/

  com_proto = 7001,
  com_proto_invalid_request = 7002,
  com_proto_bus = 7003,
  com_proto_timeout = 7004,
  com_proto_object_not_found = 7005,
  com_proto_get_unsupported = 7006,
  com_proto_set_unsupported = 7007,
  com_proto_access_point_disabled = 7008
};

/// @returns `true` if `errc` indicates an error.
constexpr bool is_error(const Errc errc) noexcept
{
  return errc != Errc::ok;
}

/**
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Errc.
 */
constexpr const char* to_literal(const Errc errc) noexcept
{
  switch (errc) {
  case Errc::ok: return "ok";

  case Errc::generic: return "generic";
  case Errc::out_of_range: return "out_of_range";
  case Errc::invalid_argument: return "invalid_argument";

  case Errc::pid_file: return "pid_file";
  case Errc::pid_file_lock_failed: return "pid_file_lock_failed";

  case Errc::board: return "board";
  case Errc::board_invalid_setting: return "board_invalid_setting";
  case Errc::board_invalid_state: return "board_invalid_state";
  case Errc::board_measurement_started: return "board_measurement_started";

  case Errc::driver: return "driver";
  case Errc::driver_invalid_setting: return "driver_invalid_setting";

  case Errc::drift_comp: return "drift_comp";
  case Errc::drift_comp_invalid_reference: return "drift_comp_invalid_reference";
  case Errc::drift_comp_no_references: return "drift_comp_no_references";
  case Errc::drift_comp_insufficient_references: return "drift_comp_insufficient_references";
  case Errc::drift_comp_excessive_references: return "drift_comp_excessive_references";

  case Errc::calib_data: return "calib_data";
  case Errc::calib_data_invalid: return "calib_data_invalid";
  case Errc::calib_data_invalid_atom_type: return "calib_data_invalid_atom_type";

  case Errc::spi: return "spi";
  case Errc::spi_send: return "spi_send";
  case Errc::spi_receive: return "spi_receive";

  case Errc::com_proto: return "com_proto";
  case Errc::com_proto_invalid_request: return "com_proto_invalid_request";
  case Errc::com_proto_bus: return "com_proto_bus";
  case Errc::com_proto_timeout: return "com_proto_timeout";
  case Errc::com_proto_object_not_found: return "com_proto_object_not_found";
  case Errc::com_proto_get_unsupported: return "com_proto_get_unsupported";
  case Errc::com_proto_set_unsupported: return "com_proto_set_unsupported";
  case Errc::com_proto_access_point_disabled: return "com_proto_access_point_disabled";
  }
  return nullptr;
}

/**
 * @returns The literal returned by `to_literal(errc)`, or literal
 * `unknown error` if `to_literal(errc)` returned `nullptr`.
 */
constexpr const char* to_literal_anyway(const Errc errc) noexcept
{
  constexpr const char* unknown{"unknown error"};
  const char* const literal{to_literal(errc)};
  return literal ? literal : unknown;
}

// -----------------------------------------------------------------------------
// Error_category
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief A category of driver errors.
 */
class Error_category final : public std::error_category {
public:
  /// @returns The literal `panda_timeswipe_error`.
  const char* name() const noexcept override
  {
    return "panda_timeswipe_error";
  }

  /**
   * @returns The string that describes the error condition denoted by `ev`.
   *
   * @par Requires
   * `ev` must corresponds to the value of Errc.
   *
   * @remarks The caller should not rely on the return value as it is a
   * subject to change.
   */
  std::string message(const int ev) const override
  {
    const char* const desc{to_literal_anyway(static_cast<Errc>(ev))};
    constexpr const char* const sep{": "};
    std::string result;
    result.reserve(std::strlen(name()) + std::strlen(sep) + std::strlen(desc));
    return result.append(name()).append(sep).append(desc);
  }
};

/**
 * @ingroup errors
 *
 * @returns The reference to the instance of type Error_category.
 */
inline const Error_category& error_category() noexcept
{
  static const Error_category instance;
  return instance;
}

/**
 * @ingroup errors
 *
 * @returns `std::error_condition(int(errc), error_category())`.
 */
inline std::error_condition make_error_condition(const Errc errc) noexcept
{
  return std::error_condition{static_cast<int>(errc), error_category()};
}

} // namespace panda::timeswipe

// -----------------------------------------------------------------------------
// Integration with std::system_error
// -----------------------------------------------------------------------------

namespace std {

/**
 * @ingroup errors
 *
 * @brief The full specialization for integration with `<system_error>`.
 */
template<> struct is_error_condition_enum<panda::timeswipe::Errc> final : true_type {};

} // namespace std

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// Exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * An exception class.
 */
class Exception : public std::exception {
public:
  /**
   * The constructor of instance which represents the generic error.
   *
   * @param what The custom what-string. If ommitted, the value returned by
   * `to_literal(errc)` will be used as a what-string.
   */
  Exception(std::string what = {})
    : Exception{Errc::generic, std::move(what)}
  {}

  /**
   * The constructor.
   *
   * @param errc The error condition.
   * @param what The custom what-string. If ommitted, the value returned by
   * `to_literal(errc)` will be used as a what-string.
   */
  explicit Exception(const Errc errc, std::string what = {})
    : condition_{errc}
    , what_holder_{what.empty() ? to_literal_anyway(errc) : what}
  {}

  /// @see std::exception::what().
  const char* what() const noexcept override
  {
    return what_holder_.what();
  }

  /// @returns Error condition.
  std::error_condition condition() const noexcept
  {
    return condition_;
  }

private:
  std::error_condition condition_;
  std::runtime_error what_holder_;
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_ERROR_HPP
