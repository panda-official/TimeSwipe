// -*- C++ -*-
// Copyright (C) 2022 Dmitry Igrishin
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Dmitry Igrishin
// dmitigr@gmail.com

#ifndef DMITIGR_MATH_STAT_HPP
#define DMITIGR_MATH_STAT_HPP

#include "../base/assert.hpp"

namespace dmitigr::math {

/**
 * @returns An average of values.
 *
 * @param data Input data
 */
template<class Container>
constexpr double avg(const Container& data) noexcept
{
  double result{};
  const auto data_size = data.size();
  for (const double num : data)
    result += (num / static_cast<double>(data_size));
  return result;
}

/**
 * @returns A variance of values.
 *
 * @param data Input data.
 * @param avg An average of `data`.
 * @param general Is the `data` represents general population?
 */
template<class Container>
constexpr double variance(const Container& data, const double avg,
  const bool general = true) noexcept
{
  const double den = data.size() - !general;
  double result{};
  for (const double num : data) {
    const double d = num - avg;
    result += (d / den) * d; // (d * d) / den
  }
  DMITIGR_ASSERT(result >= 0);
  return result;
}

/// @overload
template<class Container>
constexpr double variance(const Container& data, const bool general = true) noexcept
{
  return variance(data, avg(data), general);
}

} // namespace dmitigr::math

#endif  // DMITIGR_MATH_STAT_HPP
