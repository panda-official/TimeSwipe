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
#include <type_traits>
#include <variant>

namespace panda::timeswipe::driver {

/// Timeswipe event.
class Event final {
public:
  /// Button pressed event.
  class Button final {
  public:
    /// The default constructor.
    Button() noexcept = default;

    /// The constructor.
    Button(const bool pressed, const int count)
      : is_pressed_{pressed}
      , count_{count}
    {}

    /// @returns `true` when pressed, or `false` if released.
    bool IsPressed() const noexcept
    {
      return is_pressed_;
    }

    /// @returns Pressed (odd value) or released (even value) count.
    int GetCount() const noexcept
    {
      return count_;
    }

  private:
    bool is_pressed_{};
    int count_{};
  };

  /// Gain value event.
  class Gain final {
  public:
    /// The default constructor.
    Gain() noexcept = default;

    /// The constructor.
    explicit Gain(const int value)
      : value_{value}
    {}

    /// @returns Gain value as number.
    int GetValue() const noexcept
    {
      return value_;
    }

  private:
    int value_{};
  };

  /// SetSecondary value event.
  class SetSecondary {
  public:
    /// The default constructor.
    SetSecondary() noexcept = default;

    /// The constructor.
    explicit SetSecondary(const int value)
      : value_{value}
    {}

    /// @returns SetSecondary value as number.
    int GetValue() const noexcept
    {
      return value_;
    }

  private:
    int value_{};
  };

  /// Bridge value event.
  class Bridge {
  public:
    /// The default constructor.
    Bridge() noexcept = default;

    /// The constructor.
    Bridge(const int value)
      : value_{value}
    {}

    /// @returns Bridge value as number.
    int GetValue() const noexcept
    {
      return value_;
    }

  private:
    int value_{};
  };

  /// Record value event.
  class Record {
  public:
    /// The default constructor.
    Record() noexcept = default;

    /// The constructor.
    explicit Record(const int value)
      : value_{value}
    {}

    /// @returns Record value as number.
    int GetValue() const noexcept
    {
      return value_;
    }

  private:
    int value_{};
  };

  /// Offset value event.
  class Offset {
  public:
    /// The default constructor.
    Offset() noexcept = default;

    /// The constructor.
    explicit Offset(const int value)
      : value_{value}
    {}

    /// @returns Offset value as number.
    int GetValue() const noexcept
    {
      return value_;
    }

  private:
    int value_{};
  };

  /// Mode value event.
  class Mode {
  public:
    /// The default constructor.
    Mode() noexcept = default;

    /// The constructor.
    Mode(const int value)
      : value_(value)
    {}

    /// @returns Mode value as number.
    int GetValue() const noexcept
    {
      return value_;
    }

  private:
    int value_{};
  };

  /// The default constructor.
  Event() noexcept = default;

  /// The constructor.
  template <class E>
  Event(E&& e)
    : event_{std::move(e)}
  {}

  /**
   * @returns The pointer to the event, or `nullptr` if this event is not
   * the event of type `Event`.
   */
  template <class Ev>
  const Ev* Get() const noexcept
  {
    return std::visit([](const auto& event)
    {
      using E = std::decay_t<decltype(event)>;
      const Ev* const null{};
      (void)null;
      if constexpr (std::is_same_v<E, Event>)
        return &event;
      else
        return null;
    }, event_);
  }

private:
  std::variant<Button, Gain, SetSecondary, Bridge, Record, Offset, Mode> event_;
};

} // namespace panda::timeswipe::driver

#endif  // PANDA_TIMESWIPE_DRIVER_EVENT_HPP
