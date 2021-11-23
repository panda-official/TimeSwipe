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

/**
 * @file
 * Basic button event interfaces.
 */

#ifndef PANDA_TIMESWIPE_FIRMWARE_BUTTON_HPP
#define PANDA_TIMESWIPE_FIRMWARE_BUTTON_HPP

/// Button state.
enum class typeButtonState {
  pressed,
  released,
  short_click,
  long_click,
  double_click,
  very_long_click
};

/// Button event.
class CButtonEvent {
public:
  /// The destructor.
  virtual ~CButtonEvent() = default;

  /// Called when the button state changed.
  virtual void OnButtonState(typeButtonState nState) = 0;

  /// Default-constructible.
  CButtonEvent() = default;

  /// Non copy-constructible.
  CButtonEvent(const CButtonEvent&) = delete;

  /// Non copy-assignable.
  CButtonEvent& operator=(const CButtonEvent&) = delete;
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_BUTTON_HPP
