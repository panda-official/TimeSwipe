// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH
*/

/*
 * Designed by Christian Vorwerk
 * Implemented by Dmitry Igrishin
 */

#ifndef PANDA_TIMESWIPE_IIR_FILTER_HPP
#define PANDA_TIMESWIPE_IIR_FILTER_HPP

#include "debug.hpp"
#include "driver.hpp"
#include "exceptions.hpp"

#include <array>
#include <cmath>
#include <optional>

namespace panda::timeswipe::detail {

/**
 * @brief IIR Butterworth lowpass filter of 9th order.
 *
 * @details There are `(9 - 3)/2 == 3` digital filter stages.
 *
 * @remarks The order could be also 15th, 21st, 27th, ..., so digital filter
 * stages must be changed accordingly: `(order - 3)/2`. However, the gain at
 * filter levels higher than 9 is marginal.
 */
class Iir_filter final {
public:
  /**
   * @brief The constructor.
   *
   * @param input_freq The input signal frequency.
   * @param target_freq The target signal frequency (i.e. the signal frequency
   * after assumed downsampling with FIR for example).
   * @param cutoff_freq Cutoff frequency. Value of `std::nullopt` means the
   * default value of `0.25*(target_freq/Driver::instance().max_sample_rate())`.
   *
   * @par Requires
   *   - `input_freq ∋ (0, Driver::instance().max_sample_rate()]`;
   *   - `target_freq ∋ (0, Driver::instance().max_sample_rate()]`;
   *   - `target_freq <= input_freq`;
   *   - `cutoff_freq ∋ (0, 1]`.
   */
  Iir_filter(const int input_freq, const int target_freq,
    std::optional<double> cutoff_freq = {})
  {
    if (!(0 < input_freq && input_freq <= Driver::instance().max_sample_rate()))
      throw Exception{"invalid input signal frequency"};
    else if (!(0 < target_freq && target_freq <= Driver::instance().max_sample_rate()) ||
      target_freq > input_freq)
      throw Exception{"invalid target signal frequency"};

    cutoff_freq = cutoff_freq.value_or(.25*(target_freq /
        Driver::instance().max_sample_rate()));
    if (!(0 < cutoff_freq && cutoff_freq <= 1))
      throw Exception{"invalid cutoff frequency"};

    // Fill A2, B0.
    using std::pow;
    const std::array<double, stage_count_> a{
      1.19841413E-28*pow(input_freq, 6) -
      1.61060384E-23*pow(input_freq, 5) +
      7.20266402E-19*pow(input_freq, 4) -
      1.50956077E-14*pow(input_freq, 3) +
      1.47689120E-10*pow(input_freq, 2) -
      5.73478009E-7*input_freq + 5.18160418E-1,

      3.72308476E-29*pow(input_freq, 6) -
      9.98717751E-26*pow(input_freq, 5) -
      1.45701174E-19*pow(input_freq, 4) +
      5.61291367E-15*pow(input_freq, 3) -
      7.58842119E-11*pow(input_freq, 2) +
      3.64468431E-7*input_freq + 1.41382592E+0,

      1.57072265E-28*pow(input_freq, 6) -
      1.62059107E-23*pow(input_freq, 5) +
      5.74565265E-19*pow(input_freq, 4) -
      9.48269496E-15*pow(input_freq, 3) +
      7.18049206E-11*pow(input_freq, 2) -
      2.09009637E-7*input_freq + 1.93198634E+0
    };
    PANDA_TIMESWIPE_ASSERT(a[0] > 0);
    PANDA_TIMESWIPE_ASSERT(a[1] > 0);
    PANDA_TIMESWIPE_ASSERT(a[2] > 0);

    const double b{
      1.50792796E-28*pow(input_freq, 6) -
      1.44583225E-23*pow(input_freq, 5) +
      4.74577304E-19*pow(input_freq, 4) -
      7.02691571E-15*pow(input_freq, 3) +
      4.49174026E-11*pow(input_freq, 2) -
      9.53543220E-8*input_freq + 1.00002398E+0
    };
    PANDA_TIMESWIPE_ASSERT(b > 0);

    PANDA_TIMESWIPE_ASSERT(a.size() == b0_.size());
    PANDA_TIMESWIPE_ASSERT(b0_.size() == a2_.size());
    const double trans{1.0 / std::tan(M_PI * *cutoff_freq)};
    const auto b_trans_trans = b*trans*trans;
    for (int i{}; i < stage_count_; ++i) {
      const auto a_trans = a[i]*trans;
      b0_[i] = 1 / (1 + a_trans + b_trans_trans);
      a2_[i] = -b0_[i] * (1 - a_trans + b_trans_trans);
    }

    // Fill offsets.
    for (int i{}; i < stage_count_; ++i) {
      curr_offsets_[i] = stage_count_*i;
      next_offsets_[i] = curr_offsets_[i] + stage_count_;
    }
  }

  /// @returns Filtered `value`.
  double apply(const double value) noexcept
  {
    s_[0] %= stage_count_; // new "pointer" to `value`
    s_[1] = (s_[0] + 1) % stage_count_; // old "pointer" to `value`
    s_[2] = (s_[1] + 1) % stage_count_; // last "pointer" to `value`
    data_[s_[0]] = value;
    for (int i{}; i < stage_count_; ++i) {
      /*
       * Basis is: Y0 = A1*Y1 + A2*Y2 + B0*(X0 + 2*X1 + X2)
       * But this becomes dangerous at extreme conditions (e.g. Tast = 240kHz,
       * Hol=0.01Hz). Therefore its better to use:
       * Y0 = Y1 + A2*(Y2 - Y1) + B0 * (X0 + 2*X1 + X2 - 4*Y1)
       * which always results in Aout=Ain regardless of rounding errors.
       * This is possible because of relationship: A1 = 1 - A2 - 4*B0.
       */
      const auto cur = curr_offsets_[i];
      const auto nxt = next_offsets_[i];
      const auto x_cur_new  = cur + s_[0];
      const auto x_cur_old  = cur + s_[1];
      const auto x_cur_last = cur + s_[2];
      const auto x_nxt_new  = nxt + s_[0];
      const auto x_nxt_old  = nxt + s_[1];
      const auto x_nxt_last = nxt + s_[2];
      const auto y1 = data_[x_nxt_last];
      const auto y2 = data_[x_nxt_old];
      const auto x0 = data_[x_cur_new];
      const auto x1 = data_[x_cur_last];
      const auto x2 = data_[x_cur_old];
      const auto a2 = a2_[i];
      const auto b0 = b0_[i];
      data_[x_nxt_new] = y1 + a2*(y2 - y1) + b0*(x0 + 2*x1 + x2 - 4*y1);
    }

    if (idling_call_count_ < stage_count_) {
      ++idling_call_count_;
      for (int i{}; i < stage_count_; ++i)
        data_[next_offsets_[i] + s_[0]] = 0;
      s_[0]++;
      return 0; // y0 = 0 for first (stage_count_ - 1) calls
    } else
      return data_[3*stage_count_ + s_[0]++]; // y0 = data_[x_nxt_new]
  }

  /**
   * @returns apply(value).
   *
   * @see apply().
   */
  double operator()(const double value) noexcept
  {
    return apply(value);
  }

private:
  constexpr static int stage_count_{3};
  constexpr static int offset_count_{8}; // pow(stage_count_, stage_count_-1)-1
  int idling_call_count_{1};
  std::array<int, stage_count_> s_{};
  std::array<double, stage_count_> b0_{};
  std::array<double, stage_count_> a2_{};
  std::array<unsigned, stage_count_> curr_offsets_{};
  std::array<unsigned, stage_count_> next_offsets_{};
  std::array<double, stage_count_*(offset_count_ + 1)> data_{};
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_IIR_FILTER_HPP
