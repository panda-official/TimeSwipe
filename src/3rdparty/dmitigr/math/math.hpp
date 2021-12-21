// -*- C++ -*-
// Copyright (C) 2021 Dmitry Igrishin
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

#ifndef DMITIGR_MATH_MATH_HPP
#define DMITIGR_MATH_MATH_HPP

#include "exceptions.hpp"
#include "version.hpp"
#include "../error/assert.hpp"

#include <utility>

namespace dmitigr::math {

/// Represents a type of interval.
enum class Interval_type {
  /// Denotes [min, max] interval.
  closed,
  /// Denotes (min, max) interval.
  open,
  /// Denotes (min, max] interval.
  lopen,
  /// Denotes [min, max) interval.
  ropen
};

/// Represents an interval.
template<typename T>
class Interval final {
public:
  /// An alias of T.
  using Value_type = T;

  /// An alias of Interval_type.
  using Type = Interval_type;

  /// Constructs closed [{},{}] interval.
  Interval() noexcept = default;

  /**
   * @brief Constructs closed [min, max] interval.
   *
   * @par Requires
   * `min <= max`.
   */
  explicit Interval(T min, T max)
    : type_{Type::closed}
    , min_{std::move(min)}
    , max_{std::move(max)}
  {
    if (!(min_ <= max_))
      throw Exception{"interval is invalid (min > max)"};
  }

  /**
   * @brief Constructs the interval of the specified type.
   *
   * @par Requires
   * `(type == Type::closed && min <= max) || (type != Type::closed && min < max)`.
   */
  explicit Interval(const Type type, T min, T max)
    : type_{type}
    , min_{std::move(min)}
    , max_{std::move(max)}
  {
    const bool requirement = (type_ == Type::closed && min_ <= max_) ||
      (type_ != Type::closed && min_ < max_);
    if (!requirement)
      throw Exception{"interval is invalid (min > max or min >= max)"};
  }

  /// @returns [min, max] interval.
  static Interval make_closed(T min, T max)
  {
    return {Type::closed, std::move(min), std::move(max)};
  }

  /// @returns (min, max) interval.
  static Interval make_open(T min, T max)
  {
    return {Type::open, std::move(min), std::move(max)};
  }

  /// @returns (min, max] interval.
  static Interval make_lopen(T min, T max)
  {
    return {Type::lopen, std::move(min), std::move(max)};
  }

  /// @returns [min, max) interval.
  static Interval make_ropen(T min, T max)
  {
    return {Type::ropen, std::move(min), std::move(max)};
  }

  /// @returns The type of interval.
  Type type() const noexcept
  {
    return type_;
  }

  /// @returns The minimum of interval.
  const T& min() const noexcept
  {
    return min_;
  }

  /// @returns The maximum of interval.
  const T& max() const noexcept
  {
    return max_;
  }

  /// @returns `true` if value belongs to interval, or `false` otherwise.
  bool has(const T& value) const noexcept
  {
    switch (type_) {
    case Type::closed: return (min_ <= value) && (value <= max_); // []
    case Type::open:   return (min_ <  value) && (value <  max_); // ()
    case Type::lopen:  return (min_ <  value) && (value <= max_); // (]
    case Type::ropen:  return (min_ <= value) && (value <  max_); // [)
    }
    DMITIGR_ASSERT(false);
  }

  /**
   * @returns A pair of {min, max}.
   *
   * @par Effects
   * The state of this instance as if it default constructed.
   */
  std::pair<T, T> release() noexcept
  {
    std::pair<T, T> result{std::move(min_), std::move(max_)};
    *this = {};
    return result;
  }

private:
  Type type_{Type::closed};
  T min_{};
  T max_{};
};

/**
 * @returns An average of values.
 *
 * @param data Input data
 */
template<class Container>
constexpr double avg(const Container& data)
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
constexpr double variance(const Container& data, const double avg, const bool general = true)
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

#endif  // DMITIGR_MATH_MATH_HPP
