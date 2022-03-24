// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/debug.hpp"
#include "../../src/driver_settings.hpp"

#include <iostream>

#define ASSERT PANDA_TIMESWIPE_ASSERT

constexpr std::string_view json_text{R"(
{
"sampleRate": 24000,
"burstBufferSize": 12000,
"translationOffsets": [1.1, 2.2, 3.3, 4.4],
"translationSlopes": [1.1, 2.2, 3.3, 4.4]
}
  )"
};

int main()
try {
  namespace ts = panda::timeswipe;
  ts::Driver_settings ds{json_text};

  // Sample rate.
  {
    const int expected{24000};
    ASSERT(ds.sample_rate() == expected);
  }

  // Burst buffer size.
  {
    const std::size_t expected{12000};
    ASSERT(ds.burst_buffer_size() == expected);
  }

  // Frequency
  {
    ASSERT(ds.frequency() == std::nullopt);
  }

  // Translation offsets
  {
    const std::vector<float> expected{1.1,2.2,3.3,4.4};
    ASSERT(ds.translation_offsets() == expected);
  }

  // Translation slopes
  {
    const std::vector<float> expected{1.1,2.2,3.3,4.4};
    ASSERT(ds.translation_slopes() == expected);
  }
 } catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << std::endl;
  return 1;
 } catch (...) {
  std::cerr << "unknown error\n";
  return 2;
 }
