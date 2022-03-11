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

#include "../../debug.hpp"
#include "dsu.hpp"

#include <sam.h>

Product_series product_series() noexcept
{
  const int result = DSU->DID.bit.SERIES;
  switch (result) {
  case 3: return Product_series::e53;
  case 4: return Product_series::e54;
  default: PANDA_TIMESWIPE_ASSERT(false);
  }
}
