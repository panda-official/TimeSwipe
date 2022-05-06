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

/// The generic data filters.
class Generic_filters {
public:
  /// The destructor.
  virtual ~Generic_filters() = default;

  /// Default-constructible.
  Generic_filters() = default;

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

/**
 * @brief IIR data filters.
 *
 * @see Iir_filter.
 */
class Iir_filters final : public Generic_filters {
public:
  /// Default-constructible.
  Iir_filters() = default;

  /**
   * @brief Constructs `channel_count` filters.
   *
   * @param channel_count The number of channels.
   * @param args The arguments to be passed to filter constructor according to
   * the filter_mode parameter.
   */
  template<typename ... Types>
  explicit Iir_filters(const unsigned channel_count, Types&& ... args)
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
