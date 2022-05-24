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

#ifndef PANDA_TIMESWIPE_FIRMWARE_PIN_HPP
#define PANDA_TIMESWIPE_FIRMWARE_PIN_HPP

#include "os.h"

#include <chrono>

/**
 * @brief Pin control abstraction.
 *
 * @details There are two possible Pin behaviors:
 *   - normal behavior: logical `true` denotes high output level (1), logical
 *   `false` denotes low output level (0);
 *   - inverted behavior: logical `true` denotes low output level (0), logical
 *   `false` denotes high output level (1).
 */
class Pin {
public:
  /// The destructor.
  virtual ~Pin() = default;

  /// The default-constructible.
  Pin() = default;

  /// Non copy-constructible.
  Pin(const Pin&) = delete;

  /// Non copy-assignable.
  Pin& operator=(const Pin&) = delete;

  /// Non move-constructible.
  Pin(Pin&&) = delete;

  /// Non move-assignable.
  Pin& operator=(Pin&&) = delete;

  /**
   * @brief Sets logic state of the pin.
   *
   * @param state The logical state to be set.
   *
   * @remarks May differ from actual output level.
   *
   * @see set_inverted().
   */
  void write(const bool state)
  {
    do_write(is_inverted_ ^ state);
    if (setup_time_ > std::chrono::microseconds::zero())
      os::uwait(setup_time_.count());
  }

  /**
   * @brief Reads back logic state of the pin.
   *
   * @returns The pin value that was written.
   */
  bool read_back() const noexcept
  {
    return is_inverted_ ^ do_read_back();
  }

  /**
   * @returns Measured logic state when pin acts as an input.
   *
   * @remarks May differ from actual output level.
   *
   * @see set_inverted().
   */
  bool read() const noexcept
  {
    return is_inverted_ ^ do_read();
  }

  /// Enables or disables inverted logic behavior of the pin.
  void set_inverted(const bool value)
  {
    is_inverted_ = value;
  }

  /// @returns `true` if the bevaior of this pin is inverted.
  bool is_inverted() const noexcept
  {
    return is_inverted_;
  }

  /**
   * @brief Sets output level setup time.
   *
   * @details In general, pin output level doesn't changes instantly. It takes
   * a while to wait for the level to rise or fall.
   *
   * @param value The setup time.
   *
   * @warning Non-positive setup time value will be ignored!
   */
  void set_setup_time(const std::chrono::microseconds value)
  {
    setup_time_ = value;
  }

  /// @returns Output level setup time.
  std::chrono::microseconds setup_time() const noexcept
  {
    return setup_time_;
  }

private:
  /// Called by write().
  virtual void do_write(bool state) = 0;

  /// Called by read_back().
  virtual bool do_read_back() const noexcept = 0;

  /// Called by read().
  virtual bool do_read() const noexcept = 0;

private:
  bool is_inverted_{};
  std::chrono::microseconds setup_time_{};
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_PIN_HPP
