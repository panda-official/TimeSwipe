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

/// Current major version.
constexpr int version_major{0};
static_assert(0 <= version_major && version_major <= 99);

/// Current minor version.
constexpr int version_minor{1};
static_assert(0 <= version_minor && version_minor <= 99);

/// Current patch version.
constexpr int version_patch{1};
static_assert(0 <= version_patch && version_patch <= 99);

/// Current version.
constexpr int version{version_major*10000 + version_minor*100 + version_patch};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_VERSION_HPP
