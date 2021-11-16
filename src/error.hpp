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

#include <cstring> // std::strlen
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// Errc
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief Generic error conditions.
 */
enum class Errc {
  /// Generic error.
  generic = 1,

  /// At least one of the board settings invalid.
  board_settings_invalid = 10011,
  /// At least one of the board settings insufficient.
  board_settings_insufficient = 10021,
  /// Board measurement is in progress.
  board_measurement_started = 10111,

  /// Driver doesn't initialized.
  driver_not_initialized = 20011,
  /// At least one of the driver settings invalid.
  driver_settings_invalid = 20111,
  /// At least one of the driver settings insufficient.
  driver_settings_insufficient = 20121,
  /// Attempt to use PID file as a lock indicator failed.
  driver_pid_file_lock_failed = 20211,

  /// Drift compensation references invalid.
  drift_comp_refs_invalid = 30011,
  /// Drift compensation references not found.
  drift_comp_refs_not_found = 30021,
  /// Drift compensation references not available.
  drift_comp_refs_not_available = 30031,

  /// Calibration data is invalid.
  calib_data_invalid = 40011,

  /// Attempt to send SPI request failed.
  spi_send_failed = 50011,
  /// Attempt to receive SPI request failed.
  spi_receive_failed = 50111,
  /// Attempt to execute SPI command failed.
  spi_command_failed = 50211
};

/**
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Errc.
 */
constexpr const char* to_literal(const Errc errc) noexcept
{
  switch (errc) {
  case Errc::generic:
    return "generic";

  case Errc::board_settings_invalid:
    return "board_settings_invalid";
  case Errc::board_settings_insufficient:
    return "board_settings_insufficient";
  case Errc::board_measurement_started:
    return "board_measurement_started";

  case Errc::driver_not_initialized:
    return "driver_not_initialized";
  case Errc::driver_settings_invalid:
    return "driver_settings_invalid";
  case Errc::driver_settings_insufficient:
    return "driver_settings_insufficient";
  case Errc::driver_pid_file_lock_failed:
    return "driver_pid_file_lock_failed";

  case Errc::drift_comp_refs_invalid:
    return "drift_comp_refs_invalid";
  case Errc::drift_comp_refs_not_found:
    return "drift_comp_refs_not_found";
  case Errc::drift_comp_refs_not_available:
    return "drift_comp_refs_not_available";

  case Errc::calib_data_invalid:
    return "calib_data_invalid";

  case Errc::spi_send_failed:
    return "spi_send_failed";
  case Errc::spi_receive_failed:
    return "spi_receive_failed";
  case Errc::spi_command_failed:
    return "spi_command_failed";
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

} // namespace panda::timeswipe

// -----------------------------------------------------------------------------
// Integration with std::system_error
// -----------------------------------------------------------------------------

namespace std {

/**
 * @ingroup errors
 *
 * @brief Full specialization for integration with `<system_error>`.
 */
template<>
struct is_error_condition_enum<panda::timeswipe::Errc> final : true_type {};

} // namespace std

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// Generic_error_category
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief A generic category of errors.
 */
class Generic_error_category final : public std::error_category {
public:
  /// @returns The literal `panda_timeswipe_generic_error`.
  const char* name() const noexcept override
  {
    return "panda_timeswipe_generic_error";
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
 * @returns The reference to the instance of type Generic_error_category.
 */
inline const Generic_error_category& generic_error_category() noexcept
{
  static const Generic_error_category instance;
  return instance;
}

/**
 * @ingroup errors
 *
 * @returns `std::error_condition(int(errc), generic_error_category())`.
 */
inline std::error_condition make_error_condition(const Errc errc) noexcept
{
  return {static_cast<int>(errc), generic_error_category()};
}

// -----------------------------------------------------------------------------
// Exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief The generic exception class.
 */
class Exception : public std::exception {
public:
  /**
   * @brief The constructor.
   *
   * @param errc The error condition.
   * @param what The what-string.
   */
  Exception(const std::error_condition& errc, const std::string& what)
    : what_{what}
    , condition_{errc}
  {}

  /**
   * @brief Constructs an instance associated with Errc::generic.
   *
   * @param what The what-string.
   */
  explicit Exception(const std::string& what)
    : Exception{Errc::generic, what}
  {}

  /// @returns The what-string.
  const char* what() const noexcept override
  {
    return what_.what();
  }

  /// @returns The error condition.
  std::error_condition condition() const noexcept
  {
    return condition_;
  }

private:
  std::runtime_error what_;
  std::error_condition condition_;
};

// -----------------------------------------------------------------------------
// Sys_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief An exception thrown on system error.
 */
class Sys_exception final : public Exception {
public:
  /// The constructor.
  Sys_exception(const int ev, const std::string& what)
    : Exception{std::system_category().default_error_condition(ev), what}
  {}
};

// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------

/**
 * @brief Checks the assertion `a`.
 *
 * @details Always active regardless of `NDEBUG`.
 *
 * @par Effects
 * Terminates the process if `!a`.
 */
#define PANDA_TIMESWIPE_ASSERT(a) do {                                  \
    if (!(a)) {                                                         \
      std::cerr<<"assertion ("<<#a<<") failed at "<<__FILE__<<":"<<__LINE__<<"\n"; \
      std::terminate();                                                 \
    }                                                                   \
  } while (false)

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_ERROR_HPP
