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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SAM_BUTTON_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SAM_BUTTON_HPP

#include "../pin_button.hpp"
#include "../json/json_evsys.h"

/**
 * @brief Panda board button.
 *
 * @details This class follows the Singleton design pattern. Emits JSON event
 * according to the button state.
 */
class Sam_button final : public Pin_button<Sam_button>, public CJSONEvCP {
public:
  /// Non copy-constructible.
  Sam_button(const Sam_button&) = delete;

  /// Non copy-assignable.
  Sam_button& operator=(const Sam_button&) = delete;

  /// Non move-constructible.
  Sam_button(Sam_button&&) = delete;

  /// Non move-assignable.
  Sam_button& operator=(Sam_button&&) = delete;

  /// @returns The instance of this class.
  static Sam_button& instance()
  {
    static Sam_button instance;
    return instance;
  }

  /**
   * @brief Sets extra handler of Button_event.
   *
   * @deprecated Initialy it was used to subscribe a menu object for button events
   *
   * @todo Use only IJSONEvent instead?
   */
  void set_extra_handler(const std::shared_ptr<Button_event>& handler)
  {
    extra_handler_ = handler;
  }

  /// Enables or disables the button LED.
  void enable_led(bool on);

  /// @returns `true` if the button LED is on.
  bool is_led_enabled() const noexcept;

  /// Toggles the button LED.
  void toggle_led()
  {
    enable_led(!is_led_enabled());
  }

  /**
   * @returns The total state count: even/odd values means the latest state was
   * "released"/"pressed" correspondingly.
   */
  unsigned long total_state_count() const noexcept
  {
    return total_state_count_;
  }

private:
  unsigned long total_state_count_{};
  std::shared_ptr<Button_event> extra_handler_;

  /// Default-constructible.
  Sam_button();

  // ---------------------------------------------------------------------------
  // CRTP stuff
  // ---------------------------------------------------------------------------

  friend Pin_button<Sam_button>;

  /// @returns `true` if the button pressed, or `false` if released.
  bool do_get_signal();

  /**
   * @brief Calls the both Button_event::OnButtonState and IJSONEvent::on_event
   * event callbacks.
   */
  void do_on_state_changed(Button_state state);
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SAM_BUTTON_HPP
