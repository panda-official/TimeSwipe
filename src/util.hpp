// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

/**
 * @file
 *
 * @remarks The code of this file is exception-free!
 */

#ifndef PANDA_TIMESWIPE_UTIL_HPP
#define PANDA_TIMESWIPE_UTIL_HPP

namespace panda::timeswipe::detail {

/**
 * @returns `low` if the `value` is less than `low`, `high` if it's less than
 * `value`, or `value` otherwise.
 */
template<typename T>
constexpr T clamp(const T value, const T low, const T high) noexcept
{
  return value < low ? low : value > high ? high : value;
}

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_UTIL_HPP
