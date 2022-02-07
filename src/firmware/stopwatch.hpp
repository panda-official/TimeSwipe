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

#ifndef PANDA_TIMESWIPE_FIRMWARE_STOPWATCH_HPP
#define PANDA_TIMESWIPE_FIRMWARE_STOPWATCH_HPP

#include "../common/os.h"

class Stopwatch final {
public:
  explicit Stopwatch(const unsigned long start_time)
    : start_time_{start_time}
  {}

  unsigned long uptime()
  {
    return os::get_tick_mS() - start_time_;
  }

  float uptime_seconds()
  {
    return uptime() / 1000.0f;
  }

private:
  unsigned long start_time_{};
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_STOPWATCH_HPP
