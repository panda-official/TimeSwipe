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

#ifndef PANDA_TIMESWIPE_TYPES_FWD_HPP
#define PANDA_TIMESWIPE_TYPES_FWD_HPP

/// Public API.
namespace panda::timeswipe {

enum class Errc;
enum class Measurement_mode;

class Exception;
class Generic_error_category;

class Board_settings;
class Driver;
class Driver_settings;
template<typename> class Table;

/// Implementation details.
namespace detail {
class iDriver;
class Error;
template<typename> struct Error_or;
} // namespace detail
} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_TYPES_FWD_HPP
