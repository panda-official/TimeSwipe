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

#ifndef PANDA_TIMESWIPE_VERSION_HPP
#define PANDA_TIMESWIPE_VERSION_HPP

namespace panda::timeswipe::detail {

/// Current major driver version.
constexpr int driver_version_major{2};
static_assert(0 <= driver_version_major && driver_version_major <= 99);

/// Current minor driver version.
constexpr int driver_version_minor{1};
static_assert(0 <= driver_version_minor && driver_version_minor <= 99);

/// Current patch driver version.
constexpr int driver_version_patch{0};
static_assert(0 <= driver_version_patch && driver_version_patch <= 99);

/// Current driver version.
constexpr int driver_version{driver_version_major*10000 +
  driver_version_minor*100 + driver_version_patch};

// -----------------------------------------------------------------------------

/// Current major firmware version.
constexpr int firmware_version_major{2};
static_assert(0 <= firmware_version_major && firmware_version_major <= 99);

/// Current minor firmware version.
constexpr int firmware_version_minor{0};
static_assert(0 <= firmware_version_minor && firmware_version_minor <= 99);

/// Current patch firmware version.
constexpr int firmware_version_patch{0};
static_assert(0 <= firmware_version_patch && firmware_version_patch <= 99);

/// Current firmware version.
constexpr int firmware_version{firmware_version_major*10000 +
  firmware_version_minor*100 + firmware_version_patch};

// -----------------------------------------------------------------------------

static_assert(driver_version_major == firmware_version_major);

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_VERSION_HPP
