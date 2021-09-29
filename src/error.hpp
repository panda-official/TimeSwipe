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
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// Errc
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * Error conditions.
 */
enum class Errc {
  /** Generic section **/

  generic = 1,

  /** Board section **/

  board = 10001,
  board_setting_channel_gain_invalid = 10011,
  board_setting_pwm_frequency_invalid = 10021,
  board_setting_pwm_signal_level_invalid = 10031,
  board_setting_pwm_repeat_count_invalid = 10041,
  board_setting_pwm_duty_cycle_invalid = 10051,
  board_settings_insufficient = 10101,
  board_state_invalid = 10201,
  board_measurement_started = 10301,

  /** Driver section **/

  driver = 20001,
  driver_setting_invalid = 20011,
  driver_setting_translation_offsets_invalid = 20021,
  driver_setting_translation_slopes_invalid = 20031,
  driver_setting_sample_rate_invalid = 20041,
  driver_setting_burst_buffer_size_invalid = 20051,
  driver_setting_frequency_invalid = 20061,
  driver_settings_insufficient = 20101,
  driver_settings_mutually_exclusive = 20201,
  driver_pid_file_lock_failed = 20301,

  /** Drift compensation feature errors **/

  drift_comp = 30001,
  drift_comp_reference_invalid = 30011,
  drift_comp_references_insufficient = 30101,
  drift_comp_references_excessive = 30111,
  drift_comp_references_not_found = 30121,
  drift_comp_references_not_available = 30131,

  /** Calibration data section **/

  calib_data = 40001,
  calib_data_invalid = 40011,
  calib_data_atom_type_invalid = 40101,
  calib_data_atom_entry_invalid = 40111,

  /** SPI section **/

  spi = 50001,
  spi_send = 50011,
  spi_receive = 50101,

  /** Communication protocol section **/

  com_proto = 60001,
  com_proto_request_invalid = 60011,
  com_proto_bus = 60101,
  com_proto_timeout = 60201,
  com_proto_object_not_found = 60301,
  com_proto_get_unsupported = 60401,
  com_proto_set_unsupported = 60501,
  com_proto_access_point_disabled = 60601
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

  case Errc::board:
    return "board";
  case Errc::board_setting_channel_gain_invalid:
    return "board_setting_channel_gain_invalid";
  case Errc::board_setting_pwm_frequency_invalid:
    return "board_setting_pwm_frequency_invalid";
  case Errc::board_setting_pwm_signal_level_invalid:
    return "board_setting_pwm_signal_level_invalid";
  case Errc::board_setting_pwm_repeat_count_invalid:
    return "board_setting_pwm_repeat_count_invalid";
  case Errc::board_setting_pwm_duty_cycle_invalid:
    return "board_setting_pwm_duty_cycle_invalid";
  case Errc::board_settings_insufficient:
    return "board_settings_insufficient";
  case Errc::board_state_invalid:
    return "board_state_invalid";
  case Errc::board_measurement_started:
    return "board_measurement_started";

  case Errc::driver:
    return "driver";
  case Errc::driver_setting_invalid:
    return "driver_setting_invalid";
  case Errc::driver_setting_translation_offsets_invalid:
    return "driver_setting_translation_offsets_invalid";
  case Errc::driver_setting_translation_slopes_invalid:
    return "driver_setting_translation_slopes_invalid";
  case Errc::driver_setting_sample_rate_invalid:
    return "driver_setting_sample_rate_invalid";
  case Errc::driver_setting_burst_buffer_size_invalid:
    return "driver_setting_burst_buffer_size_invalid";
  case Errc::driver_setting_frequency_invalid:
    return "driver_setting_frequency_invalid";
  case Errc::driver_settings_insufficient:
    return "driver_settings_insufficient";
  case Errc::driver_settings_mutually_exclusive:
    return "driver_settings_mutually_exclusive";
  case Errc::driver_pid_file_lock_failed:
    return "driver_pid_file_lock_failed";

  case Errc::drift_comp:
    return "drift_comp";
  case Errc::drift_comp_reference_invalid:
    return "drift_comp_reference_invalid";
  case Errc::drift_comp_references_insufficient:
    return "drift_comp_references_insufficient";
  case Errc::drift_comp_references_excessive:
    return "drift_comp_references_excessive";
  case Errc::drift_comp_references_not_found:
    return "drift_comp_references_not_found";
  case Errc::drift_comp_references_not_available:
    return "drift_comp_references_not_available";

  case Errc::calib_data:
    return "calib_data";
  case Errc::calib_data_invalid:
    return "calib_data_invalid";
  case Errc::calib_data_atom_type_invalid:
    return "calib_data_atom_type_invalid";
  case Errc::calib_data_atom_entry_invalid:
    return "calib_data_atom_entry_invalid";

  case Errc::spi:
    return "spi";
  case Errc::spi_send:
    return "spi_send";
  case Errc::spi_receive:
    return "spi_receive";

  case Errc::com_proto:
    return "com_proto";
  case Errc::com_proto_request_invalid:
    return "com_proto_request_invalid";
  case Errc::com_proto_bus:
    return "com_proto_bus";
  case Errc::com_proto_timeout:
    return "com_proto_timeout";
  case Errc::com_proto_object_not_found:
    return "com_proto_object_not_found";
  case Errc::com_proto_get_unsupported:
    return "com_proto_get_unsupported";
  case Errc::com_proto_set_unsupported:
    return "com_proto_set_unsupported";
  case Errc::com_proto_access_point_disabled:
    return "com_proto_access_point_disabled";
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
template<> struct is_error_condition_enum<panda::timeswipe::Errc> final : true_type {};

} // namespace std

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// Error_category
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
 * The base exception class.
 */
class Exception : public std::exception {
public:
  /// @returns The what-string.
  virtual const char* what() const noexcept override = 0;

  /// @returns The error condition.
  virtual std::error_condition condition() const noexcept = 0;
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_ERROR_HPP
