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

#include <cstdint>
#include <memory>

class TimeSwipeEventImpl;

/// Timeswipe event
class TimeSwipeEvent final {
public:
  /**
   * \brief Button press event
   */
  class Button {
  public:
    Button() = default;
    Button(bool _pressed, unsigned _count);
    /**
     * \brief returns true when pressed and false if released
     */
    bool pressed() const;
    /**
     * \brief returns press/release counter,
     * odd value is pressed, even value is released
     */
    unsigned count() const;
  private:
    bool _pressed;
    unsigned _count;
  };

  /**
   * \brief Gain value event
   */
  class Gain {
  public:
    Gain() = default;
    Gain(int _value);
    /**
     * \brief returns Gain value as number
     */
    int value() const;
  private:
    int _value;
  };

  /**
   * \brief SetSecondary value event
   */
  class SetSecondary {
  public:
    SetSecondary() = default;
    SetSecondary(int _value);
    /**
     * \brief returns SetSecondary value as number
     */
    int value() const;
  private:
    int _value;
  };

  /**
   * \brief Bridge value event
   */
  class Bridge {
  public:
    Bridge() = default;
    Bridge(int _value);
    /**
     * \brief returns Bridge value as number
     */
    int value() const;
  private:
    int _value;
  };


  /**
   * \brief Record value event
   */
  class Record {
  public:
    Record() = default;
    Record(int _value);
    /**
     * \brief returns Record value as number
     */
    int value() const;
  private:
    int _value;
  };

  /**
   * \brief Offset value event
   */
  class Offset {
  public:
    Offset() = default;
    Offset(int _value);
    /**
     * \brief returns Offset value as number
     */
    int value() const;
  private:
    int _value;
  };

  /**
   * \brief Mode value event
   */
  class Mode {
  public:
    Mode() = default;
    Mode(int _value);
    /**
     * \brief returns Mode value as number
     */
    int value() const;
  private:
    int _value;
  };

  /**
   * \brief Check for interested event
   */
  template <class EVENT>
  bool is() const;

  /**
   * \brief get interested event
   */
  template <class EVENT>
  const EVENT& get() const;

  TimeSwipeEvent();
  ~TimeSwipeEvent();

  template <class EVENT>
  TimeSwipeEvent(EVENT&& ev);

  template <class EVENT>
  TimeSwipeEvent(const EVENT& ev);

  TimeSwipeEvent(TimeSwipeEvent&& ev) = default;
  TimeSwipeEvent(const TimeSwipeEvent& ev) = default;
  TimeSwipeEvent& operator=(const TimeSwipeEvent&) = default;
private:
  std::shared_ptr<TimeSwipeEventImpl> impl_;
};

#endif  // PANDA_TIMESWIPE_DRIVER_EVENT_HPP
