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

#include "data_vector.hpp"
#include "error_detail.hpp"

namespace panda::timeswipe {
using namespace detail;

Data_vector::Data_vector(const size_type channel_count)
  : channel_count_{channel_count}
{
  if (channel_count > max_channel_count)
    throw Generic_exception{"cannot create data vector by using excessive channel count"};
}

} // namespace panda::timeswipe
