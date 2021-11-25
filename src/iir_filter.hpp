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

#include "error.hpp"

#include <array>
#include <cmath>
#include <stdexcept>

namespace panda::timeswipe::detail {

/**
 * @brief IIR Butterworth filter of 9th order.
 *
 * @details There are `(9 - 3)/2 == 3` digital filter stages.
 *
 * @remarks It could be also 15th, 21st, 27th, ... order, so digital filter
 * stages must be changed accordingly: `(order - 3)/2`. However, the gain at
 * filter levels higher than 9 is marginal.
 */
class Iir_filter final {
public:
  /**
   * @brief The constructor.
   *
   * @param target_sample_rate Resulting sample rate after passing the values
   * calculated by apply() to the downsampler.
   * @param source_sample_rate Source sample rate.
   * @param cutoff Cutoff frequency at half the Nyquist frequency.
   */
  Iir_filter(const int target_sample_rate, const int source_sample_rate,
    const double cutoff_freq = .25)
  {
    if (target_sample_rate <= 0)
      throw std::runtime_error{"invalid target sample rate"};
    else if (source_sample_rate <= 0)
      throw std::runtime_error{"invalid source sample rate"};
    else if (target_sample_rate > source_sample_rate)
      throw std::runtime_error{"filtering for upsampling doesn't supported"};

    // Fill A2, B0.
    using std::pow;
    const std::array<double, stage_count_> a{
      1.19841413E-28*pow(target_sample_rate, 6) -
      1.61060384E-23*pow(target_sample_rate, 5) +
      7.20266402E-19*pow(target_sample_rate, 4) -
      1.50956077E-14*pow(target_sample_rate, 3) +
      1.47689120E-10*pow(target_sample_rate, 2) -
      5.73478009E-7*target_sample_rate + 5.18160418E-1,

      3.72308476E-29*pow(target_sample_rate, 6) -
      9.98717751E-26*pow(target_sample_rate, 5) -
      1.45701174E-19*pow(target_sample_rate, 4) +
      5.61291367E-15*pow(target_sample_rate, 3) -
      7.58842119E-11*pow(target_sample_rate, 2) +
      3.64468431E-7*target_sample_rate + 1.41382592E+0,

      1.57072265E-28*pow(target_sample_rate, 6) -
      1.62059107E-23*pow(target_sample_rate, 5) +
      5.74565265E-19*pow(target_sample_rate, 4) -
      9.48269496E-15*pow(target_sample_rate, 3) +
      7.18049206E-11*pow(target_sample_rate, 2) -
      2.09009637E-7*target_sample_rate + 1.93198634E+0
    };
    PANDA_TIMESWIPE_ASSERT(a[0] > 0);
    PANDA_TIMESWIPE_ASSERT(a[1] > 0);
    PANDA_TIMESWIPE_ASSERT(a[2] > 0);
    const double b{
      1.50792796E-28*pow(target_sample_rate, 6) -
      1.44583225E-23*pow(target_sample_rate, 5) +
      4.74577304E-19*pow(target_sample_rate, 4) -
      7.02691571E-15*pow(target_sample_rate, 3) +
      4.49174026E-11*pow(target_sample_rate, 2) -
      9.53543220E-8*target_sample_rate + 1.00002398E+0
    };
    PANDA_TIMESWIPE_ASSERT(b > 0);

    PANDA_TIMESWIPE_ASSERT(a.size() == b0_.size());
    PANDA_TIMESWIPE_ASSERT(b0_.size() == a2_.size());
    const double trans{1.0/std::tan(M_PI*cutoff_freq*target_sample_rate/source_sample_rate)};
    for (int i{}; i < stage_count_; ++i) {
      const auto abtrans = a[i]*trans + b*trans*trans;
      b0_[i] = 1 / (1 + abtrans);
      a2_[i] = -b0_[i] * (1 - abtrans);
    }

    // Fill offsets.
    for (int i{}; i < offset_count_; ++i) {
      current_offsets_[i] = 3*i;
      next_offsets_[i] = current_offsets_[i] + 3;
    }
  }

  /// @returns Filtered `value`.
  double apply(const double value) noexcept
  {
    s_[0] %= stage_count_; // "pointer" to `value`
    s_[1] = (s_[0] + 1) % stage_count_;
    s_[2] = (s_[1] + 1) % stage_count_;
    data_[s_[0]] = value;
    for (int i{}; i < stage_count_; ++i) {
      const auto nxt_offset = next_offsets_[i];
      const auto cur_offset = current_offsets_[i];
      data_[nxt_offset+s_[0]] = data_[nxt_offset+s_[2]] +
        a2_[i]*(data_[nxt_offset+s_[1]] - data_[nxt_offset+s_[2]]) +
        b0_[i]*(data_[cur_offset+s_[0]] + 2*data_[cur_offset+s_[2]] +
          data_[cur_offset+s_[1]] - 4*data_[nxt_offset+s_[2]]);
    }
    return data_[3*stage_count_ + s_[0]++];
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
  std::array<int, stage_count_> s_{};
  std::array<double, stage_count_> b0_{};
  std::array<double, stage_count_> a2_{};
  constexpr static int offset_count_{8}; // pow(stage_count_, stage_count_-1)-1
  std::array<unsigned, offset_count_> current_offsets_{};
  std::array<unsigned, offset_count_> next_offsets_{};
  std::array<double, stage_count_*(offset_count_ + 1)> data_{};
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_IIR_FILTER_HPP
