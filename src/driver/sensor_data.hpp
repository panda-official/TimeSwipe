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

#ifndef PANDA_TIMESWIPE_DRIVER_SENSOR_DATA_HPP
#define PANDA_TIMESWIPE_DRIVER_SENSOR_DATA_HPP

#include "basics.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace panda::timeswipe::driver {

/// Sensor data.
class Sensors_data final {
public:
  using value_type = std::vector<float>;
  using size_type = value_type::size_type;

  /// @returns The sensor count.
  /// FIXME: remove me.
  static constexpr std::size_t get_sensor_count() noexcept
  {
    return max_data_channel_count;
  }

  /// @returns The number of values per sensor.
  size_type get_size() const noexcept
  {
    return !data_.empty() ? data_[0].size() : 0;
  }

  /// For STL compatibility.
  size_type size() const noexcept
  {
    return get_size();
  }

  /// @returns The reference to the value at the given `index`.
  const value_type& operator[](const size_type index) const noexcept
  {
    return data_[index];
  }

  /// @overload
  value_type& operator[](const size_type index) noexcept
  {
    return const_cast<value_type&>(static_cast<const Sensors_data*>(this)->operator[](index));
  }

  void reserve(const size_type size)
  {
    const auto sc = get_sensor_count();
    for (std::size_t i{}; i < sc; ++i)
      data_[i].reserve(size);
  }

  void resize(const size_type size)
  {
    const auto sc = get_sensor_count();
    for (std::size_t i{}; i < sc; ++i)
      data_[i].resize(size);
  }

  void clear() noexcept
  {
    const auto sc = get_sensor_count();
    for (std::size_t i{}; i < sc; ++i)
      data_[i].clear();
  }

  bool is_empty() const noexcept
  {
    return !get_size();
  }

  /// For STL compatibility.
  bool empty() const noexcept
  {
    return is_empty();
  }

  void append(const Sensors_data& other)
  {
    append(other, other.get_size());
  }

  void append(const Sensors_data& other, const size_type count)
  {
    const auto sc = get_sensor_count();
    for (std::size_t i{}; i < sc; ++i) {
      const auto in_size = std::min<std::size_t>(other.data_[i].size(), count);
      const auto out_offset = data_[i].size();
      data_[i].resize(data_[i].size() + in_size);
      const auto b = other.data_[i].begin();
      std::copy(b, b + in_size, data_[i].begin() + out_offset);
    }
  }

  void erase_front(const size_type count) noexcept
  {
    const auto sc = get_sensor_count();
    for (std::size_t i{}; i < sc; i++)
      data_[i].erase(data_[i].begin(), data_[i].begin() + count);
  }

  void erase_back(const size_type count) noexcept
  {
    const auto sc = get_sensor_count();
    for (std::size_t i{}; i < sc; i++)
      data_[i].resize(data_[i].size() - count);
  }

  /// @name Iterators
  /// @{

  /// @returns Iterator that points to a first channel.
  auto begin() noexcept
  {
    return data_.begin();
  }

  /// @returns Constant iterator that points to a first channel.
  auto begin() const noexcept
  {
    return data_.begin();
  }

  /// @returns Constant iterator that points to a first channel.
  auto cbegin() const noexcept
  {
    return data_.cbegin();
  }

  /// @returns Iterator that points to an one-past-the-last channel.
  auto end() noexcept
  {
    return data_.end();
  }

  /// @returns Constant iterator that points to an one-past-the-last channel.
  auto end() const noexcept
  {
    return data_.end();
  }

  /// @returns Constant iterator that points to an one-past-the-last channel.
  auto cend() const noexcept
  {
    return data_.cend();
  }

  /// @}
private:
  using Array = std::array<std::vector<float>, max_data_channel_count>;
  Array data_;
  int sensor_count_{max_data_channel_count};
};

} // namespace panda::timeswipe::driver

#endif  // PANDA_TIMESWIPE_DRIVER_SENSOR_DATA_HPP
