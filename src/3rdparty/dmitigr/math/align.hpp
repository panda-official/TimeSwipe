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

#ifndef DMITIGR_MATH_ALIGN_HPP
#define DMITIGR_MATH_ALIGN_HPP

#include "exceptions.hpp"

namespace dmitigr::math {

/// @returns `true` if `number` is a power of 2.
template<typename T>
constexpr bool is_power_of_two(const T number) noexcept
{
  return (number & (number - 1)) == 0;
}

/**
 * @returns The size of padding.
 *
 * @param value A value for which a padding need to be calculated.
 * @param alignment An aligment to calculated the padding.
 *
 * @par Requires
 * `(size >= 0 && is_power_of_two(alignment))`.
 */
template<typename T>
constexpr auto padding(const T value, const T alignment)
{
  if (!(value >= 0))
    throw Exception{"cannot calculate padding for a negative value"};
  else if (!is_power_of_two(alignment))
    throw Exception{"cannot calculate padding with alignment that is not power of 2"};
  return (static_cast<T>(0) - value) & static_cast<T>(alignment - 1);
}

/**
 * @returns The value aligned by using `alignment`.
 *
 * @par Requires
 * `(value >= 0 && is_power_of_two(alignment))`.
 */
template<typename T>
constexpr T aligned(const T value, const T alignment)
{
  if (!(value >= 0))
    throw Exception{"cannot align a negative value"};
  else if (!is_power_of_two(alignment))
    throw Exception{"cannot align a value with alignment that is not power of 2"};
  return (value + (alignment - 1)) & -alignment;
}

} // namespace dmitigr::math

#endif  // DMITIGR_MATH_ALIGN_HPP
