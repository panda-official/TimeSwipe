// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/error_detail.hpp"
#include "../../src/board_settings.hpp"

#include <iostream>

#define ASSERT PANDA_TIMESWIPE_ASSERT

constexpr std::string_view json_text{R"(
{
"Mode": 1,
"CH1.mode": 0, "CH2.mode": 0, "CH3.mode": 1, "CH4.mode": 1,
"CH1.gain": 1.1, "CH2.gain": 2.2, "CH3.gain": 3.3, "CH4.gain": 4.4,
"CH1.iepe": true, "CH2.iepe": false, "CH3.iepe": false, "CH4.iepe": true,
"PWM1": false, "PWM2": true,
"PWM1.freq": 1, "PWM2.freq": 10,
"PWM1.low": 11, "PWM1.high": 22, "PWM2.low": 33, "PWM2.high": 44,
"PWM1.repeats": 0, "PWM2.repeats": 11,
"PWM1.duty": 0.11, "PWM2.duty": 0.22
}
  )"
};

int main()
try {
  namespace ts = panda::timeswipe;
  ts::Board_settings bs{json_text};

  ASSERT(bs.signal_mode() == ts::Signal_mode::normal);

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

  // PWMs.
  {
    const std::vector<bool> expected{false,true};
    ASSERT(bs.pwms() == expected);
  }

  // PWM frequencies.
  {
    const std::vector<int> expected{1,10};
    ASSERT(bs.pwm_frequencies() == expected);
  }

  // PWM signal levels.
  {
    const std::vector<std::pair<int,int>> expected{{11,22},{33,44}};
    ASSERT(bs.pwm_signal_levels() == expected);
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
