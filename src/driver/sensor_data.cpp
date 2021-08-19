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

#include "sensor_data.hpp"

std::vector<float>& SensorsData::operator[](std::size_t num) noexcept
{
  return data_[num];
}

std::size_t SensorsData::DataSize() const noexcept
{
  return !data_.empty() ? data_[0].size() : 0;
}

auto SensorsData::data() -> CONTAINER&
{
  return data_;
}

void SensorsData::reserve(std::size_t num)
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].reserve(num);
}

void SensorsData::resize(const std::size_t new_size)
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].resize(new_size);
}

void SensorsData::clear() noexcept
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].clear();
}

bool SensorsData::empty() const noexcept
{
  return DataSize() == 0;
}

void SensorsData::append(const SensorsData& other)
{
  append(other, other.DataSize());
}

void SensorsData::append(const SensorsData& other, const std::size_t count)
{
  for (std::size_t i = 0; i < SENSORS; ++i) {
    const auto in_size = std::min<std::size_t>(other.data_[i].size(), count);
    const auto out_offset = data_[i].size();
    data_[i].resize(data_[i].size() + in_size);
    const auto b = other.data_[i].begin();
    std::copy(b, b + in_size, data_[i].begin() + out_offset);
  }
}

void SensorsData::erase_front(const std::size_t count) noexcept
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].erase(data_[i].begin(), data_[i].begin() + count);
}

void SensorsData::erase_back(const std::size_t count) noexcept
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].resize(data_[i].size() - count);
}
