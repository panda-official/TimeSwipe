// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/driver/timeswipe.hpp"

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

namespace chrono = std::chrono;

namespace {

void measure(TimeSwipe& ts, const std::string& logfile)
{
  ts.SetSampleRate(48000);
  ts.SetBurstSize(48000 / 10);
  constexpr auto log_mode{std::ios_base::trunc | std::ios_base::out};
  std::ofstream log{logfile, log_mode};
  log.precision(5 + 4);
  ts.Start([&log](auto data, const auto)
  {
    const auto row_count{data.DataSize()};
    for (auto row{0*row_count}; row < row_count; ++row) {
      for (std::size_t col{}; col < data.SensorsSize(); ++col)
        log << data[col][row] << " ";
      log << "\n";
    }
  });
  std::this_thread::yield();
  std::this_thread::sleep_for(chrono::seconds{1});
  const auto stopped = ts.Stop();
  assert(stopped);
}

} // namespace

int main()
try {
  TimeSwipe ts;
  std::clog << "Measuring 1..." << std::endl;
  measure(ts, "measurement1.log");
  std::clog << "done" << std::endl;

  std::clog << "Measuring 2..." << std::endl;
  measure(ts, "measurement2.log");
  std::clog << "done" << std::endl;
} catch (const std::exception& e) {
  std::clog << "Error: " << e.what() << std::endl;
  return 1;
} catch (...) {
  std::clog << "Unknown error" << std::endl;
  return 2;
}
