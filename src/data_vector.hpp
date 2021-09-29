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

#ifndef PANDA_TIMESWIPE_DATA_VECTOR_HPP
#define PANDA_TIMESWIPE_DATA_VECTOR_HPP

#include "basics.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace panda::timeswipe {

/// Data vector.
class Data_vector final {
public:
  /// Alias of the value type.
  using value_type = std::vector<float>;

  /// Alias the size type.
  using size_type = value_type::size_type;

  /// The constructor.
  explicit Data_vector(size_type channel_count = max_channel_count);

  /// @returns The number of channels whose data this vector contains.
  size_type channel_count() const noexcept
  {
    return channel_count_;
  }

  /// @returns The number of values per sensor.
  size_type size() const noexcept
  {
    return !data_.empty() ? data_[0].size() : 0;
  }

  /// @returns The reference to the value at the given `index`.
  const value_type& operator[](const size_type index) const noexcept
  {
    return data_[index];
  }

  /// @overload
  value_type& operator[](const size_type index) noexcept
  {
    return const_cast<value_type&>(static_cast<const Data_vector*>(this)->operator[](index));
  }

  /// Reserves the memory for given `size`.
  void reserve(const size_type size)
  {
    const auto cc = channel_count();
    for (size_type i{}; i < cc; ++i)
      data_[i].reserve(size);
  }

  /// Resizes this vector to the given `size`.
  void resize(const size_type size)
  {
    const auto cc = channel_count();
    for (size_type i{}; i < cc; ++i)
      data_[i].resize(size);
  }

  /// Clears the vector.
  void clear() noexcept
  {
    const auto cc = channel_count();
    for (size_type i{}; i < cc; ++i)
      data_[i].clear();
  }

  /// @returns `true` if the vector is empty.
  bool is_empty() const noexcept
  {
    return !size();
  }

  /// For STL compatibility.
  bool empty() const noexcept
  {
    return is_empty();
  }

  /// Appends `other` to the end of this vector.
  void append(const Data_vector& other)
  {
    append(other, other.size());
  }

  /// Appends no more than `count` elements of `other` to the end of this vector.
  void append(const Data_vector& other, const size_type count)
  {
    const auto cc = channel_count();
    for (size_type i{}; i < cc; ++i) {
      const auto in_size = std::min(other.data_[i].size(), count);
      const auto out_offset = data_[i].size();
      data_[i].resize(data_[i].size() + in_size);
      const auto b = other.data_[i].begin();
      std::copy(b, b + in_size, data_[i].begin() + out_offset);
    }
  }

  /// Removes the given `count` of elements from the begin of this vector.
  void erase_front(const size_type count) noexcept
  {
    const auto cc = channel_count();
    for (size_type i{}; i < cc; i++)
      data_[i].erase(data_[i].begin(), data_[i].begin() + count);
  }

  /// Removes the given `count` of elements from the end of this vector.
  void erase_back(const size_type count) noexcept
  {
    const auto cc = channel_count();
    for (size_type i{}; i < cc; i++)
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
  using Array = std::array<std::vector<float>, max_channel_count>;
  Array data_;
  size_type channel_count_{};
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_DATA_VECTOR_HPP
