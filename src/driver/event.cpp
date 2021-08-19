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

#include "event.hpp"

#include <type_traits>
#include <variant>

struct TimeSwipeEvent::Rep final {
  Rep() = default;

  template<class E>
  Rep(E&& e)
    : events{std::move(e)}
  {}

  std::variant<
    TimeSwipeEvent::Button,
    TimeSwipeEvent::Gain,
    TimeSwipeEvent::SetSecondary,
    TimeSwipeEvent::Bridge,
    TimeSwipeEvent::Record,
    TimeSwipeEvent::Offset,
    TimeSwipeEvent::Mode
    > events;
  friend TimeSwipeEvent;
};

TimeSwipeEvent::~TimeSwipeEvent() = default;

TimeSwipeEvent::TimeSwipeEvent()
  : rep_{std::make_shared<Rep>()}
{}

template <class EVENT>
TimeSwipeEvent::TimeSwipeEvent(EVENT ev)
  : rep_{std::make_shared<Rep>(std::move(ev))}
{}
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Button ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Gain ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::SetSecondary ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Bridge ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Record ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Offset ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Mode ev);

TimeSwipeEvent::Button::Button(bool pressed, int count)
  : pressed_{pressed}
  , count_{count}
{}

bool TimeSwipeEvent::Button::pressed() const
{
  return pressed_;
}

int TimeSwipeEvent::Button::count() const
{
  return count_;
}

TimeSwipeEvent::Gain::Gain(int value)
  : value_{value}
{}

int TimeSwipeEvent::Gain::value() const
{
  return value_;
}

TimeSwipeEvent::SetSecondary::SetSecondary(int value)
  : value_{value}
{}

int TimeSwipeEvent::SetSecondary::value() const
{
  return value_;
}

TimeSwipeEvent::Bridge::Bridge(int value)
  : value_{value}
{}

int TimeSwipeEvent::Bridge::value() const
{
  return value_;
}

TimeSwipeEvent::Record::Record(int value)
  : value_{value}
{}

int TimeSwipeEvent::Record::value() const
{
  return value_;
}

TimeSwipeEvent::Offset::Offset(int value)
  : value_{value}
{}

int TimeSwipeEvent::Offset::value() const
{
  return value_;
}

TimeSwipeEvent::Mode::Mode(int value)
  : value_(value)
{}

int TimeSwipeEvent::Mode::value() const
{
  return value_;
}

template <class EVENT>
const EVENT* TimeSwipeEvent::Get() const noexcept
{
  return std::visit([](const auto& event)
  {
    using E = std::decay_t<decltype(event)>;
    if constexpr (const EVENT* const null{}; std::is_same_v<E, EVENT>)
      return &event;
    else
      return null;
  }, rep_->events);
}
template const TimeSwipeEvent::Button* TimeSwipeEvent::Get<TimeSwipeEvent::Button>() const noexcept;
template const TimeSwipeEvent::Gain* TimeSwipeEvent::Get<TimeSwipeEvent::Gain>() const noexcept;
template const TimeSwipeEvent::SetSecondary* TimeSwipeEvent::Get<TimeSwipeEvent::SetSecondary>() const noexcept;
template const TimeSwipeEvent::Bridge* TimeSwipeEvent::Get<TimeSwipeEvent::Bridge>() const noexcept;
template const TimeSwipeEvent::Record* TimeSwipeEvent::Get<TimeSwipeEvent::Record>() const noexcept;
template const TimeSwipeEvent::Offset* TimeSwipeEvent::Get<TimeSwipeEvent::Offset>() const noexcept;
template const TimeSwipeEvent::Mode* TimeSwipeEvent::Get<TimeSwipeEvent::Mode>() const noexcept;
