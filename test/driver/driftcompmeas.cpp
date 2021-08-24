// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/common/json.hpp"
#include "../../src/driver.hpp"
#include "../../src/3rdparty/dmitigr/math.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace chrono = std::chrono;
namespace drv = panda::timeswipe::driver;
namespace math = dmitigr::math;

namespace {

void measure(TimeSwipe& ts, const std::chrono::milliseconds dur)
{
  ts.SetSampleRate(48000);
  ts.SetBurstSize(48000);
  constexpr auto channel_count{SensorsData::SensorsSize()};
  std::vector<double> aavg(channel_count);
  std::vector<double> astddev(channel_count);
  ts.Start([&aavg, &astddev, call_count=0](auto data, const auto) mutable
  {
    for (auto i{0*channel_count}; i < channel_count; ++i) {
      const auto& channel{data[i]};
      const auto avg{math::avg(channel)};
      const auto var{math::dispersion(channel, avg, false)};
      const auto stddev{std::sqrt(var)};
      aavg[i] += avg;
      astddev[i] += stddev;
      if (call_count) {
        aavg[i] /= 2;
        astddev[i] /= 2;
      }
    }
    call_count++;
  });
  std::this_thread::sleep_for(dur);
  const auto stopped = ts.Stop();
  (void)stopped;
  assert(stopped);

  // Print the results
  const auto prec{std::cout.precision()};
  try {
    std::cout.precision(5 + 4);
    std::cout << "avg: ";
    for (std::size_t i{}; i < channel_count; ++i) {
      std::cout << aavg[i];
      if (i < channel_count - 1)
        std::cout << ' ';
    }
    std::cout << "\nstddev: ";
    for (std::size_t i{}; i < channel_count; ++i) {
      std::cout << astddev[i];
      if (i < channel_count - 1)
        std::cout << ' ';
    }
    std::cout << "\n";
  } catch (...) {
    std::cout.precision(prec);
    throw;
  }
  std::cout.precision(prec);
}

} // namespace

int main(const int argc, const char* const argv[])
try {
  TimeSwipe ts;
  assert(!ts.IsBusy());

  // Set the measure duration.
  using ms = chrono::milliseconds;
  const auto dur{argc > 1 ? ms{std::stoi(argv[1])} : ms{500}};
  if (dur <= ms::zero())
    throw std::invalid_argument{"invalid duration"};

  // Process the config file.
  const auto cfg_file_name{argc > 2 ? argv[2] : "driftcompmeas.json"};
  if (std::ifstream in{cfg_file_name}) {
    nlohmann::json config;
    in >> config;
    if (const auto cs = config.find("CONFIG_SCRIPT"); cs != config.end()) {
      if (!cs->empty()) {
        std::string msg;
        ts.SetSettings(cs->dump(), msg);
        if(!msg.empty())
          throw std::invalid_argument{"invalid config: " + msg};
      }
    }
  }

  if (!ts.DriftReferences()) {
    // Normally, it means the first program run.
    const auto refs{ts.CalculateDriftReferences()};
    (void)refs;
    assert(refs.size() == SensorsData::SensorsSize());
  }
  assert(ts.DriftReferences());

  // Calculate deltas.
  assert(!ts.DriftDeltas());
  auto deltas{ts.CalculateDriftDeltas()};
  (void)deltas;
  assert(deltas.size() == SensorsData::SensorsSize());
  assert(ts.DriftDeltas());

  // Measure.
  measure(ts, dur);
} catch (const std::exception& e) {
  std::clog << "error: " << e.what() << std::endl;
  return 1;
} catch (...) {
  std::clog << "unknown error" << std::endl;
  return 2;
}
