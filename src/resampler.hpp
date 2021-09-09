// -*- C++ -*-

// PANDA Timeswipe Project
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

#ifndef PANDA_TIMESWIPE_RESAMPLER_HPP
#define PANDA_TIMESWIPE_RESAMPLER_HPP

#include "data_vector.hpp"
#include "fir_resampler.hpp"
#include "math.hpp"

#include "3rdparty/dmitigr/assert.hpp"

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

namespace panda::timeswipe::detail {

/// A timeswipe resampler options.
/// FIXME: merge up_factor() and down_factor() into factors().
class Resampler_options final {
public:
  /**
   * @brief The constructor.
   *
   * @par Requires
   * `freq.size() == ampl.size()`.
   */
  Resampler_options(const unsigned up_factor = 1, const unsigned down_factor = 1,
    const Signal_extrapolation extrapolation = Signal_extrapolation::zero,
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
    DMITIGR_ASSERT(is_invariant_ok());
  }

  /**
   * Sets the up-factor.
   *
   * @returns *this.
   */
  Resampler_options& set_up_factor(const unsigned value) noexcept
  {
    up_factor__(value);
    DMITIGR_ASSERT(is_invariant_ok());
    return *this;
  }

  /// @returns The up-factor.
  unsigned get_up_factor() const noexcept
  {
    return up_factor_;
  }

  /// @returns The default up-factor.
  static unsigned get_default_up_factor() noexcept
  {
    return 1;
  }

  /**
   * Sets the down-factor.
   *
   * @returns *this.
   */
  Resampler_options& set_down_factor(const unsigned value) noexcept
  {
    down_factor__(value);
    DMITIGR_ASSERT(is_invariant_ok());
    return *this;
  }

  /// @returns The down-factor.
  unsigned get_down_factor() const noexcept
  {
    return down_factor_;
  }

  /// @returns The default down-factor.
  static unsigned get_default_down_factor() noexcept
  {
    return 1;
  }

  /**
   * Sets the signal extrapolation mode.
   *
   * @returns *this.
   */
  Resampler_options& set_extrapolation(const Signal_extrapolation value) noexcept
  {
    extrapolation_ = value;
    DMITIGR_ASSERT(is_invariant_ok());
    return *this;
  }

  /// @returns The signal extrapolation mode.
  Signal_extrapolation get_extrapolation() const noexcept
  {
    return extrapolation_;
  }

  /// @returns The default extrapolation mode.
  static Signal_extrapolation get_default_extrapolation() noexcept
  {
    return Signal_extrapolation::zero;
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
  Resampler_options& set_crop_extra(const bool value) noexcept
  {
    crop_extra_ = value;
    DMITIGR_ASSERT(is_invariant_ok());
    return *this;
  }

  /// @returns The crop extra samples mode.
  bool get_crop_extra() const noexcept
  {
    return crop_extra_;
  }

  /// @returns The default crop extra samples mode.
  static bool get_default_crop_extra() noexcept
  {
    return true;
  }

  /**
   * Sets the filter length.
   *
   * @returns *this.
   */
  Resampler_options& set_filter_length(const unsigned value) noexcept
  {
    filter_length__(value);
    DMITIGR_ASSERT(is_invariant_ok());
    return *this;
  }

  /// @returns The filter length.
  unsigned get_filter_length() const noexcept
  {
    return filter_length_;
  }

  /// @returns The default filter length.
  unsigned get_default_filter_length() const noexcept
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
  Resampler_options& set_freq_ampl(std::vector<double> freq, std::vector<double> ampl) noexcept
  {
    DMITIGR_ASSERT(freq.size() == ampl.size());
    freq_ampl__(std::move(freq), std::move(ampl));
    DMITIGR_ASSERT(is_invariant_ok());
    return *this;
  }

  /**
   * @returns The pairs of frequency band edges.
   *
   * @see firls().
   */
  const std::vector<double>& get_freq() const noexcept
  {
    return freq_;
  }

  /// @returns The default pairs of frequency band edges.
  std::vector<double> get_default_freq() const
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
  const std::vector<double>& get_ampl() const noexcept
  {
    return ampl_;
  }

  /// @returns The default amplitude values of the function at each frequency point.
  static std::vector<double> get_default_ampl()
  {
    return {1, 1, 0, 0};
  }

private:
  unsigned up_factor_{};
  unsigned down_factor_{};
  Signal_extrapolation extrapolation_{Signal_extrapolation::zero};
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
    up_factor_ = value ? value : get_default_up_factor();
  }

  void down_factor__(const unsigned value) noexcept
  {
    down_factor_ = value ? value : get_default_down_factor();
  }

  void filter_length__(const unsigned value) noexcept
  {
    filter_length_ = value ? value : get_default_filter_length();
  }

  void freq_ampl__(std::vector<double>&& freq, std::vector<double>&& ampl) noexcept
  {
    if (freq.empty()) {
      freq_ = get_default_freq();
      ampl_ = get_default_ampl();
    } else {
      freq_ = std::move(freq);
      ampl_ = std::move(ampl);
    }
  }
};

/**
 * @brief A resampler.
 *
 * This class is the wrapper of the class Fir_resampler and provides the
 * stream-style API in order to resample the chunks of variable length. After
 * the resampling of the last chunk, the method flush() should be called in
 * order to resample and flush the extrapolated (extra) sequence of length of
 * one polyphase of filter.
 *
 * @remarks Both excess leading and excess trailing samples (which are actually
 * artifacts of the resampling) will be cropped automatically.
 *
 * @see apply(), flush(), Fir_resampler.
 */
class Resampler final {
public:
  /// An alias of Resampler_options.
  using Options = Resampler_options;

  /// The constructor.
  Resampler(Options options)
    : options_{std::move(options)}
  {
    // Calculate FIR coefficients.
    const auto firc = [this]
    {
      std::clog << "Calculating FIR coefficients...";
      std::vector<double> firc = firls(options_.get_filter_length() - 1, options_.get_freq(), options_.get_ampl());
      if (firc.size() > std::numeric_limits<int>::max())
        throw std::runtime_error{"too many FIR coefficients required"};
      DMITIGR_ASSERT(options_.get_filter_length() == firc.size());
      std::clog << firc.size() << " coefficients will be used\n";
      // print_firc(firc);

      /*
       * Apply Kaiser window.
       * We must find the most suitable shape factor (beta) possible before
       * applying Kaiser window.
       */
      const auto clog_prec = std::clog.precision();
      std::clog.precision(std::numeric_limits<double>::max_digits10);
      std::clog << "Guessing the best shape factor for Kaiser window of size "<<firc.size()<<"...";
      std::vector<double> result(firc.size());

      // Create convenient applicator of Kaiser window and adder of the result of application.
      const auto apply_kaiser_and_sum = [&firc, &result, u = options_.get_up_factor()](const double beta)
      {
        const auto window = kaiser(firc.size(), beta);
        DMITIGR_ASSERT(firc.size() == window.size());
        transform(cbegin(window), cend(window), cbegin(firc), begin(result),
          [u](const auto w, const auto c)
          {
            if (const auto val = u * w * c; std::isnan(val))
              throw std::runtime_error{"one of FIR coefficients would be NaN"};
            else
              return val;
          });
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
      constexpr double inf{}, sup{30};
      const auto up_factor = options_.get_up_factor();
      for (double prev_beta{initial_beta}, prev_sum{initial_sum};;) {
        const double beta = prev_beta + delta;
        const double sum = apply_kaiser_and_sum(beta);
        // std::clog << "beta = " << beta  << ", sum = " << sum << std::endl;
        // break;
        if (std::abs(sum - up_factor) > std::abs(prev_sum - up_factor)) {
          apply_kaiser_and_sum(prev_beta);
          std::clog << prev_beta << "\n";
          break;
        } else if (!(inf < beta && beta < sup))
          throw std::runtime_error{"unable to guess shape factor for Kaiser window"
              " (probably, either up-factor "+std::to_string(options_.get_up_factor())+
              " or down-factor "+std::to_string(options_.get_down_factor())+
              " are exorbitant to handle)"};

        prev_beta = beta;
        prev_sum = sum;
      }
      std::clog.precision(clog_prec);
      return result;
    }();
    // print_firc(firc);

    // Initializing the underlying resamplers and the associated states.
    for (auto& rs : rstates_)
      rs = State{options_, firc};
  }

  /// @returns The options instance.
  const Options& get_options() const noexcept
  {
    return options_;
  }

  /**
   * @brief Resamples the given records.
   *
   * @returns The resampled records.
   */
  Data_vector apply(Data_vector&& records)
  {
    return resample([this, &records](const std::size_t column_index)
    {
      auto& rstate = rstates_[column_index];
      auto& resampler = rstate.resampler;
      auto& input = records[column_index];
      const auto input_size = input.size();
      if (!input_size)
        return Data_vector::value_type{}; // short-circuit

      // Apply the filter.
      auto result = make_zero_result(resampler, input_size);
      const auto out = resampler.apply(cbegin(input), cend(input), begin(result));
      using Sz = decltype(result.size());
      DMITIGR_ASSERT(result.size() == static_cast<Sz>(std::distance(begin(result), out)));
      if (rstate.unskipped_leading_count) {
        DMITIGR_ASSERT(options_.get_crop_extra());
        const auto b = cbegin(result);
        const auto skip_count = std::min<std::size_t>(rstate.unskipped_leading_count, result.size());
        result.erase(b, b + skip_count);
        rstate.unskipped_leading_count -= skip_count;
      }

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
  Data_vector flush()
  {
    return resample([this](const std::size_t column_index)
    {
      auto& rstate = rstates_[column_index];
      auto& resampler = rstate.resampler;
      if (!resampler.is_applied())
        return Data_vector::value_type{}; // short-circuit

      // Flush the end samples.
      auto result = make_zero_result(resampler, resampler.get_coefs_per_phase() - 1);
      if (options_.get_crop_extra()) {
        const auto skip_count = get_trailing_skip_count(resampler);
        DMITIGR_ASSERT(skip_count < result.size());
        resampler.flush(begin(result));
        result.resize(result.size() - skip_count);
      } else
        resampler.flush(begin(result));

      return result;
    });
  }

private:
  using R = Fir_resampler<float>;
  Options options_;
  struct State final {
    State() = default;

    template<class T>
    explicit State(const Options& options, const std::vector<T>& firc)
      : resampler{options.get_up_factor(), options.get_down_factor(),
                  cbegin(firc), cend(firc), options.get_extrapolation()}
      , unskipped_leading_count{options.get_crop_extra() ? get_leading_skip_count(resampler) : 0}
    {}

    R resampler;
    std::size_t unskipped_leading_count{};
  };
  std::array<State, max_data_channel_count> rstates_;

  template<typename F>
  Data_vector resample(F&& run)
  {
    Data_vector result(max_data_channel_count);
    constexpr auto cc = max_data_channel_count;
    for (std::size_t i {}; i < cc; ++i)
      result[i] = run(i);
    return result;
  }

  static Data_vector::value_type make_zero_result(const R& resampler, const std::size_t input_size)
  {
    const auto result_size = resampler.get_output_sequence_size(input_size);
    return Data_vector::value_type(result_size);
  }

  static std::size_t get_leading_skip_count(const R& resampler) noexcept
  {
    return resampler.get_output_sequence_size(resampler.get_coefs_per_phase() - 1) / 2;
  }

  static std::size_t get_trailing_skip_count(const R& resampler) noexcept
  {
    const auto sz = resampler.get_output_sequence_size(resampler.get_coefs_per_phase() - 1);
    return (sz + sz % 2) / 2;
  }

  static void print_firc(const std::vector<double>& firc)
  {
    const auto odd = firc.size() % 2;
    for (std::size_t i{}; i < firc.size(); ++i) {
      for (std::size_t j{}; j < std::min(i, firc.size() - odd - i); ++j)
        std::cout << " ";
      std::cout << firc[i] << "\n";
    }
    std::cout << std::endl;
  }
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_RESAMPLER_HPP
