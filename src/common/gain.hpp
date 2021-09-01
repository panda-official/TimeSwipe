// -*- C++ -*-

// PANDA TimeSwipe Project
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

#ifndef PANDA_TIMESWIPE_COMMON_GAIN_HPP
#define PANDA_TIMESWIPE_COMMON_GAIN_HPP

#include <algorithm>
#include <array>

namespace panda::timeswipe {

/// Output gain table factor for even entries.
constexpr float ogain_table_factor{1.375};

/// Output gain table.
constexpr std::array<float, 22> ogain_table{
  1,
  1*ogain_table_factor,
  2,
  2*ogain_table_factor,
  4,
  4*ogain_table_factor,
  8,
  8*ogain_table_factor,
  16,
  16*ogain_table_factor,
  32,
  32*ogain_table_factor,
  64,
  64*ogain_table_factor,
  128,
  128*ogain_table_factor,
  256,
  256*ogain_table_factor,
  512,
  512*ogain_table_factor,
  1024,
  1024*ogain_table_factor
};
static_assert(!(ogain_table.size() % 2));

/// @returns An index of `ogain_table` by `value`.
std::size_t get_ogain_table_index(const float value) noexcept
{
  const auto beg = cbegin(ogain_table);
  const auto end = cend(ogain_table);
  // Add .0001 to value to compensate possible inaccuracies upon comparing floats.
  const auto itr = std::find_if(beg, end, [val = value + .0001f](const auto threshold)
  {
    return val < threshold;
  });
  return std::max(0, itr - beg - 1);
}

} // namespace panda::timeswipe

/// FIXME: remove after placing the entire code base in the namespace
using namespace panda::timeswipe;

#endif  // PANDA_TIMESWIPE_COMMON_GAIN_HPP
