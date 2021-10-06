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

#include "data_vector.hpp"
#include "error_detail.hpp"

#include <algorithm>

namespace panda::timeswipe {

Data_vector::Data_vector(const size_type channel_count)
  : channels_(channel_count)
{}

auto Data_vector::operator[](const size_type index) const -> const value_type&
{
  if (!(index < channel_count()))
    throw detail::Generic_exception{"cannot get channel by invalid index"};

  return channels_[index];
}

void Data_vector::append(const Data_vector& other, const size_type count)
{
  if (!channel_count()) {
    channels_ = {}; // prevent UB if instance was moved
    channels_.resize(other.channel_count());
  } else if (!(channel_count() == other.channel_count()))
    throw detail::Generic_exception{"cannot append data vector"
      " with different channel count"};

  const size_type cc = channel_count();
  for (size_type i{}; i < cc; ++i) {
    const auto in_size = std::min(other.channels_[i].size(), count);
    const auto out_offset = channels_[i].size();
    channels_[i].resize(out_offset + in_size);
    const auto b = other.channels_[i].begin();
    std::copy(b, b + in_size, channels_[i].begin() + out_offset);
  }
}

void Data_vector::erase_front(const size_type count) noexcept
{
  const size_type cc = channel_count();
  for (size_type i{}; i < cc; i++) {
    const auto num = std::min(count, channels_[i].size());
    channels_[i].erase(channels_[i].begin(), channels_[i].begin() + num);
  }
}

void Data_vector::erase_back(const size_type count) noexcept
{
  const size_type cc = channel_count();
  for (size_type i{}; i < cc; i++) {
    const auto num = std::min(count, channels_[i].size());
    channels_[i].resize(channels_[i].size() - num);
  }
}

void Data_vector::reserve(const size_type size)
{
  const size_type cc = channel_count();
  for (size_type i{}; i < cc; ++i)
    channels_[i].reserve(size);
}

void Data_vector::resize(const size_type size)
{
  const size_type cc = channel_count();
  for (size_type i{}; i < cc; ++i)
    channels_[i].resize(size);
}

void Data_vector::clear() noexcept
{
  const size_type cc = channel_count();
  for (size_type i{}; i < cc; ++i)
    channels_[i].clear();
}

} // namespace panda::timeswipe
