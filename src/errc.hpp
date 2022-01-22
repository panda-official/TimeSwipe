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

/**
 * @file
 *
 * @remarks The code of this file is exception-free!
 */

#ifndef PANDA_TIMESWIPE_ERRC_HPP
#define PANDA_TIMESWIPE_ERRC_HPP

namespace panda::timeswipe {

/**
 * @ingroup errors
 *
 * @brief Generic error conditions.
 *
 * @see to_literal(Errc).
 */
enum class Errc {
  /// Not an error.
  ok = 0,

  /// Generic error.
  generic = 1,

  /// Bug. (Unexpected case.)
  bug = 11,

  /// Out of memory.
  out_of_memory = 111,

  /// At least one of the board settings invalid.
  board_settings_invalid = 10011,
  /// At least one of the board settings unknown.
  board_settings_unknown = 10021,
  /// Read for at least one of the board settings is forbidden.
  board_settings_read_forbidden = 10031,
  /// Write for at least one of the board settings is forbidden.
  board_settings_write_forbidden = 10041,
  /// Calibration data provided is invalid.
  board_settings_calibration_data_invalid = 10051,
  /// Calibration procedure is forbidden.
  board_settings_calibration_forbidden = 10061,
  /// At least one of the board settings insufficient.
  board_settings_insufficient = 10071,
  /// Board measurement mode is started.
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

  /// Attempt to send SPI request failed.
  spi_send_failed = 40011,
  /// Attempt to receive SPI request failed.
  spi_receive_failed = 40111,
  /// Attempt to execute SPI command failed.
  spi_command_failed = 40211,

  /// EEPROM is not available (neither read nor write are possible).
  hat_eeprom_unavailable = 50011,
  /// Whole EEPROM data is corrupted.
  hat_eeprom_data_corrupted = 50111,
  /// Some atom of EEPROM data is corrupted.
  hat_eeprom_atom_corrupted = 50211,
  /// Requested atom is not presents in EEPROM.
  hat_eeprom_atom_missed = 50221
};

/**
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Errc.
 *
 * @see Errc, to_literal_anyway(Errc).
 */
constexpr const char* to_literal(const Errc errc) noexcept
{
  switch (errc) {
  case Errc::ok:
    return "ok";

  case Errc::generic:
    return "generic";
  case Errc::bug:
    return "bug";

  case Errc::out_of_memory:
    return "out_of_memory";

  case Errc::board_settings_invalid:
    return "board_settings_invalid";
  case Errc::board_settings_unknown:
    return "board_settings_unknown";
  case Errc::board_settings_read_forbidden:
    return "board_settings_read_forbidden";
  case Errc::board_settings_write_forbidden:
    return "board_settings_write_forbidden";
  case Errc::board_settings_calibration_data_invalid:
    return "board_settings_calibration_data_invalid";
  case Errc::board_settings_calibration_forbidden:
    return "board_settings_calibration_forbidden";
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

  case Errc::spi_send_failed:
    return "spi_send_failed";
  case Errc::spi_receive_failed:
    return "spi_receive_failed";
  case Errc::spi_command_failed:
    return "spi_command_failed";

  case Errc::hat_eeprom_unavailable:
    return "hat_eeprom_unavailable";
  case Errc::hat_eeprom_data_corrupted:
    return "hat_eeprom_data_corrupted";
  case Errc::hat_eeprom_atom_corrupted:
    return "hat_eeprom_atom_corrupted";
  case Errc::hat_eeprom_atom_missed:
    return "hat_eeprom_atom_missed";
  }
  return nullptr;
}

/**
 * @returns The literal returned by `to_literal(errc)`, or literal
 * `unknown error` if `to_literal(errc)` returned `nullptr`.
 *
 * @see to_literal(Errc).
 */
constexpr const char* to_literal_anyway(const Errc errc) noexcept
{
  constexpr const char* unknown{"unknown error"};
  const char* const literal{to_literal(errc)};
  return literal ? literal : unknown;
}

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_ERRC_HPP
