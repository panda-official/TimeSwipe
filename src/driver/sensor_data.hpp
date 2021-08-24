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

#ifndef PANDA_TIMESWIPE_DRIVER_SENSOR_DATA_HPP
#define PANDA_TIMESWIPE_DRIVER_SENSOR_DATA_HPP

#include <array>
#include <cstdint>
#include <vector>

/// Sensor data.
class SensorsData final {
  static constexpr std::size_t SENSORS = 4;
  using CONTAINER = std::array<std::vector<float>, SENSORS>;
public:
  using Value = std::vector<float>;

  /**
   * \brief Get number of sensors
   *
   *
   * @return number of sensors
   */
  static constexpr std::size_t SensorsSize() noexcept
  {
    static_assert(SENSORS > 0);
    return SENSORS;
  }

  /**
   * \brief Get number of data entries
   *
   *
   * @return number of data entries each sensor has
   */
  std::size_t DataSize() const noexcept
  {
    return !data_.empty() ? data_[0].size() : 0;
  }

  /**
   * \brief Access sensor data
   *
   * @param num - sensor number. Valid values from 0 to @ref SensorsSize-1
   *
   * @return number of data entries each sensor has
   */
  std::vector<float>& operator[](const std::size_t num) noexcept
  {
    return data_[num];
  }

  /// @overload
  const Value& operator[](const std::size_t index) const noexcept
  {
    return data_[index];
  }

  CONTAINER& data()
  {
    return data_;
  }

  void reserve(const std::size_t num)
  {
    for (std::size_t i = 0; i < SENSORS; i++)
      data_[i].reserve(num);
  }

  void resize(const std::size_t new_size)
  {
    for (std::size_t i = 0; i < SENSORS; i++)
      data_[i].resize(new_size);
  }

  void clear() noexcept
  {
    for (std::size_t i = 0; i < SENSORS; i++)
      data_[i].clear();
  }

  bool empty() const noexcept
  {
    return DataSize() == 0;
  }

  void append(const SensorsData& other)
  {
    append(other, other.DataSize());
  }

  void append(const SensorsData& other, const std::size_t count)
  {
    for (std::size_t i = 0; i < SENSORS; ++i) {
      const auto in_size = std::min<std::size_t>(other.data_[i].size(), count);
      const auto out_offset = data_[i].size();
      data_[i].resize(data_[i].size() + in_size);
      const auto b = other.data_[i].begin();
      std::copy(b, b + in_size, data_[i].begin() + out_offset);
    }
  }

  void erase_front(const std::size_t count) noexcept
  {
    for (std::size_t i = 0; i < SENSORS; i++)
      data_[i].erase(data_[i].begin(), data_[i].begin() + count);
  }

  void erase_back(const std::size_t count) noexcept
  {
    for (std::size_t i = 0; i < SENSORS; i++)
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
  CONTAINER data_;
};

#endif  // PANDA_TIMESWIPE_DRIVER_SENSOR_DATA_HPP
