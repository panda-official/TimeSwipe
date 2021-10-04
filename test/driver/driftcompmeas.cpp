// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/firmware/json.hpp"
#include "../../src/driver.hpp"
#include "../../src/error_detail.hpp"
#include "../../src/3rdparty/dmitigr/math.hpp"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace chrono = std::chrono;
namespace math = dmitigr::math;
namespace ts = panda::timeswipe;

namespace {

void measure(ts::Driver& drv, const std::chrono::milliseconds dur)
{
  drv.set_settings(std::move(ts::Driver_settings{}.set_sample_rate(48000)
      .set_burst_buffer_size(48000)));
  const int channel_count = drv.max_channel_count();
  std::vector<double> aavg(channel_count);
  std::vector<double> astddev(channel_count);
  drv.start_measurement([&aavg,
      &astddev, call_count=0, channel_count](auto data, const auto) mutable
  {
    for (int i{}; i < channel_count; ++i) {
      const auto& channel= data[i];
      const auto avg = math::avg(channel);
      const auto var = math::variance(channel, avg, false);
      const auto stddev = std::sqrt(var);
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
  drv.stop_measurement();

  // Print the results
  const auto prec = std::cout.precision();
  try {
    std::cout.precision(5 + 4);
    std::cout << "avg: ";
    for (int i{}; i < channel_count; ++i) {
      std::cout << aavg[i];
      if (i < channel_count - 1)
        std::cout << ' ';
    }
    std::cout << "\nstddev: ";
    for (int i{}; i < channel_count; ++i) {
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
  auto& driver = ts::Driver::instance();
  PANDA_TIMESWIPE_ASSERT(!driver.is_measurement_started());

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
      if (!cs->empty())
        driver.set_board_settings(ts::Board_settings{cs->dump()});
    }
  }

  if (!driver.drift_references()) {
    // Normally, it means the first program run.
    const auto refs = driver.calculate_drift_references();
    (void)refs;
    PANDA_TIMESWIPE_ASSERT(refs.size() == static_cast<unsigned>(driver.max_channel_count()));
  }
  PANDA_TIMESWIPE_ASSERT(driver.drift_references());

  // Calculate deltas.
  PANDA_TIMESWIPE_ASSERT(!driver.drift_deltas());
  auto deltas{driver.calculate_drift_deltas()};
  (void)deltas;
  PANDA_TIMESWIPE_ASSERT(deltas.size() == static_cast<unsigned>(driver.max_channel_count()));
  PANDA_TIMESWIPE_ASSERT(driver.drift_deltas());

  // Measure.
  measure(driver, dur);
} catch (const std::exception& e) {
  std::clog << "error: " << e.what() << std::endl;
  return 1;
} catch (...) {
  std::clog << "unknown error" << std::endl;
  return 2;
}
