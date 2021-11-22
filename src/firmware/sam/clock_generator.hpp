// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SAM_CLOCK_GENERATOR_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SAM_CLOCK_GENERATOR_HPP

#include <memory>

/// A SAME5x clock generator.
class Sam_clock_generator final {
public:
  /// Clock generator ID.
  enum class Id {
    MCLK,  GCLK1, GCLK2, GCLK3, GCLK4, GCLK5,
    GCLK6, GCLK7, GCLK8, GCLK9, GCLK10, GCLK11 };

  /// The destructor.
  ~Sam_clock_generator();

  /// Non copy-constructible.
  Sam_clock_generator(const Sam_clock_generator&) = delete;

  /// Non copy-assignable.
  Sam_clock_generator& operator=(const Sam_clock_generator&) = delete;

  /// Non move-constructible.
  Sam_clock_generator(Sam_clock_generator&&) = delete;

  /// Non move-assignable.
  Sam_clock_generator& operator=(Sam_clock_generator&&) = delete;

  /// @returns The generator ID.
  Id id() const noexcept
  {
    return id_;
  }

  /**
   * @returns The newly created instance, or `nullptr` if no clock generators
   * available.
   */
  static std::shared_ptr<Sam_clock_generator> make();

  /*!
   * \brief Sets clock generator output frequency divider
   * \param div A divider value
   * \details "The Generator clock frequency equals the clock source frequency divided by 2^(N+1), where
   N is the Division Factor Bits for the selected generator" page 165
  */
  void SetDiv(unsigned short div);

  /*!
   * \brief Enables the generator
   * \param how true=enabled, false=disabled
   */
  void Enable(bool how);

private:
  inline static Sam_clock_generator* instances_[12]{};
  Id id_;

  /// The constructor.
  explicit Sam_clock_generator(Id id);

  /**
   * @brief Waits the completion of the bus synchronization.
   *
   * @details Required for some GCLK operations.
   *
   * @see SAME54 manual, page 159: "Due to asynchronicity between the main clock
   * domain and the peripheral clock domains, some registers need to be
   * synchronized when written or read."
   */
  void wait_sync();
};

#endif  //  PANDA_TIMESWIPE_FIRMWARE_SAM_CLOCK_GENERATOR_HPP
