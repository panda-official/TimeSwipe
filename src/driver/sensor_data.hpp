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
  std::size_t DataSize() const noexcept;

  /**
   * \brief Access sensor data
   *
   * @param num - sensor number. Valid values from 0 to @ref SensorsSize-1
   *
   * @return number of data entries each sensor has
   */
  std::vector<float>& operator[](std::size_t num) noexcept;

  /// @overload
  const Value& operator[](const std::size_t index) const noexcept
  {
    return data_[index];
  }

  CONTAINER& data();
  void reserve(std::size_t num);
  void resize(std::size_t new_size);
  void clear() noexcept;
  bool empty() const noexcept;
  void append(const SensorsData& other);
  void append(const SensorsData& other, std::size_t count);
  void erase_front(std::size_t count) noexcept;
  void erase_back(std::size_t count) noexcept;

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
