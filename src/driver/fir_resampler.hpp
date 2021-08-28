// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#ifndef PANDA_TIMESWIPE_DRIVER_FIR_RESAMPLER_HPP
#define PANDA_TIMESWIPE_DRIVER_FIR_RESAMPLER_HPP

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace panda::timeswipe::driver::detail {

/**
 * @brief Signal extrapolation method.
 *
 * @see https://pywavelets.readthedocs.io/en/latest/ref/signal-extension-modes.html
 */
enum class Signal_extrapolation : unsigned char {
  /**
   * @brief Signal is extended by adding zero samples.
   *
   * @par Exposition
   * `... 0 | x1 ... xn | 0 ...`.
   */
  zero,

  /**
   * @brief Border values are replicated.
   *
   * @par Exposition
   * `... x1 | x1 ... xn | xn ...`.
   */
  constant,

  /**
   * @brief Signal is extended by mirroring samples.
   *
   * @par Exposition
   * `... x2 x1 | x1 x2 ... xn-1 xn | xn xn-1 ...`.
   *
   * @remarks This method is also known as half-sample symmetric.
   */
  symmetric,

  /**
   * @brief Signal is extended by reflecting samples.
   *
   * @par Exposition
   * `... x3 x2 | x1 x2 x3 ... xn-2 xn-1 xn | xn-1 xn-2 ...`.
   *
   * @remarks This method is also known as whole-sample symmetric.
   */
  reflect,

  /**
   * @brief Signal is treated as a periodic one.
   *
   * @par Exposition
   * `... xn-1 xn | x1 x2 ... xn-1 xn | x1 x2 ...`.
   */
  periodic,

  /**
   * Signal is extended according to the first derivatives calculated on the
   * edges (straight line).
   */
  smooth,

  /**
   * @brief Signal is extended by mirroring and negating samples.
   *
   * @par Exposition
   * `... -x2 -x1 | x1 x2 ... xn-1 xn | -xn -xn-1 ...`.
   *
   * @remarks This method is also known as half-sample anti-symmetric.
   */
  antisymmetric,

  /**
   * @brief Anti-symmetric-reflect padding. Signal is extended by reflecting
   * anti-symmetrically about the edge samples.
   *
   * @par Exposition
   * `... (2*x1 - x3) (2*x1 - x2) | x1 x2 x3 ... xn-2 xn-1 xn | (2*xn - xn-1) (2*xn - xn-2) ...`.
   *
   * @remarks This method is also known as whole-sample anti-symmetric.
   */
  antireflect
};

/**
 * @brief A functor for resampling.
 *
 * This class provides an efficient, polyphase finite impulse response (FIR)
 * resampling. The FIR filter is usually designed to prevent aliasing from
 * corrupting the output signal.
 *
 * A "filter bank with resampling" is an operation on an input signal that
 * generates an output signal, consisting of the following 3 steps:
 *   -# upsampling (that is, zero-insertion) of the input signal by the `up_factor`;
 *   -# applying an FIR filter to the result of `(1)`;
 *   -# downsampling (i.e. decimation) of the result of `(2)` by the `down_factor`.
 * For an input signal with sampling rate `rate`, the generated output signal has
 * sampling rate of `(rate * up_factor/down_factor)`.
 *
 * @see Figure 4.3-8(d) on page 129 of the P. P. Vaidyanathan, Multirate Systems
 * and Filter Banks, Prentice Hall PTR, 1993.
 */
template<typename In, typename Coef = In, typename Out = std::common_type_t<In, Coef>>
class FirResampler final {
public:
  /// An alias of input element type.
  using Input = In;

  /// An alias of coefficient type.
  using Coeff = Coef;

  /// An alias of output element type.
  using Output = Out;

  /// The default constructor.
  FirResampler() = default;

  /**
   * @brief The constructor.
   *
   * @param up_rate Up rate.
   * @param down_rate Down rate.
   * @param coefs_first First coefficient iterator. (Must be random access iterator.)
   * @param coefs_last Last coefficient iterator. (Must be random access iterator.)
   *
   * The coefficients are copied into local storage in a transposed, flipped
   * arrangement. For example, suppose `up_rate` is `3`, and the input number
   * of coefficients is `10`, represented as h[0], ..., h[9]. Then the internal
   * buffer will be represented as follows:
   *    h[9], h[6], h[3], h[0],   // flipped phase 0 coefs
   *       0, h[7], h[4], h[1],   // flipped phase 1 coefs (zero-padded)
   *       0, h[8], h[5], h[2],   // flipped phase 2 coefs (zero-padded)
   */
  template<typename InputIt>
  FirResampler(const unsigned up_rate, const unsigned down_rate,
    const InputIt coefs_first, const InputIt coefs_last,
    const Signal_extrapolation signal_extrapolation = Signal_extrapolation::zero)
    : up_rate_{up_rate}
    , down_rate_{down_rate}
    , signal_extrapolation_{signal_extrapolation}
  {
    using ItCtg = typename std::iterator_traits<InputIt>::iterator_category;
    static_assert(std::is_same_v<ItCtg, std::random_access_iterator_tag>);

    const auto coefs_size = std::distance(coefs_first, coefs_last);
    if (up_rate <= 0)
      throw std::invalid_argument{"up_rate"};
    else if (down_rate <= 0)
      throw std::invalid_argument{"down_rate"};
    else if (!coefs_size)
      throw std::invalid_argument{"coefs_first or coefs_last"};

    // Initial coefficients with padding.
    transposed_coefs_.resize(coefs_size + (up_rate - coefs_size % up_rate));

    // Coefficients per phase and initial state buffer.
    coefs_per_phase_ = transposed_coefs_.size() / up_rate_;
    state_.resize(coefs_per_phase_ - 1);

    // Transposing and "flipping" each phase.
    for (auto i = 0*up_rate_; i < up_rate_; ++i) {
      for (auto j = 0*coefs_per_phase_; j < coefs_per_phase_; ++j) {
        if (j*up_rate_ + i < coefs_size)
          transposed_coefs_[(coefs_per_phase_ - 1 - j) + i*coefs_per_phase_] = coefs_first[j*up_rate_ + i];
      }
    }

    assert(is_invariant_ok());
  }

  /// Non copy-constructible.
  FirResampler(const FirResampler&) = delete;

  /// Non copy-assignable.
  FirResampler& operator=(const FirResampler&) = delete;

  /// Move-constructible.
  FirResampler(FirResampler&& rhs) noexcept
  {
    swap(rhs);
  }

  /// Move-assignable.
  FirResampler& operator=(FirResampler&& rhs) noexcept
  {
    if (this != &rhs) {
      FirResampler tmp{std::move(rhs)};
      swap(tmp);
    }
    return *this;
  }

  /// Swappable.
  void swap(FirResampler& rhs) noexcept
  {
    using std::swap;
    swap(up_rate_, rhs.up_rate_);
    swap(down_rate_, rhs.down_rate_);
    swap(signal_extrapolation_, rhs.signal_extrapolation_);
    swap(coefs_phase_, rhs.coefs_phase_);
    swap(apply_offset_, rhs.apply_offset_);
    swap(coefs_per_phase_, rhs.coefs_per_phase_);
    swap(transposed_coefs_, rhs.transposed_coefs_);
    swap(state_, rhs.state_);
  }

  /**
   * @brief Resamples the sequence in range `[first, last)`. Writes
   * `output_sequence_size(std::distance(first, last))` samples to the output
   * sequence that starts with `out`.
   *
   * The first time this function is called, the initial signal extrapolation
   * performed. The length of the initial (left-hand-side) signal extension is
   * `std::min(coefs_per_phase() - 1, last - first)`. In case when
   * `last - first < coefs_per_phase() - 1` the leading values of the extended
   * signal are default-constructed (i.e. zeros).
   *
   * @param first First input sequence iterator. (Must be random access iterator.)
   * @param last Last input sequence iterator. (Must be random access iterator.)
   * @param out First output sequence iterator.
   *
   * @returns Output iterator to the element in the destination range,
   * one-past-the-last element copied, or `out` if
   *   - not enough input samples for initial extrapolation;
   *   - `[first, last)` denotes empty sequence.
   *
   * @see flush().
   */
  template<typename InputIt, typename OutputIt>
  auto apply(const InputIt first, const InputIt last, OutputIt out)
  {
    using It_ctg = typename std::iterator_traits<InputIt>::iterator_category;
    using It_value = typename std::iterator_traits<InputIt>::value_type;
    static_assert(std::is_same_v<It_ctg, std::random_access_iterator_tag>);
    static_assert(std::is_convertible_v<It_value, Input>);

    const decltype(state_.size()) in_size = std::distance(first, last);
    if (!in_size)
      return out;

    assert(in_size);

    if (!is_applied_) {
      // Handle simple extrapolation methods.
      switch (signal_extrapolation_) {
      case Signal_extrapolation::zero:
        goto resample; // already done upon construction
      case Signal_extrapolation::smooth: {
        const auto sz = state_.size();
        const auto [x1, x2] = in_size > 1 ?
          std::make_pair(first[0], first[1]):
          std::make_pair(first[0], It_value{});
        for (auto k = sz; k > 0; --k)
          state_[sz - k] = x1 - k*(x2 - x1);
        goto resample;
      }
      case Signal_extrapolation::constant:
        fill(begin(state_), end(state_), *first);
        goto resample;
      default:;
      }

      // Copy the input to the state buffer for more complicated extrapolation.
      {
        const auto count = std::min(state_.size(), in_size);
        const auto e = end(state_);
        const auto b = e - count;
        assert(first + count <= last);
        copy(first, first + count, b);
      }

      // Handle more complicated extrapolation methods by modifying the state buffer.
      switch (signal_extrapolation_) {
      case Signal_extrapolation::zero:;
        [[fallthrough]];
      case Signal_extrapolation::smooth:
        [[fallthrough]];
      case Signal_extrapolation::constant:;
        assert(false);
        std::terminate();
      case Signal_extrapolation::symmetric:
        reverse(begin(state_), end(state_));
        break;
      case Signal_extrapolation::reflect:
        reflect_left(state_);
        break;
      case Signal_extrapolation::periodic:
        break;
      case Signal_extrapolation::antisymmetric:
        reverse(begin(state_), end(state_));
        for_each(begin(state_), end(state_), [](auto& x){ x = -x; });
        break;
      case Signal_extrapolation::antireflect:
        auto reflected = state_;
        reflect_left(reflected);
        if (const auto sz = state_.size(); sz >= 2) {
          const auto x1 = state_[0];
          for (auto i = 0*sz; i < sz; ++i)
            state_[i] = 2*x1 - reflected[i];
        }
        break;
      }
    }

  resample:
    auto in = first + apply_offset_;
    while (in < last) {
      Output value{};
      auto h = cbegin(transposed_coefs_) + coefs_phase_*coefs_per_phase_;
      auto in_ptr = in - state_.size();
      if (const auto diff = first - in_ptr; diff > 0) {
        // Use values from the state_ buffer.
        assert(diff <= state_.size());
        const auto e = cend(state_);
        for (auto state_ptr = e - diff; state_ptr < e; ++state_ptr, ++h)
          value += *state_ptr * *h;
        std::advance(in_ptr, diff);
      }
      for (; in_ptr <= in; ++in_ptr, ++h)
        value += *in_ptr * *h;

      *out = value;
      ++out;

      coefs_phase_ += down_rate_;
      const auto advance_amount = coefs_phase_ / up_rate_;
      std::advance(in, advance_amount);
      coefs_phase_ %= up_rate_;
    }
    apply_offset_ = in - last;

    // --------------------
    // Manage state_ buffer
    // --------------------

    if (in_size < state_.size()) {
      // Calculate number of samples retained in the state buffer.
      const auto retain = state_.size() - in_size;

      // Copy end of buffer to beginning.
      auto o = copy(cend(state_) - retain, cend(state_), begin(state_));

      // Copy the entire (short) input to end of buffer.
      o = copy(first, last, o);

      assert(o - cbegin(state_) == state_.size());
    } else
      // Just copy last input samples into state buffer.
      copy(last - state_.size(), last, begin(state_));

    is_applied_ = true;
    assert(is_invariant_ok());
    return out;
  }

  /**
   * @brief Resamples the extrapolated (extra) sequence of length of one
   * polyphase of filter. Writes `output_sequence_size(coefs_per_phase() - 1)`
   * samples to the output sequence that starts with `out`.
   *
   * @param out First output sequence iterator.
   *
   * This method should be called after the last call of apply() in order to
   * flush the end samples out.
   *
   * @returns Output iterator to the element in the destination range,
   * one-past-the-last element copied.
   *
   * @see apply().
   */
  template<typename OutputIt>
  auto flush(OutputIt out)
  {
    auto extra = state_;
    const auto b = begin(extra);
    const auto e = end(extra);
    switch (signal_extrapolation_) {
    case Signal_extrapolation::zero:
      fill(b, e, Input{});
      break;
    case Signal_extrapolation::smooth: {
      using Value = typename decltype(extra)::value_type;
      const auto sz = extra.size();
      const auto [xn, xn_1] = extra.size() > 1 ?
        std::make_pair(extra[sz - 1], extra[sz - 2]) :
        std::make_pair(Value{}, extra[sz - 1]);
      for (auto k = 0*sz + 1; k <= sz; ++k)
        extra[k - 1] = xn + k*(xn - xn_1);
      break;
    }
    case Signal_extrapolation::constant:
      fill(b, e, state_.back());
      break;
    case Signal_extrapolation::symmetric:
      reverse(b, e);
      break;
    case Signal_extrapolation::reflect:
      reflect_right(extra);
      break;
    case Signal_extrapolation::periodic:
      break;
    case Signal_extrapolation::antisymmetric:
      reverse(b, e);
      for_each(b, e, [](auto& x){ x = -x; });
      break;
    case Signal_extrapolation::antireflect:
      auto reflected = extra;
      reflect_right(reflected);
      if (const auto sz = extra.size(); sz >= 2) {
        const auto xn = extra.back();
        for (auto i = 0*sz; i < sz; ++i)
          extra[i] = 2*xn - reflected[i];
      }
      break;
    }
    auto result = apply(cbegin(extra), cend(extra), out);
    is_flushed_ = true;
    return result;
  }

  /// @returns apply(first, last, out).
  template<typename InputIt, typename OutputIt>
  auto operator()(const InputIt first, const InputIt last, OutputIt out)
  {
    return apply(first, last, out);
  }

  /// @returns flush(out).
  template<typename OutputIt>
  auto operator()(OutputIt out)
  {
    return flush(out);
  }

  /// @returns `true` if apply() is successfuly called at least once.
  bool is_applied() const noexcept
  {
    return is_applied_;
  }

  /// @returns `true` if flush() is successfuly called at least once.
  bool is_flushed() const noexcept
  {
    return is_flushed_;
  }

  /**
   * @returns The required size of the output sequence, i.e. how many samples
   * will be written out upon processing the input sequence of size `in_size`.
   */
  std::size_t output_sequence_size(const std::size_t in_size) const noexcept
  {
    const auto np = in_size * up_rate_;
    std::size_t result = np / down_rate_;
    if ((coefs_phase_ + up_rate_ * apply_offset_) < (np % down_rate_))
      result++;
    return result;
  }

  /// @returns The number of coefficients per phase.
  unsigned coefs_per_phase() const noexcept
  {
    return coefs_per_phase_;
  }

private:
  bool is_applied_{};
  bool is_flushed_{};
  unsigned up_rate_{};
  unsigned down_rate_{};
  Signal_extrapolation signal_extrapolation_{Signal_extrapolation::zero};
  unsigned coefs_phase_{}; // next phase of the filter to use (mod up_rate_)
  unsigned apply_offset_{}; // the amount of samples to skip upon apply()
  typename std::vector<Coeff>::size_type coefs_per_phase_{}; // transposed_coefs_.size() / up_rate_
  std::vector<Coeff> transposed_coefs_;
  std::vector<Input> state_; // state buffer of size (coefs_per_phase_ - 1)

  bool is_invariant_ok() const noexcept
  {
    const bool rates_ok = up_rate_ > 0 && down_rate_ > 0;
    const bool time_ok = coefs_phase_ < up_rate_;
    const bool vecs_ok =
      !state_.empty() &&
      (state_.size() == (coefs_per_phase_ - 1)) &&
      (transposed_coefs_.size() == coefs_per_phase_ * up_rate_) &&
      !(transposed_coefs_.size() % up_rate_);
    return rates_ok && time_ok && vecs_ok;
  }

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  static void reflect_left(std::vector<Input>& state) noexcept
  {
    if (state.size() >= 3) {
      copy(begin(state) + 1, end(state), begin(state)); // x2,...,xn,xn
      reverse(begin(state), end(state)); // xn,xn,...,x2
      state[0] = state[2]; // xn-1,xn,...,x2
    }
  }

  static void reflect_right(std::vector<Input>& state) noexcept
  {
    if (state.size() >= 3) {
      reverse(begin(state), end(state)); // xn,...,x2,x1
      copy(begin(state) + 1, end(state), begin(state)); // xn-1,...,x2,x1,x1
      state.back() = state[state.size() - 3]; // xn-1,...,x2,x1,x2
    }
  }
};

/**
 * @brief Performs a one-shot resampling.
 *
 * @returns The result vector.
 */
template<typename InputIt, typename CoefInputIt>
auto resample(const unsigned up_rate, const unsigned down_rate,
  const CoefInputIt coefs_first, const CoefInputIt coefs_last,
  const InputIt first, const InputIt last,
  const Signal_extrapolation extrapolation = Signal_extrapolation::zero)
{
  using InputItCtg = typename std::iterator_traits<InputIt>::iterator_category;
  using CoefInputItCtg = typename std::iterator_traits<CoefInputIt>::iterator_category;
  static_assert(std::is_same_v<InputItCtg, std::random_access_iterator_tag>);
  static_assert(std::is_same_v<CoefInputItCtg, std::random_access_iterator_tag>);

  using In = typename std::iterator_traits<InputIt>::value_type;
  using Coef = typename std::iterator_traits<CoefInputIt>::value_type;
  using R = FirResampler<In, Coef>;
  using Out = typename R::Output;
  R resampler{up_rate, down_rate, coefs_first, coefs_last, extrapolation};
  const auto end_sz = resampler.output_sequence_size(resampler.coefs_per_phase() - 1);
  const auto res_sz = resampler.output_sequence_size(std::distance(first, last)) + end_sz;
  std::vector<Out> result(res_sz);
  auto out = resampler(first, last, begin(result));
  assert(cend(result) - out == end_sz);
  resampler(out);
  return result;
}

} // namespace panda::timeswipe::driver::detail

#endif  // PANDA_TIMESWIPE_DRIVER_FIR_RESAMPLER_HPP
