// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2022  PANDA GmbH

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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SAM_DSU_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SAM_DSU_HPP

/**
 * @brief The product series part of the ordering code.
 *
 * @see Device Identification section of SAM D5x/E5x Family Data Sheet.
 */
enum class Product_series {
  e53 = 3,
  e54 = 4
};

/// @returns The product series part of the ordering code.
Product_series product_series() noexcept;

#endif  // PANDA_TIMESWIPE_FIRMWARE_SAM_DSU_HPP
