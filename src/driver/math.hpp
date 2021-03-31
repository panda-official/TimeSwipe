// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#ifndef PANDA_TIMESWIPE_DRIVER_MATH_HPP
#define PANDA_TIMESWIPE_DRIVER_MATH_HPP

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

/// @returns A positive value near to zero.
constexpr double positive_near_zero() noexcept
{
  return std::pow(10, 1 - std::numeric_limits<double>::max_digits10);
}

/// @returns `(value * value)`.
template<typename T>
constexpr auto square(T value) noexcept
{
  return value * value;
}

/// @returns Quotient ceil of `num1` and `num2`.
template<typename T, typename U>
constexpr auto quotientCeil(const T num1, const U num2) noexcept
{
  return num1 / num2 + (num1 % num2) % 2;
}

/// @returns `sin(π * x) / (π * x)`. Returns `1` if `(|x| < positive_near_zero())`.
inline double sinc(const double x) noexcept
{
  const auto a = M_PI * x;
  return std::fabs(x) < positive_near_zero() ? 1 : std::sin(a) / a;
}

/**
 * Calculates least-square linear-phase finite impulse response (FIR) filter.
 *
 * This filter minimizes the weighted, integrated squared error between an ideal
 * piecewise linear function and the magnitude response of the filter over a set
 * of desired frequency bands.
 *
 * @param order Order of the filter. For odd orders, the frequency response at
 * the Nyquist frequency is necessarily `0`. For this reason, `firls()` is always
 * uses an even filter order for configurations with a passband at the Nyquist
 * frequency.
 *
 * @param freq Pairs of frequency band edges in ascending order in range `[0, 1]`,
 * where `1` corresponds to the Nyquist frequency (or half the sampling frequency).
 * Duplicates frequencies can be used to design Window-based filters.
 *
 * @param ampl Amplitude values of the function at each frequency point. This vector
 * specifies the desired amplitude of the frequency response of the result. The size
 * of this vector must be even and equals to the size of `freq`.
 * The desired amplitude at frequencies between pairs of points `(f(k), f(k+1))`:
 *   - for `k` odd, is the line segment connecting the points `(f(k), a(k))` and
 * `(f(k+1), a(k+1))`;
 *   - for `k` even is unspecified. (These are "transition bands" or "don't care"
 *   regions.)
 *
 * @returns The vector containing the `(order + 1)` coefficients of the order-`order`
 * FIR filter in inreasing order. This filter has frequency-amplitude characteristics
 * approximately matching those given by vectors `freq` and `ampl` in the least squares
 * sense. The type of returned filter is depends on the value of `order`: if even, the
 * filter of Type 2 returned, otherwise, the filter of Type 1 returned.
 *
 * @remarks If the specified `order` is odd and `(freq.back() == 1 && ampl.back() != 0)`,
 * then the vector or size `(order + 2)` will be returned since `firls()` is always
 * uses an even filter order, and thus, initial value of `order` is `(order + 1)`.
 *
 * @throws `std::runtime_error` if `freq` doesn't represents fullband.
 */
inline std::vector<double> firls(const std::size_t order, std::vector<double> freq, const std::vector<double>& ampl)
{
  // Initial preconditions.
  assert(order > 0);
  assert(!freq.empty());
  assert(!ampl.empty());
  assert(*min_element(cbegin(freq), cend(freq)) >= 0);
  assert(*max_element(cbegin(freq), cend(freq)) <= 1);
  const auto freq_size = freq.size();
  assert(freq_size == ampl.size());
  assert(!(freq_size % 2));

  /*
   * Weights to weigh the fit for each frequency band, specified as a vector
   * of size half the size of `freq` and `ampl`, so there's exactly one weight
   * per band. `weights` indicates how much emphasis to put on minimizing the
   * integral squared error in each band, relative to the other bands.
   * Note: don't forget to uncomment for_each below if/when `weights` becomes
   * the parameter!
   */
  const std::vector<double> weights(freq_size / 2, 1);
  // for_each(begin(weights), end(weights), [](auto& e){e=std::sqrt(e);});
  assert(*min_element(cbegin(weights), cend(weights)) >= 0);

  // Increase the order if necessary.
  const auto order_increment = (
    (.999999 < freq.back() && freq.back() <= 1) &&        // == 1
    !(-.000001 < ampl.back() && ampl.back() < .000001) && // != 0
    (order % 2)) ? 1 : 0;
  const auto filter_length = order + 1 + order_increment;

  // Modify the frequences.
  for_each(begin(freq), end(freq), [](auto& e){e/=2;});

  // Determine fullband.
  const bool is_fullband = [freq, freq_size]() mutable
  {
    const auto b = begin(freq);
    const auto e = end(freq);
    adjacent_difference(b, e, b);
    assert(none_of(b + 1, e, [](const auto e){return e < 0;}));

    const bool result = freq_size > 2;
    for (auto i = b + 2; i != e; i += 2) {
      if (*i != 0)
        return false;
    }
    return result;
  }();

  // Determine the weights constancy.
  const bool is_constant_weights = [&weights]
  {
    const auto w = accumulate(cbegin(weights), cend(weights), double(0),
      [w0 = weights[0]](const auto a, const auto e)
      {
        return a + e - w0;
      });
    return w >= 0 && w <= .000001;
  }();

  // Final preconditions.
  if (!is_fullband)
    throw std::runtime_error{"frequences must represents fullband"};
  else if (!is_constant_weights)
    throw std::runtime_error{"weights must be constant"};

  // Find the order.
  const auto k_size = (filter_length - 1)/2 + 1;

  // Is filter length odd?
  const auto odd = filter_length % 2;

  // Basis vectors are cos(2*pi*m*f).
  std::vector<double> k(k_size);
  generate(begin(k), end(k), [i = 0*k_size, odd]() mutable
  {
    return i++ + .5 * !odd; // (i + 0) - for type 1, (i + .5) - for type 2
  });

  // B-vector.
  std::vector<double> b(k_size);
  for (auto i = 0*freq_size; i < freq_size; i += 2) {
    const auto aa = ampl[i+1], a = ampl[i];
    const auto ff = freq[i+1], f = freq[i];
    const auto slope = (aa - a) / (ff - f);
    const auto b1 = a - slope * f; // y-intercept
    const auto asw = std::fabs(square(weights[(i+1)/2]));

    // If `odd` b[0] must be calculated separately, since `(k[0] == 0)`.
    if (odd)
      b[0] += (b1*(ff - f) + slope/2 * (square(ff) - square(f))) * asw;
    for (auto j = 0*k_size + odd; j < k_size; ++j) {
      const auto kj2 = k[j] * 2;
      const auto kj2pi = kj2 * M_PI;
      using std::cos;
      b[j] += (slope/(4*square(M_PI))*(cos(kj2pi*ff) - cos(kj2pi*f)) / square(k[j])) * asw
        + (ff*(slope*ff + b1)*sinc(kj2*ff) - f*(slope*f + b1)*sinc(kj2*f)) * asw;
    }
  }

  // A-vector.
  std::vector<double> a(k_size);
  transform(cbegin(b), cend(b), begin(a),
    [sw0 = square(weights[0])](const auto v){return sw0 * 4 * v;});

  // Result vector.
  std::vector<double> r(2 * k_size - odd);
  static const auto div2 = [](const auto v){return v/2;};
  const auto m = transform(crbegin(a), crend(a), begin(r), div2);
  transform(cbegin(a) + odd, cend(a), m, div2);

  // Postconditions.
  assert(r.size() == filter_length);
  return r;
}

/**
 * @brief Calculates Kaiser Window.
 *
 * @param length Window length.
 * @param beta Shape factor. Must be positive. This parameter affects the
 * sidelobe attenuation of the Fourier transform of the window.
 *
 * @par Requires
 * `(length > 1 && beta >= 0)`.
 *
 * @returns A vector of filter coefficients of a length-point Kaiser window
 * with a shape factor `beta`.
 */
inline std::vector<double> kaiser(const int length, const double beta = .5)
{
  assert(length > 1);
  assert(beta >= 0);
  std::vector<double> result(length);
  generate(begin(result), end(result), [i = 0]() mutable { return i++; });
  transform(cbegin(result), cend(result), begin(result),
    [n = length - 1, beta, d = std::cyl_bessel_i(0, beta)](const double x)
  {
    const double a = 2 * beta/n * std::sqrt(x * (n - x));
    return std::cyl_bessel_i(0, a) / d;
  });
  return result;
}

#endif  // PANDA_TIMESWIPE_DRIVER_MATH_HPP
