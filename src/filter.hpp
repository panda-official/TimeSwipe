// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH
*/

#ifndef PANDA_TIMESWIPE_FILTER_HPP
#define PANDA_TIMESWIPE_FILTER_HPP

#include "iir_filter.hpp"

#include <utility>
#include <vector>

namespace panda::timeswipe::detail {

/**
 * @brief Data filter.
 *
 * @details This class is the wrapper of the class Iir_filter.
 *
 * @see Iir_filter.
 */
class Filter final {
public:
  /// Default-constructible.
  Filter() = default;

  /**
   * @brief Constructs `channel_count` filters.
   *
   * @param channel_count The number of channels.
   * @param args The arguments to be passed to Iir_filter constructor.
   *
   * @see Iir_filter.
   */
  template<typename ... Types>
  explicit Filter(const unsigned channel_count, Types&& ... args)
  {
    if (!channel_count)
      throw Exception{"cannot create IIR filter(s): invalid channel count"};

    for (unsigned i{}; i < channel_count; ++i)
      filters_.emplace_back(std::forward<Types>(args)...);
  }

  /// Applies filter of th egiven `channel` to `value`.
  double apply(const unsigned channel, const double value) const
  {
    if (!(channel < filters_.size()))
      throw Exception{"invalid channel index for IIR filtering"};
    return filters_[channel].apply(value);
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

private:
  mutable std::vector<Iir_filter> filters_;
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_FILTER_HPP
