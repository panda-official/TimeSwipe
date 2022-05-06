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

#include "debug.hpp"
#include "driver_basics.hpp"
#include "exceptions.hpp"
#include "fir_resampler.hpp"
#include "math.hpp"
#include "table.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <type_traits>
#include <vector>
#include <utility>

namespace panda::timeswipe::detail {

/// A timeswipe resampler options.
class Resampler_options final {
public:
  /// The default constructor.
  Resampler_options()
    : channel_count_{1}
    , up_factor_{1}
    , down_factor_{1}
    , extrapolation_{Signal_extrapolation::zero}
    , crop_extra_{true}
    , filter_length_{default_filter_length(up_factor_, down_factor_)}
    , freq_{default_freq(up_factor_)}
    , ampl_{default_ampl()}
  {
    PANDA_TIMESWIPE_ASSERT(is_invariant_ok());
  }

  /**
   * @brief Sets the channel count.
   *
   * @par Requires
   * `(value > 0)`.
   *
   * @returns *this.
   */
  Resampler_options& set_channel_count(const unsigned value)
  {
    if (!(value > 0))
      throw Exception{"invalid channel count for resampler"};

    channel_count_ = value;
    PANDA_TIMESWIPE_ASSERT(is_invariant_ok());
    return *this;
  }

  /// @returns Channel count.
  unsigned channel_count() const noexcept
  {
    return channel_count_;
  }

  /**
   * @brief Sets the up and down factors.
   *
   * @param up Up factor.
   * @param down Down factor.
   *
   * @par Requires
   * `(up > 0 && down > 0)`.
   *
   * @returns *this.
   */
  Resampler_options& set_up_down(const int up, const int down)
  {
    if (!(up > 0))
      throw Exception{"invalid up factor for resampler"};
    else if (!(down > 0))
      throw Exception{"invalid down factor for resampler"};

    up_factor_ = up;
    down_factor_ = down;
    PANDA_TIMESWIPE_ASSERT(is_invariant_ok());

    return *this;
  }

  /// @overload
  Resampler_options& set_up_down(const std::pair<int, int> ud)
  {
    return set_up_down(ud.first, ud.second);
  }

  /// @returns The up factor.
  int up_factor() const noexcept
  {
    return up_factor_;
  }

  /// @returns The down factor.
  int down_factor() const noexcept
  {
    return down_factor_;
  }

  /// @returns The pair of up_factor() and down_factor().
  std::pair<int, int> up_down() const noexcept
  {
    return {up_factor(), down_factor()};
  }

  /**
   * @brief Sets the signal extrapolation mode.
   *
   * @returns *this.
   */
  Resampler_options& set_extrapolation(const Signal_extrapolation value)
  {
    extrapolation_ = value;
    PANDA_TIMESWIPE_ASSERT(is_invariant_ok());
    return *this;
  }

  /// @returns The signal extrapolation mode.
  Signal_extrapolation extrapolation() const noexcept
  {
    return extrapolation_;
  }

  /**
   * @brief Sets the crop extra samples mode.
   *
   * @details The resampled sequence is always contains some extra samples at
   * both the begin and end. If this option is set to `true` these extra samples
   * are eliminated automatically.
   *
   * @returns *this.
   */
  Resampler_options& set_crop_extra(const bool value)
  {
    crop_extra_ = value;
    PANDA_TIMESWIPE_ASSERT(is_invariant_ok());
    return *this;
  }

  /// @returns The crop extra samples mode.
  bool crop_extra() const noexcept
  {
    return crop_extra_;
  }

  /**
   * @brief Sets the filter length.
   *
   * @par Requires
   * `(value > 0)`.
   *
   * @returns *this.
   */
  Resampler_options& set_filter_length(const int value)
  {
    if (!(value > 0))
      throw Exception{"invalid filter length for resampler"};

    filter_length_ = value;
    PANDA_TIMESWIPE_ASSERT(is_invariant_ok());
    return *this;
  }

  /// @returns The filter length.
  int filter_length() const noexcept
  {
    return filter_length_;
  }

  /**
   * @brief Sets the both pairs of frequency band edges and amplitude values.
   *
   * @param freq The pairs of frequency band edges. See firls().
   * @param ampl Amplitude values of the function at each frequency point. See firls().
   *
   * @returns *this.
   *
   * @par Requires
   * `!freq.empty() && (freq.size() == ampl.size())`.
   *
   * @see firls().
   */
  Resampler_options& set_freq_ampl(std::vector<double> freq, std::vector<double> ampl)
  {
    if (freq.empty())
      throw Exception{"empty freq for resampler"};
    else if (ampl.empty())
      throw Exception{"empty ampl for resampler"};
    else if (freq.size() != ampl.size())
      throw Exception{"freq and ampl of different sizes for resampler"};

    freq_ = std::move(freq);
    ampl_ = std::move(ampl);
    PANDA_TIMESWIPE_ASSERT(is_invariant_ok());
    return *this;
  }

  /// @overload
  Resampler_options& set_freq_ampl(std::pair<std::vector<double>, std::vector<double>> fa)
  {
    return set_freq_ampl(std::move(fa.first), std::move(fa.second));
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

  /**
   * @returns Amplitude values of the function at each frequency point.
   *
   * @see firls().
   */
  const std::vector<double>& ampl() const noexcept
  {
    return ampl_;
  }

  /// @returns The pair of copies of freq() and ampl().
  std::pair<std::vector<double>, std::vector<double>> freq_ampl() const
  {
    return {freq(), ampl()};
  }

  /// @name Utilities
  ///
  /// @brief Default values.
  ///
  /// @{

  /// @returns The default up factor.
  static int default_up_factor() noexcept
  {
    return 1;
  }

  /// @returns The default down factor.
  static int default_down_factor() noexcept
  {
    return 1;
  }

  /// @returns The pair of default_up_factor() and default_down_factor().
  static std::pair<int, int> default_up_down() noexcept
  {
    return {default_up_factor(), default_down_factor()};
  }

  /// @returns The default filter length.
  static int default_filter_length(const int up_factor, const int down_factor) noexcept
  {
    PANDA_TIMESWIPE_ASSERT(up_factor > 0 && down_factor > 0);
    return 2 * 10 * std::max(up_factor, down_factor) + 1;
  }

  /// @returns The default pairs of frequency band edges.
  static std::vector<double> default_freq(const int up_factor)
  {
    /*
     * Note: when the band_numerator is 1 some of the default
     * firc values can be NaN, so band_numerator is .(9).
     */
    constexpr double band_numerator{1 - positive_near_zero()};
    const double band = band_numerator / up_factor;
    return {0, band, band, 1};
  }

  /// @returns The default amplitude values of the function at each frequency point.
  static std::vector<double> default_ampl()
  {
    return {1, 1, 0, 0};
  }

  /// @returns The pair of default_freq() and default_ampt().
  static std::pair<std::vector<double>, std::vector<double>> default_freq_ampl(const int up_factor)
  {
    return {default_freq(up_factor), default_ampl()};
  }

  /// @}

private:
  unsigned channel_count_;
  int up_factor_;
  int down_factor_;
  Signal_extrapolation extrapolation_;
  bool crop_extra_;
  int filter_length_;
  std::vector<double> freq_;
  std::vector<double> ampl_;

  bool is_invariant_ok() const noexcept
  {
    const bool channel_count_ok = channel_count_ > 0;
    const bool factors_ok = up_factor_ > 0 && down_factor_ > 0;
    const bool length_ok = filter_length_ > 0;
    const bool vecs_ok = !freq_.empty() && !ampl_.empty() && (freq_.size() == ampl_.size());
    return channel_count_ok && factors_ok && length_ok && vecs_ok;
  }
};

/// The generic table data resampler.
template<typename T>
class Generic_table_resampler {
public:
  /// The destructor.
  virtual ~Generic_table_resampler() = default;

  /// @returns `table`.
  virtual Table<T> apply(Table<T> table)
  {
    return table;
  }

  /// @returns The empty table.
  virtual Table<T> flush()
  {
    return Table<T>{};
  }
};

/**
 * @brief A FIR table resampler.
 *
 * @details This class is the wrapper of the class Fir_resampler and provides
 * the stream-style API in order to resample the chunks of variable length.
 * After the resampling of the last chunk, the method flush() should be called
 * in order to resample and flush the extrapolated (extra) sequence of length of
 * one polyphase of filter.
 *
 * @remarks Both excess leading and excess trailing samples (which are actually
 * artifacts of the resampling) will be cropped automatically.
 *
 * @see apply(), flush(), Fir_resampler.
 */
template<typename T>
class Fir_table_resampler final : public Generic_table_resampler<T> {
public:
  /// An alias of Resampler_options.
  using Options = Resampler_options;

  /// The constructor.
  explicit Fir_table_resampler(Options options)
    : options_{std::move(options)}
    , rstates_(options_.channel_count())
  {
    // Calculate FIR coefficients.
    const auto firc = [this]
    {
      std::clog << "Calculating FIR coefficients...";
      std::vector<double> firc = firls(options_.filter_length() - 1,
        options_.freq(), options_.ampl());
      if (firc.size() > std::numeric_limits<int>::max())
        throw Exception{"too many FIR coefficients required"};
      PANDA_TIMESWIPE_ASSERT(static_cast<unsigned>(
          options_.filter_length()) == firc.size());
      std::clog << firc.size() << " coefficients will be used\n";
      // print_firc(firc);

      /*
       * Apply Kaiser window.
       * We must find the most suitable shape factor (beta) possible before
       * applying Kaiser window.
       */
      const auto clog_prec = std::clog.precision();
      std::clog.precision(std::numeric_limits<double>::max_digits10);
      std::clog << "Guessing the best shape factor for Kaiser window of size "
                << firc.size() << "...";
      std::vector<double> result(firc.size());

      /*
       * Create convenient applicator of Kaiser window and adder of the result
       * of application.
       */
      const auto apply_kaiser_and_sum = [&firc, &result,
        u = options_.up_factor()](const double beta)
      {
        const auto window = kaiser(firc.size(), beta);
        PANDA_TIMESWIPE_ASSERT(firc.size() == window.size());
        transform(cbegin(window), cend(window), cbegin(firc), begin(result),
          [u](const auto w, const auto c)
          {
            if (const auto val = u * w * c; std::isnan(val))
              throw Exception{"one of FIR coefficients would be NaN"};
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
        } else if (!(inf < beta && beta < sup))
          throw Exception{"unable to guess shape factor for Kaiser window"
              " (probably, either up factor "+std::to_string(options_.up_factor())+
              " or down factor "+std::to_string(options_.down_factor())+
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

    PANDA_TIMESWIPE_ASSERT(is_invariant_ok());
  }

  /// @returns The options instance.
  const Options& options() const noexcept
  {
    return options_;
  }

  /**
   * @brief Resamples the given table.
   *
   * @returns The resampled table.
   */
  Table<T> apply(Table<T> table) override
  {
    if (table.column_count() != rstates_.size())
      throw Exception{std::string{"cannot resample table with "}
        .append("illegal column count (")
        .append(std::to_string(table.column_count()))
        .append(" instead of ")
        .append(std::to_string(rstates_.size()))
        .append(")")};

    return resample([this, table = std::move(table)](const std::size_t column_index)
    {
      auto& rstate = rstates_[column_index];
      auto& resampler = rstate.resampler;
      auto& input = table.column(column_index);
      const auto input_size = input.size();
      if (!input_size)
        return typename Table<T>::Column{}; // short-circuit

      // Apply the filter.
      auto result = make_zero_result(resampler, input_size);
      const auto out = resampler.apply(cbegin(input), cend(input), begin(result));
      using Sz = decltype(result.size());
      PANDA_TIMESWIPE_ASSERT(result.size() ==
        static_cast<Sz>(std::distance(begin(result), out)));
      if (rstate.unskipped_leading_count) {
        PANDA_TIMESWIPE_ASSERT(options_.crop_extra());
        const auto b = cbegin(result);
        const auto skip_count =
          std::min<std::size_t>(rstate.unskipped_leading_count, result.size());
        result.erase(b, b + skip_count);
        rstate.unskipped_leading_count -= skip_count;
      }

      return result;
    });
  }

  /**
   * @brief Resamples the extrapolated sequence.
   *
   * @returns The resampled table.
   *
   * @remarks Normally, this method should be called after the resampling of
   * the last chunk of data.
   */
  Table<T> flush() override
  {
    return resample([this](const std::size_t column_index)
    {
      auto& rstate = rstates_[column_index];
      auto& resampler = rstate.resampler;
      if (!resampler.is_applied())
        return typename Table<T>::Column{}; // short-circuit

      // Flush the end samples.
      auto result = make_zero_result(resampler, resampler.coefs_per_phase() - 1);
      if (options_.crop_extra()) {
        const auto skip_count = trailing_skip_count(resampler);
        PANDA_TIMESWIPE_ASSERT(skip_count < result.size());
        resampler.flush(begin(result));
        result.resize(result.size() - skip_count);
      } else
        resampler.flush(begin(result));

      return result;
    });
  }

private:
  using R = Fir_resampler<T>;
  Options options_;
  struct State final {
    State() = default;

    template<class U>
    explicit State(const Options& options, const std::vector<U>& firc)
      : resampler{options.up_factor(), options.down_factor(),
                  cbegin(firc), cend(firc), options.extrapolation()}
      , unskipped_leading_count{options.crop_extra() ?
                                leading_skip_count(resampler) : 0}
    {}

    R resampler;
    std::size_t unskipped_leading_count{};
  };
  std::vector<State> rstates_;

  bool is_invariant_ok() const
  {
    return !rstates_.empty() && (options_.channel_count() == rstates_.size());
  }

  template<typename F>
  Table<T> resample(const F& make_resampled)
  {
    const auto column_count = rstates_.size();
    Table<T> result;
    result.reserve_columns(column_count);
    for (std::decay_t<decltype(column_count)> i{}; i < column_count; ++i)
      result.append_column(make_resampled(i));
    return result;
  }

  static typename Table<T>::Column make_zero_result(const R& resampler,
    const std::size_t input_size)
  {
    const auto result_size = resampler.output_sequence_size(input_size);
    return typename Table<T>::Column(result_size);
  }

  static std::size_t leading_skip_count(const R& resampler) noexcept
  {
    return resampler.output_sequence_size(resampler.coefs_per_phase() - 1) / 2;
  }

  static std::size_t trailing_skip_count(const R& resampler) noexcept
  {
    const auto sz = resampler.output_sequence_size(resampler.coefs_per_phase() - 1);
    return (sz + sz % 2) / 2;
  }

  static void print_firc(const std::vector<double>& firc)
  {
    using Size = std::decay_t<decltype(firc.size())>;
    const auto odd = firc.size() % 2;
    for (Size i{}; i < firc.size(); ++i) {
      for (Size j{}; j < std::min(i, firc.size() - odd - i); ++j)
        std::cout << " ";
      std::cout << firc[i] << "\n";
    }
    std::cout << std::endl;
  }
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_RESAMPLER_HPP
