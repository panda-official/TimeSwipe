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

#ifndef PANDA_TIMESWIPE_LIMITS_HPP
#define PANDA_TIMESWIPE_LIMITS_HPP

namespace panda::timeswipe::detail {

/// The absolute maximum possible number of data channels.
constexpr unsigned max_channel_count{4};

/// The absolute maximum possible number of PWMs.
constexpr unsigned max_pwm_count{2};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_LIMITS_HPP
