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

#include <vector>

namespace panda::timeswipe {

/// Data vector.
class Data_vector final {
public:
  /// Alias of the value type.
  using value_type = std::vector<float>;

  /// Alias the size type.
  using size_type = value_type::size_type;

  /// The default constructor. Constructs instance with zero channel count.
  Data_vector() = default;

  /// The constructor.
  explicit Data_vector(size_type channel_count);

  /// @returns The number of channels whose data this vector contains.
  size_type channel_count() const noexcept
  {
    return channels_.size();
  }

  /**
   * @returns channel_count().
   *
   * @remarks This method for STL compatibility.
   */
  auto size() const noexcept
  {
    return channel_count();
  }

  /// @returns `true` if the vector is empty.
  bool is_empty() const noexcept
  {
    return !channel_count();
  }

  /**
   * @returns is_empty().
   *
   * @remarks This method for STL compatibility.
   */
  auto empty() const noexcept
  {
    return is_empty();
  }

  /**
   * @returns The reference to the channel at the given `index`.
   *
   * @par Requires
   * `index` in range `[0, channel_count())`.
   */
  const value_type& operator[](size_type index) const;

  /// @overload
  value_type& operator[](const size_type index)
  {
    return const_cast<value_type&>(static_cast<const Data_vector*>(this)->operator[](index));
  }

  /**
   * Appends no more than `count` elements of `other` to the end of this vector.
   *
   * @par Requires
   * `(!channel_count() || (channel_count() == other.channel_count()))`.
   *
   * @par Effects
   * `(channel_count() == other.channel_count())`.
   */
  void append(const Data_vector& other, size_type count);

  /// Appends `other` to the end of this vector.
  void append(const Data_vector& other)
  {
    append(other, other.size());
  }

  /**
   * Removes `std::min(count, (*this)[i].size())` elements from the begin
   * of each channel.
   */
  void erase_front(const size_type count) noexcept;

  /**
   * Removes `std::min(count, (*this)[i].size())` elements from the end
   * of each channel.
   */
  void erase_back(const size_type count) noexcept;

  /// Reserves the memory for all the channels by the given `size`.
  void reserve(size_type size);

  /// Resizes all the channels by the given `size`.
  void resize(size_type size);

  /// Clears all the channels.
  void clear() noexcept;

  /// @name Iterators
  /// @{

  /// @returns Iterator that points to a first channel.
  auto begin() noexcept
  {
    return channels_.begin();
  }

  /// @returns Constant iterator that points to a first channel.
  auto begin() const noexcept
  {
    return channels_.begin();
  }

  /// @returns Constant iterator that points to a first channel.
  auto cbegin() const noexcept
  {
    return channels_.cbegin();
  }

  /// @returns Iterator that points to an one-past-the-last channel.
  auto end() noexcept
  {
    return channels_.end();
  }

  /// @returns Constant iterator that points to an one-past-the-last channel.
  auto end() const noexcept
  {
    return channels_.end();
  }

  /// @returns Constant iterator that points to an one-past-the-last channel.
  auto cend() const noexcept
  {
    return channels_.cend();
  }

  /// @}

private:
  std::vector<std::vector<float>> channels_;
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_DATA_VECTOR_HPP
