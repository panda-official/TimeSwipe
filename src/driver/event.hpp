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

#ifndef PANDA_TIMESWIPE_DRIVER_EVENT_HPP
#define PANDA_TIMESWIPE_DRIVER_EVENT_HPP

#include <memory>

/// Timeswipe event
class TimeSwipeEvent final {
public:
  /// Button pressed event.
  class Button final {
  public:
    /// The default constructor.
    Button() = default;

    /// The constructor.
    Button(bool pressed, int count);

    /// @returns `true` when pressed, or `false` if released.
    bool pressed() const;

    /// @returns Pressed (odd value) or released (even value) count.
    int count() const;

  private:
    bool pressed_{};
    int count_{};
  };

  /// Gain value event.
  class Gain final {
  public:
    /// The default constructor.
    Gain() = default;

    /// The constructor.
    explicit Gain(int value);

    /// @returns Gain value as number.
    int value() const;

  private:
    int value_{};
  };

  /// SetSecondary value event.
  class SetSecondary {
  public:
    /// The default constructor.
    SetSecondary() = default;

    /// The constructor.
    explicit SetSecondary(int value);

    /// @returns SetSecondary value as number.
    int value() const;

  private:
    int value_{};
  };

  /// Bridge value event.
  class Bridge {
  public:
    /// The default constructor.
    Bridge() = default;

    /// The constructor.
    Bridge(int value);

    /// @returns Bridge value as number.
    int value() const;
  private:
    int value_{};
  };

  /// Record value event.
  class Record {
  public:
    /// The default constructor.
    Record() = default;

    /// The constructor.
    explicit Record(int value);

    /// @returns Record value as number.
    int value() const;

  private:
    int value_{};
  };

  /// Offset value event.
  class Offset {
  public:
    /// The default constructor.
    Offset() = default;

    /// The constructor.
    explicit Offset(int value);

    /// @returns Offset value as number.
    int value() const;

  private:
    int value_{};
  };

  /// Mode value event.
  class Mode {
  public:
    /// The default constructor.
    Mode() = default;

    /// The constructor.
    Mode(int value);

    /// @returns Mode value as number.
    int value() const;

  private:
    int value_{};
  };

  /// The destructor.
  ~TimeSwipeEvent();

  /// The default constructor.
  TimeSwipeEvent();

  /// The constructor.
  template <class EVENT>
  TimeSwipeEvent(EVENT ev);

  /**
   * @returns The pointer to the event, or `nullptr` if this event is not
   * the event of type `EVENT`.
   */
  template <class EVENT>
  const EVENT* Get() const noexcept;

private:
  struct Rep;
  std::shared_ptr<Rep> rep_;
};

#endif  // PANDA_TIMESWIPE_DRIVER_EVENT_HPP
