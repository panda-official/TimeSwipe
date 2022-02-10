// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/basics.hpp"
#include "../../src/board_settings.hpp"
#include "../../src/debug.hpp"

#include <iostream>

#define ASSERT PANDA_TIMESWIPE_ASSERT

constexpr std::string_view json_text{R"(
{
"channel1Mode": 0, "channel2Mode": 0, "channel3Mode": 1, "channel4Mode": 1,
"channel1Gain": 1.1, "channel2Gain": 2.2, "channel3Gain": 3.3, "channel4Gain": 4.4,
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
  using Mm = ts::Measurement_mode;
  using std::any_cast;
  ts::Board_settings bs{json_text};

  // Measurement mode.
  {
    constexpr auto c = Mm::current;
    constexpr auto v = Mm::voltage;
    ASSERT(any_cast<Mm>(bs.value("channel1Mode")) == v);
    ASSERT(any_cast<Mm>(bs.value("channel2Mode")) == v);
    ASSERT(any_cast<Mm>(bs.value("channel3Mode")) == c);
    ASSERT(any_cast<Mm>(bs.value("channel4Mode")) == c);
  }

  // Channel gains.
  {
    ASSERT(static_cast<int>(10*any_cast<float>(bs.value("channel1Gain"))) == 11);
    ASSERT(static_cast<int>(10*any_cast<float>(bs.value("channel2Gain"))) == 22);
    ASSERT(static_cast<int>(10*any_cast<float>(bs.value("channel3Gain"))) == 33);
    ASSERT(static_cast<int>(10*any_cast<float>(bs.value("channel4Gain"))) == 44);
  }

  // Channel IEPEs.
  {
    ASSERT(any_cast<bool>(bs.value("channel1Iepe")) == true);
    ASSERT(any_cast<bool>(bs.value("channel2Iepe")) == false);
    ASSERT(any_cast<bool>(bs.value("channel3Iepe")) == false);
    ASSERT(any_cast<bool>(bs.value("channel4Iepe")) == true);
  }

  // PWM enabled.
  {
    ASSERT(any_cast<bool>(bs.value("pwm1Enabled")) == false);
    ASSERT(any_cast<bool>(bs.value("pwm2Enabled")) == true);
  }

  // PWM frequencies.
  {
    ASSERT(any_cast<int>(bs.value("pwm1Frequency")) == 1);
    ASSERT(any_cast<int>(bs.value("pwm2Frequency")) == 10);
  }

  // PWM boundaries.
  {
    ASSERT(any_cast<int>(bs.value("pwm1LowBoundary"))  == 11);
    ASSERT(any_cast<int>(bs.value("pwm1HighBoundary")) == 22);
    ASSERT(any_cast<int>(bs.value("pwm2LowBoundary"))  == 33);
    ASSERT(any_cast<int>(bs.value("pwm2HighBoundary")) == 44);
  }

  // PWM repeat counts.
  {
    ASSERT(any_cast<int>(bs.value("pwm1RepeatCount")) == 0);
    ASSERT(any_cast<int>(bs.value("pwm2RepeatCount")) == 11);
  }

  // PWM duty cycles.
  {
    ASSERT(static_cast<int>(100*any_cast<float>(bs.value("pwm1DutyCycle"))) == 11);
    ASSERT(static_cast<int>(100*any_cast<float>(bs.value("pwm2DutyCycle"))) == 22);
  }
 } catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << std::endl;
  return 1;
 } catch (...) {
  std::cerr << "unknown error\n";
  return 2;
 }
