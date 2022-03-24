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

#ifndef PANDA_TIMESWIPE_BOARD_SETTINGS_HPP
#define PANDA_TIMESWIPE_BOARD_SETTINGS_HPP

#include "types_fwd.hpp"

#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace panda::timeswipe {

/**
 * @brief Board-level settings.
 *
 * @details <a href="https://github.com/panda-official/TimeSwipe/blob/v2/doc/firmware-api.md">The Communication Protocol Description</a>.
 *
 * @see Driver::set_board_settings().
 */
class Board_settings final {
public:
  /// The destructor.
  ~Board_settings();

  /// Copy-constructible.
  Board_settings(const Board_settings&);

  /// Copy-assignable.
  Board_settings& operator=(const Board_settings&);

  /// Move-constructible.
  Board_settings(Board_settings&&);

  /// Move-assignable.
  Board_settings& operator=(Board_settings&&);

  /// The default constructor.
  Board_settings();

  /**
   * @brief The constructor.
   *
   * @details Parses the JSON input.
   *
   * @see to_json_text().
   */
  explicit Board_settings(std::string_view json_text);

  /// @returns The vector of setting names.
  std::vector<std::string> names() const;

  /// @returns The vector of setting names which cannot be applied directly.
  std::vector<std::string> inapplicable_names() const;

  /// Swaps this instance with the `other` one.
  void swap(Board_settings& other) noexcept;

  /// Sets settings from `other` instance.
  Board_settings& set(const Board_settings& other);

  /// @returns The result of conversion of this instance to a JSON text.
  std::string to_json_text() const;

  /// @returns `true` if this instance has no settings.
  bool is_empty() const;

  /**
   * @brief Sets the value of the specified setting.
   *
   * @par Requires
   * The type of `value` must be one of the following:
   *   - Measurement_mode;
   *   - bool;
   *   - std::int8_t, std::int16_t, std::int32_t, std::int64_t;
   *   - std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t;
   *   - float, double;
   *   - std::string, std::string_view.
   *
   * @warning Some settings are applicable only if no measurement is started.
   *
   * @see channel_measurement_modes(), Driver::is_measurement_started().
   */
  Board_settings& set_value(std::string_view name, std::any value);

  /**
   * @returns The value of the specified setting, or empty value if the setting
   * is either not available or null. Depending on the underlying type and `name`
   * the type of the result can be one of the following:
   *   - Measurement_mode;
   *   - bool;
   *   - std::int32_t, std::int64_t;
   *   - std::uint32_t, std::uint64_t;
   *   - float, double;
   *   - std::string.
   *
   * @see set_value().
   */
  std::any value(std::string_view name) const;

private:
  friend detail::iDriver;

  struct Rep;

  explicit Board_settings(std::unique_ptr<Rep> rep);

  std::unique_ptr<Rep> rep_;
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_BOARD_SETTINGS_HPP
