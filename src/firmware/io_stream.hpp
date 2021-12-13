// -*- C++ -*-

// PANDA Timeswipe Project
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

#ifndef PANDA_TIMESWIPE_FIRMWARE_IO_STREAM_HPP
#define PANDA_TIMESWIPE_FIRMWARE_IO_STREAM_HPP

#include "error.hpp"

#include <string>
#include <optional>
#include <utility>

/**
 * @brief An IO stream.
 *
 * @details Provides an API for operating on the stream, such as reading and
 * writing the data of the following types: `bool`, `int`, `unsigned int`,
 * `float` and `std::string`. The support of any other type can be achieved by
 * overloading operators `<<` and `>>`:
 *   -# `Io_stream& operator<<(Io_stream&, const T&)` for writing values;
 *   -# `Io_stream& operator>>(Io_stream&, T&)` for reading values;
 *   -# `Io_stream& operator<<(Io_stream&, const std::optional<T>&)` for
 *   writing semantically nullable values;
 *   -# `Io_stream& operator>>(Io_stream&, std::optional<T>&)` for
 *   reading semantically nullable values.
 * The implementation of the last two operators doesn't required since available
 * automatically if the first two operators are implemented.
 *
 * @remarks Designed as a lightweight alternative to the standard IO streams.
 */
class Io_stream {
public:
  /// The destructor.
  virtual ~Io_stream() = default;

  /// @returns `true` if the last operation was successful.
  virtual bool is_good() const noexcept = 0;

  /// Writes null value.
  virtual void write(std::nullopt_t) = 0;

  /// Writes boolean value.
  virtual void write(bool) = 0;
  /// Reads boolean value or null.
  virtual void read(std::optional<bool>&) = 0;

  /// Writes signed integer value.
  virtual void write(int) = 0;
  /// Reads signed integer value or null.
  virtual void read(std::optional<int>&) = 0;

  /// Writes unsigned integer value.
  virtual void write(unsigned int) = 0;
  /// Reads unsigned integer value or null.
  virtual void read(std::optional<unsigned int>&) = 0;

  /// Writes float value.
  virtual void write(float) = 0;
  /// Reads float value or null.
  virtual void read(std::optional<float>&) = 0;

  /// Writes string value.
  virtual void write(const std::string&) = 0;
  /// Reads string value or null.
  virtual void read(std::optional<std::string>&) = 0;
};

/**
 * Writes the `value` to the stream `os`.
 *
 * @returns os.
 */
template<typename T>
Io_stream& operator<<(Io_stream& os, const T& value)
{
  os.write(value);
  return os;
}

/// @overload
inline Io_stream& operator<<(Io_stream& os, const char* const value)
{
  PANDA_TIMESWIPE_FIRMWARE_ASSERT(value);
  os.write(std::string{value});
  return os;
}

/**
 * Writes the `value` or null to the stream `os`.
 *
 * @returns os.
 *
 * @remarks Calls `os << *value` if `value`.
 */
template<typename T>
Io_stream& operator<<(Io_stream& os, const std::optional<T>& value)
{
  if (value)
    os << *value;
  else
    os.write(std::nullopt);
  return os;
}

/**
 * Reads the `value` or null from the stream `is`.
 *
 * @returns is.
 */
template<typename T>
Io_stream& operator>>(Io_stream& is, std::optional<T>& value)
{
  is.read(value);
  return is;
}

/**
 * Reads the `value` from the stream `is`.
 *
 * @returns is.
 *
 * @remarks Calls `operator>>(is, std::optional<T>&)`.
 */
template<typename T>
Io_stream& operator>>(Io_stream& is, T& value)
{
  std::optional<T> val;
  is >> val;
  if (val) value = std::move(*val);
  return is;
}

#endif  // PANDA_TIMESWIPE_FIRMWARE_IO_STREAM_HPP
