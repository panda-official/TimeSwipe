// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/board_settings.hpp"
#include "../../src/debug.hpp"

#include <iostream>

#define ASSERT PANDA_TIMESWIPE_ASSERT

constexpr std::string_view json_text{R"(
{
"channel1Mode": 0, "channel2Mode": 0, "channel3Mode": 1, "channel4Mode": 1,
"channel1.gain": 1.1, "channel2Gain": 2.2, "channel3Gain": 3.3, "channel4Gain": 4.4,
"channel1Iepe": true, "channel2Iepe": false, "channel3Iepe": false, "channel4Iepe": true,
"pwm1Enabled": false, "pwm2Enabled": true,
"pwm1Frequency": 1, "pwm2Frequency": 10,
"pwm1LowBoundary": 11, "pwm1HighBoundary": 22,
"pwm2LowBoundary": 33, "pwm2HighBoundary": 44,
"pwm1RepeatCount": 0, "pwm2RepeatCount": 11,
"pwm1DutyCycle": 0.11, "pwm2DutyCycle": 0.22
}
  )"
};

int main()
try {
  namespace ts = panda::timeswipe;
  ts::Board_settings bs{json_text};

  // Measurement mode.
  {
    constexpr auto c = ts::Measurement_mode::current;
    constexpr auto v = ts::Measurement_mode::voltage;
    const std::vector<ts::Measurement_mode> expected{v,v,c,c};
    ASSERT(bs.channel_measurement_modes() == expected);
  }

  // Channel gains.
  {
    const std::vector<float> expected{1.1,2.2,3.3,4.4};
    ASSERT(bs.channel_gains() == expected);
  }

  // Channel IEPEs.
  {
    const std::vector<bool> expected{true,false,false,true};
    ASSERT(bs.channel_iepes() == expected);
  }

  // PWM enabled.
  {
    const std::vector<bool> expected{false,true};
    ASSERT(bs.pwm_enabled() == expected);
  }

  // PWM frequencies.
  {
    const std::vector<int> expected{1,10};
    ASSERT(bs.pwm_frequencies() == expected);
  }

  // PWM boundaries.
  {
    const std::vector<std::pair<int,int>> expected{{11,22},{33,44}};
    ASSERT(bs.pwm_boundaries() == expected);
  }

  // PWM repeat counts.
  {
    const std::vector<int> expected{0,11};
    ASSERT(bs.pwm_repeat_counts() == expected);
  }

  // PWM duty cycles.
  {
    const std::vector<float> expected{.11,.22};
    ASSERT(bs.pwm_duty_cycles() == expected);
  }

  {
    ts::Board_settings bs;
    bs.set_channel_gains({1.0,1.0,1.0,1.0});
    ASSERT(bs.channel_gains());
    constexpr auto volt = ts::Measurement_mode::voltage;
    bs.set_channel_measurement_modes({volt,volt,volt,volt});
    ASSERT(bs.channel_measurement_modes());
  }
 } catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << std::endl;
  return 1;
 } catch (...) {
  std::cerr << "unknown error\n";
  return 2;
 }
