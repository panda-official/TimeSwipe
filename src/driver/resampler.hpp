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

#ifndef PANDA_TIMESWIPE_DRIVER_RESAMPLER_HPP
#define PANDA_TIMESWIPE_DRIVER_RESAMPLER_HPP

#include "fir_resampler.hpp"
#include "math.hpp"
#include "timeswipe.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>
#include <utility>

/// A timeswipe resampler options.
class TimeSwipeResamplerOptions final {
public:
  /**
   * @brief The constructor.
   *
   * @par Requires
   * `freq.size() == ampl.size()`.
   */
  TimeSwipeResamplerOptions(const unsigned up_factor = 1, const unsigned down_factor = 1,
    const SignalExtrapolation extrapolation = SignalExtrapolation::zero,
    const unsigned flength = 0,
    std::vector<double> freq = {}, std::vector<double> ampl = {})
    : up_factor_{up_factor}
    , down_factor_{down_factor}
    , extrapolation_{extrapolation}
  {
    up_factor__(up_factor);
    down_factor__(down_factor);
    filter_length__(flength);
    freq_ampl__(std::move(freq), std::move(ampl));
    assert(is_invariant_ok());
  }

  /**
   * Sets the up-factor.
   *
   * @returns *this.
   */
  TimeSwipeResamplerOptions& up_factor(const unsigned value) noexcept
  {
    up_factor__(value);
    assert(is_invariant_ok());
    return *this;
  }

  /// @returns The up-factor.
  unsigned up_factor() const noexcept
  {
    return up_factor_;
  }

  /// @returns The default up-factor.
  static unsigned default_up_factor() noexcept
  {
    return 1;
  }

  /**
   * Sets the down-factor.
   *
   * @returns *this.
   */
  TimeSwipeResamplerOptions& down_factor(const unsigned value) noexcept
  {
    down_factor__(value);
    assert(is_invariant_ok());
    return *this;
  }

  /// @returns The down-factor.
  unsigned down_factor() const noexcept
  {
    return down_factor_;
  }

  /// @returns The default down-factor.
  static unsigned default_down_factor() noexcept
  {
    return 1;
  }

  /**
   * Sets the signal extrapolation mode.
   *
   * @returns *this.
   */
  TimeSwipeResamplerOptions& extrapolation(const SignalExtrapolation value) noexcept
  {
    extrapolation_ = value;
    assert(is_invariant_ok());
    return *this;
  }

  /// @returns The signal extrapolation mode.
  SignalExtrapolation extrapolation() const noexcept
  {
    return extrapolation_;
  }

  /// @returns The default extrapolation mode.
  static SignalExtrapolation default_extrapolation() noexcept
  {
    return SignalExtrapolation::zero;
  }

  /**
   * @brief Sets the crop extra samples mode.
   *
   * The resampled sequence is always contains some extra samples at both the
   * begin and end. If this option is set to `true` these extra samples are
   * eliminated automatically.
   *
   * @returns *this.
   */
  TimeSwipeResamplerOptions& crop_extra(const bool value) noexcept
  {
    crop_extra_ = value;
    assert(is_invariant_ok());
    return *this;
  }

  /// @returns The crop extra samples mode.
  bool crop_extra() const noexcept
  {
    return crop_extra_;
  }

  /// @returns The default crop extra samples mode.
  static bool default_crop_extra() noexcept
  {
    return true;
  }

  /**
   * Sets the filter length.
   *
   * @returns *this.
   */
  TimeSwipeResamplerOptions& filter_length(const unsigned value) noexcept
  {
    filter_length__(value);
    assert(is_invariant_ok());
    return *this;
  }

  /// @returns The filter length.
  unsigned filter_length() const noexcept
  {
    return filter_length_;
  }

  /// @returns The default filter length.
  unsigned default_filter_length() const noexcept
  {
    return 2 * 10 * std::max(up_factor_, down_factor_) + 1;
  }

  /**
   * Sets the both pairs of frequency band edges and amplitude values.
   *
   * @param freq The pairs of frequency band edges. See firls().
   * @param ampl Amplitude values of the function at each frequency point. See firls().
   *
   * @returns *this.
   *
   * @see firls().
   */
  TimeSwipeResamplerOptions& freq_ampl(std::vector<double> freq, std::vector<double> ampl) noexcept
  {
    assert(freq.size() == ampl.size());
    freq_ampl__(std::move(freq), std::move(ampl));
    assert(is_invariant_ok());
    return *this;
  }

  /**
   * @returns The pairs of frequency band edges.
   *
   * @see firls().
   */
  const std::vector<double>& freq() const noexcept
  {
    return freq_;
  }

  /// @returns The default pairs of frequency band edges.
  std::vector<double> default_freq() const
  {
    /*
     * Note: when the band_numerator is 1 some of the default
     * firc values can be NaN, so band_numerator is .(9).
     */
    constexpr double band_numerator{1 - positive_near_zero()};
    const double band = band_numerator / up_factor_;
    return {0, band, band, 1};
  }

  /**
   * @returns Amplitude values of the function at each frequency point.
   *
   * @see firls().
   */
  const std::vector<double>& ampl() const noexcept
  {
    return ampl_;
  }

  /// @returns The default amplitude values of the function at each frequency point.
  static std::vector<double> default_ampl()
  {
    return {1, 1, 0, 0};
  }

private:
  unsigned up_factor_{};
  unsigned down_factor_{};
  SignalExtrapolation extrapolation_{SignalExtrapolation::zero};
  bool crop_extra_{true};
  unsigned filter_length_{};
  std::vector<double> freq_;
  std::vector<double> ampl_;

  bool is_invariant_ok() const noexcept
  {
    const bool factors_ok = up_factor_ > 0 && down_factor_ > 0;
    const bool length_ok = filter_length_ > 0;
    const bool vecs_ok = !freq_.empty() && !ampl_.empty() && (freq_.size() == ampl_.size());
    return factors_ok && length_ok && vecs_ok;
  }

  void up_factor__(const unsigned value) noexcept
  {
    up_factor_ = value ? value : default_up_factor();
  }

  void down_factor__(const unsigned value) noexcept
  {
    down_factor_ = value ? value : default_down_factor();
  }

  void filter_length__(const unsigned value) noexcept
  {
    filter_length_ = value ? value : default_filter_length();
  }

  void freq_ampl__(std::vector<double>&& freq, std::vector<double>&& ampl) noexcept
  {
    if (freq.empty()) {
      freq_ = default_freq();
      ampl_ = default_ampl();
    } else {
      freq_ = std::move(freq);
      ampl_ = std::move(ampl);
    }
  }
};

/**
 * @brief A TimeSwipe resampler.
 *
 * This class is the wrapper of the class FirResampler and provides the
 * stream-style API in order to resample the chunks of variable length. After
 * the resampling of the last chunk, the method flush() should be called in
 * order to resample and flush the extrapolated (extra) sequence of length of
 * one polyphase of filter.
 *
 * @remarks Both excess leading and excess trailing samples (which are actually
 * artifacts of the resampling) will be cropped automatically.
 *
 * @see apply(), flush(), FirResampler.
 */
class TimeSwipeResampler final {
public:
  /// An alias of TimeSwipeResamplerOptions.
  using Options = TimeSwipeResamplerOptions;

  /// The constructor.
  TimeSwipeResampler(Options options)
    : options_{std::move(options)}
  {
    // Calculate FIR coefficients.
    const auto firc = [this]
    {
      std::vector<double> firc = firls(options_.filter_length() - 1, options_.freq(), options_.ampl());
      assert(options_.filter_length() == firc.size());
      // print_firc(firc);

      /*
       * Apply Kaiser window.
       * We must find the most suitable shape factor (beta) possible before
       * applying Kaiser window.
       */
      const auto clog_prec = std::clog.precision();
      std::clog.precision(std::numeric_limits<double>::max_digits10);
      std::clog << "Guessing the most suitable shape factor for Kaiser window...";
      std::vector<double> result(firc.size());

      // Create convenient applicator of Kaiser window and adder of the result of application.
      const auto apply_kaiser_and_sum = [&firc, &result, u = options_.up_factor()](const double beta)
      {
        const auto window = kaiser(firc.size(), beta);
        assert(firc.size() == window.size());
        transform(cbegin(window), cend(window), cbegin(firc), begin(result),
          [u](const auto w, const auto c) { return u * w * c; });
        return accumulate(cbegin(result), cend(result), .0);
      };

      // Find initial delta required in order to find the most suitable shape factor.
      const double initial_beta{10};
      const auto [delta, initial_sum] = [initial_beta, &apply_kaiser_and_sum]
      {
        constexpr double delta{.01};
        const double suml{apply_kaiser_and_sum(initial_beta - delta)};
        const double sum{apply_kaiser_and_sum(initial_beta)};
        const double sumr{apply_kaiser_and_sum(initial_beta + delta)};
        const auto diffl = std::abs(sum - suml);
        const auto diffr = std::abs(sum - sumr);
        return std::make_pair(diffl < diffr ? -delta : delta, sum);
      }();

      // Find the most suitable shape factor step by step by delta (in-)decrementing.
      constexpr double inf{}, sup{20};
      const auto up_factor = options_.up_factor();
      for (double prev_beta{initial_beta}, prev_sum{initial_sum};;) {
        const double beta = prev_beta + delta;
        const double sum = apply_kaiser_and_sum(beta);
        // std::clog << "beta = " << beta  << ", sum = " << sum << std::endl;
        // break;
        if (std::abs(sum - up_factor) > std::abs(prev_sum - up_factor)) {
          apply_kaiser_and_sum(prev_beta);
          std::clog << prev_beta << "\n";
          break;
        } else if (!(beta > inf && beta < sup))
          throw std::runtime_error{"unable to guess shape factor for Kaiser window"
              " (probably, either up-factor or down-factor are exorbitant to handle)"};

        prev_beta = beta;
        prev_sum = sum;
      }
      std::clog.precision(clog_prec);
      return result;
    }();
    if (firc.size() > std::numeric_limits<int>::max())
      throw std::runtime_error{"too many FIR coefficients required"};
    else if (any_of(cbegin(firc), cend(firc), [](const auto v){ return std::isnan(v); }))
      throw std::runtime_error{"FIR coefficients contains NaN"};
    // print_firc(firc);

    // Initializing the underlying resamplers.
    for (auto& r : resamplers_)
      r = {options_.up_factor(), options_.down_factor(), cbegin(firc), cend(firc), options_.extrapolation()};
  }

  /// @returns The options instance.
  const Options& options() const noexcept
  {
    return options_;
  }

  /**
   * @brief Resamples the given records.
   *
   * @returns The resampled records.
   */
  SensorsData apply(SensorsData&& records)
  {
    return resample([this, &records](const std::size_t column_index)
    {
      auto& resampler = resamplers_[column_index];
      auto& input = records[column_index];
      const auto input_size = input.size();
      if (!input_size)
        return SensorsData::Value{}; // short-circuit

      // Apply the filter.
      auto result = zero_result(resampler, input_size);
      if (options_.crop_extra()) {
        const bool is_first_apply = !resampler.is_applied(); // must precede to resampler.apply()!
        resampler.apply(cbegin(input), cend(input), begin(result));
        if (is_first_apply) {
          const auto b = cbegin(result);
          result.erase(b, b + leading_skip_count(resampler));
        }
      } else
        resampler.apply(cbegin(input), cend(input), begin(result));

      return result;
    });
  }

  /**
   * @brief Resamples the extrapolated sequence.
   *
   * @returns The resampled records.
   *
   * @remarks Normally, this method should be called after the resampling of
   * the last chunk of data.
   */
  SensorsData flush()
  {
    return resample([this](const std::size_t column_index)
    {
      auto& resampler = resamplers_[column_index];
      if (!resampler.is_applied())
        return SensorsData::Value{}; // short-circuit

      // Flush the end samples.
      auto result = zero_result(resampler, resampler.coefs_per_phase() - 1);
      if (options_.crop_extra()) {
        const auto skip_count = trailing_skip_count(resampler);
        resampler.flush(begin(result));
        result.resize(result.size() - skip_count);
      } else
        resampler.flush(begin(result));

      return result;
    });
  }

private:
  using R = FirResampler<float>;
  Options options_;
  std::array<R, SensorsData::SensorsSize()> resamplers_;

  template<typename F>
  SensorsData resample(F&& run)
  {
    SensorsData result;
    constexpr auto sz = SensorsData::SensorsSize();
    for (auto i = 0*sz; i < sz; ++i)
      result[i] = run(i);
    return result;
  }

  SensorsData::Value zero_result(const R& resampler, const std::size_t input_size)
  {
    const auto result_size = resampler.output_sequence_size(input_size);
    return SensorsData::Value(result_size);
  }

  std::size_t leading_skip_count(const R& resampler) const noexcept
  {
    return resampler.output_sequence_size(resampler.coefs_per_phase() - 1) / 2;
  }

  std::size_t trailing_skip_count(const R& resampler) const noexcept
  {
    const auto sz = resampler.output_sequence_size(resampler.coefs_per_phase() - 1);
    return (sz + sz % 2) / 2;
  }

  static void print_firc(const std::vector<double>& firc)
  {
    const auto odd = firc.size() % 2;
    for (auto i = 0*firc.size(); i < firc.size(); ++i) {
      for (auto j = 0*i; j < std::min(i, firc.size() - odd - i); ++j)
        std::cout << " ";
      std::cout << firc[i] << "\n";
    }
    std::cout << std::endl;
  }
};

#endif  // PANDA_TIMESWIPE_DRIVER_RESAMPLER_HPP
