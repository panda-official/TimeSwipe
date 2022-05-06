// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH
*/

#ifndef PANDA_TIMESWIPE_FILTER_HPP
#define PANDA_TIMESWIPE_FILTER_HPP

#include "driver_basics.hpp"
#include "iir_filter.hpp"

#include <utility>
#include <vector>

namespace panda::timeswipe::detail {

// -----------------------------------------------------------------------------
// Generic_filter_container
// -----------------------------------------------------------------------------

/// The generic data filter container.
class Generic_filter_container {
public:
  /// The destructor.
  virtual ~Generic_filter_container() = default;

  /// @returns `value`.
  virtual double apply(const unsigned /*channel*/, const double value) const
  {
    return value;
  }

  /**
   * @returns apply(channel, value).
   *
   * @see apply().
   */
  double operator()(const unsigned channel, const double value) const
  {
    return apply(channel, value);
  }
};

// -----------------------------------------------------------------------------
// Iir_filter_vector
// -----------------------------------------------------------------------------

/**
 * @brief IIR data filter vector.
 *
 * @see Iir_filter.
 */
class Iir_filter_vector final : public Generic_filter_container {
public:
  /// Default-constructible.
  Iir_filter_vector() = default;

  /**
   * @brief Constructs the filter vector of size `channel_count`.
   *
   * @param channel_count The number of channels.
   * @param args The arguments to be passed to filter constructor according to
   * the filter_mode parameter.
   */
  template<typename ... Types>
  explicit Iir_filter_vector(const unsigned channel_count, Types&& ... args)
  {
    if (!channel_count)
      throw Exception{"cannot create filter(s): invalid channel count"};

    for (unsigned i{}; i < channel_count; ++i)
      filters_.emplace_back(std::forward<Types>(args)...);
  }

  /// Applies filter of th egiven `channel` to `value`.
  double apply(const unsigned channel, const double value) const override
  {
    if (!(channel < filters_.size()))
      throw Exception{"invalid channel index for filtering"};
    return filters_[channel].apply(value);
  }

private:
  mutable std::vector<Iir_filter> filters_;
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_FILTER_HPP
