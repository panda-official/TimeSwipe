// -*- C++ -*-

// PANDA TimeSwipe Project
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

#ifndef PANDA_TIMESWIPE_FIRMWARE_BASE_PIN_BUTTON_HPP
#define PANDA_TIMESWIPE_FIRMWARE_BASE_PIN_BUTTON_HPP

#include "../button.hpp"
#include "../error.hpp"
#include "../os.h"

/**
 * @brief The button which uses digital pin state as an input signal with a
 * debouncing code.
 *
 * @details This class follows the CRTP design pattern. The derived class T must
 * provide:
 *   -# impl_get_signal() which returns a value of type `bool`, where `true`
 *   means "pressed" and `false` means "released". This function is called to
 *   acquire a raw signal from pin;
 *   -# impl_on_state_changed(Button_state) which returns `void`. This function
 *   is called to emit the corresponding button event.
 * To remove the signal noise (debouncing) a simple 1-order digital filter is
 * used. When filtered signal level drops down below low_threshold_ the "released"
 * state is established. When filtered signal level exceeds high_threshold_ the
 * "pressed" state is established.
 *
 * @see Button_event.
 */
template <class T>
class Pin_button {
public:
  /// Updates the button state.
  void update()
  {
    // Fixate the call time.
    const auto call_time = os::get_tick_mS();
    PANDA_TIMESWIPE_FIRMWARE_ASSERT(call_time >= last_update_time_);

    /*
     * If min update interval reached, get the signal level and apply 1-order
     * digital filter.
     */
    if (const auto elapsed = call_time - last_update_time_; elapsed >= min_update_interval_) {
      last_update_time_ = call_time;
      signal_level_ += (get_signal() - signal_level_) * elapsed * filter_factor_;
    } else
      return;

    // Determine the button state based on filtered signal level.
    if (signal_level_ >= high_threshold_)
      current_state_ = Button_state::pressed;
    else if (signal_level_ <= low_threshold_)
      current_state_ = Button_state::released;

    if (previous_state_ == Button_state::pressed) {
      const auto pressing_time = call_time - last_press_time_;
      if (!is_long_click_) {
        if (pressing_time > max_short_click_duration_) {
          is_first_of_double_click_ = false;
          is_long_click_ = true;
          on_state_changed(Button_state::long_click);
        }
      }
      if (!is_very_long_click_) {
        if (pressing_time > min_very_long_click_duration_) {
          is_first_of_double_click_ = false;
          is_very_long_click_ = true;
          on_state_changed(Button_state::very_long_click);
        }
      }
    } else if (is_first_of_double_click_) {
      if ((call_time - last_release_time_) > max_second_click_duration_) {
        is_first_of_double_click_ = false;
        on_state_changed(Button_state::short_click);
      }
    }

    // Emit Button_event if the state changed.
    if (previous_state_ != current_state_) {
      if (current_state_ == Button_state::pressed) {
        last_press_time_ = call_time;
        last_interclick_interval_ = last_press_time_ - last_release_time_;
      } else {
        is_long_click_ = false;
        is_very_long_click_ = false;

        last_release_time_ = call_time;
        current_click_duration_ = last_release_time_ - last_press_time_;

        // Click.
        if (current_click_duration_ < max_short_click_duration_) {
          if (current_click_duration_ < max_second_click_duration_) {
            if (is_first_of_double_click_) {
              if (last_interclick_interval_ < max_second_click_duration_)
                on_state_changed(Button_state::double_click);
              is_first_of_double_click_ = false;
            } else
              is_first_of_double_click_ = true;
          } else {
            on_state_changed(Button_state::short_click);
            is_first_of_double_click_ = false;
          }
        } else
          is_first_of_double_click_ = false;
      }
      on_state_changed(current_state_);
      previous_state_ = current_state_;
    }
  }

  /// @returns Low threshold level of a filtered signal level to detect Button_state::released.
  constexpr static float low_threshold() noexcept
  {
    return low_threshold_;
  }

  /// @returns High threshold level of a filtered signal level to detect Button_state::pressed.
  constexpr static float high_threshold() noexcept
  {
    return high_threshold_;
  }

  /// @returns 1-order digital filter pre-calculated factor.
  constexpr static float filter_factor() noexcept
  {
    return filter_factor_;
  }

  /// @returns The maximum duration of the "short" click, ms.
  constexpr static unsigned long max_short_click_duration() noexcept
  {
    return max_short_click_duration_;
  }

  /// @returns The maximum duration of the second click, ms.
  constexpr static unsigned long max_second_click_duration() noexcept
  {
    return max_second_click_duration_;
  }

  /// @returns The minimum duration of the "very long" click, ms.
  constexpr static unsigned long min_very_long_click_duration() noexcept
  {
    return min_very_long_click_duration_;
  }

  /// @returns Minimum valuable time between two consecutive update() calls, ms.
  constexpr static unsigned long min_update_interval() noexcept
  {
    return min_update_interval_;
  }

  /// @returns Filtered signal level.
  float signal_level() const noexcept
  {
    return signal_level_;
  }

  /**
   * @returns The time of last call of update(), ms.
   *
   * @see update().
   */
  unsigned long last_update_time() const noexcept
  {
    return last_update_time_;
  }

  /// @returns The last time when button was pressed, ms.
  unsigned long last_press_time() const noexcept
  {
    return last_press_time_;
  }

  /// @returns The last time when button was released, ms.
  unsigned long last_release_time() const noexcept
  {
    return last_release_time_;
  }

  /// @returns Current click (press/release) duration, ms.
  unsigned long current_click_duration() const noexcept
  {
    return current_click_duration_;
  }

  /// @returns `true` if the first click of double-click detected.
  bool is_first_of_double_click() const noexcept
  {
    return is_first_of_double_click_;
  }

  /// @returns `true` if "long" click detected.
  bool is_long_click() const noexcept
  {
    return is_long_click_;
  }

  /// @returns `true` if "very long" click detected.
  bool is_very_long_click() const noexcept
  {
    return is_very_long_click_;
  }

  /// @returns The current state.
  Button_state current_state() const noexcept
  {
    return current_state_;
  }

  /// @returns The previous state.
  Button_state previous_state() const noexcept
  {
    return previous_state_;
  }

private:
  constexpr static float low_threshold_{.05f};
  constexpr static float high_threshold_{.95f};
  constexpr static float filter_factor_{1.0f/(.013f*1000.0f)};
  constexpr static unsigned long max_short_click_duration_{1200};
  constexpr static unsigned long max_second_click_duration_{400};
  constexpr static unsigned long min_very_long_click_duration_{6000};
  constexpr static unsigned long min_update_interval_{4};

  float signal_level_{};
  unsigned long last_update_time_{};
  unsigned long last_press_time_{};
  unsigned long last_release_time_{};
  unsigned long current_click_duration_{};
  unsigned long last_interclick_interval_{}; // The time interval between two
                                             // consecutive clicks
                                             // (last_press_time_ - last_release_time_), ms.
  bool is_first_of_double_click_{};
  bool is_long_click_{};
  bool is_very_long_click_{};
  Button_state current_state_{Button_state::released};
  Button_state previous_state_{Button_state::released};

  /**
   * @brief Acquires a raw signal level from pin.
   *
   * @returns `true` if pressed, or `false` if released.
   */
  bool get_signal()
  {
    return static_cast<T*>(this)->impl_get_signal();
  }

  /// Emits the button event.
  void on_state_changed(const Button_state state)
  {
    static_cast<T*>(this)->impl_on_state_changed(state);
  }
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_BASE_PIN_BUTTON_HPP
