// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/driver.hpp"
#include "../../src/common/error.hpp"
#include "../../src/3rdparty/dmitigr/filesystem.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>

namespace chrono = std::chrono;
namespace ts = panda::timeswipe;
namespace drv = ts::driver;

namespace {

void log(const std::vector<float>& data)
{
  for (const auto dat : data)
    std::clog << dat << " ";
  std::clog << std::endl;
}

void measure(drv::TimeSwipe& ts, const std::filesystem::path& logfile)
{
  ts.SetSampleRate(48000);
  ts.SetBurstSize(48000 / 10);
  constexpr auto log_mode{std::ios_base::trunc | std::ios_base::out};
  std::ofstream log{logfile, log_mode};
  log.precision(5 + 4);
  ts.Start([&log](auto data, const auto)
  {
    const auto row_count{data.size()};
    for (auto row{0*row_count}; row < row_count; ++row) {
      for (const auto& channel : data)
        log << channel[row] << " ";
      log << "\n";
    }
  });
  std::this_thread::sleep_for(chrono::seconds{1});
  const auto stopped = ts.Stop();
  (void)stopped;
  assert(stopped);
}

} // namespace

int main()
{
  auto& ts = drv::TimeSwipe::instance();
  assert(!ts.IsBusy());

  ts.ClearDriftReferences();
  assert(!ts.DriftReferences(false));
  assert(!ts.DriftReferences(true));

  try {
    assert(!ts.DriftDeltas());
    ts.CalculateDriftDeltas();
  } catch (const ts::RuntimeException& e) {
    assert(e.condition() == ts::Errc::kNoDriftReferences);
  }

  // ---------------------------------------------------------------------------
  // Calculate references
  // ---------------------------------------------------------------------------

  const auto refs{ts.CalculateDriftReferences()};
  assert(refs.size() == SensorsData::SensorsSize());
  std::clog << "Calculated references: ";
  log(refs);

  assert(!ts.IsBusy());
  {
    const auto refs1{ts.DriftReferences(false)};
    const auto refs2{ts.DriftReferences(true)};
    assert(refs1);
    assert(refs2);
    std::vector<int> refsi(refs.size());
    transform(cbegin(refs), cend(refs), begin(refsi),
      [](const auto v) {return static_cast<int>(std::lroundf(v));});
    std::vector<int> refsi1(refs1->size());
    transform(cbegin(*refs1), cend(*refs1), begin(refsi1),
      [](const auto v) {return static_cast<int>(std::lroundf(v));});
    std::vector<int> refsi2(refs2->size());
    transform(cbegin(*refs2), cend(*refs2), begin(refsi2),
      [](const auto v) {return static_cast<int>(std::lroundf(v));});
    assert(refsi1 == refsi2);
    assert(refsi == refsi1);
  }

  // ---------------------------------------------------------------------------
  // Calculate deltas
  // ---------------------------------------------------------------------------

  assert(!ts.DriftDeltas());
  auto deltas{ts.CalculateDriftDeltas()};
  assert(deltas.size() == refs.size());
  std::clog << "Calculated deltas: ";
  log(deltas);

  assert(!ts.IsBusy());
  {
    const auto deltas1{ts.DriftDeltas()};
    assert(deltas1);
    assert(deltas == deltas1);
  }

  // ---------------------------------------------------------------------------
  // Measuring
  // ---------------------------------------------------------------------------

  std::clog << "Measuring compensated..." << std::endl;
  measure(ts, "drift_compensation-compensated.log");
  assert(!ts.IsBusy());
  std::clog << "done" << std::endl;

  ts.ClearDriftDeltas();
  assert(!ts.DriftDeltas());

  std::clog << "Measuring uncompensated..." << std::endl;
  measure(ts, "drift_compensation-uncompensated.log");
  assert(!ts.IsBusy());
  std::clog << "done" << std::endl;

  // ---------------------------------------------------------------------------
  // Calculate deltas 2
  // ---------------------------------------------------------------------------

  assert(!ts.DriftDeltas());
  deltas = ts.CalculateDriftDeltas();
  assert(deltas.size() == refs.size());
  std::clog << "Calculated deltas 2: ";
  log(deltas);

  assert(!ts.IsBusy());
  {
    const auto deltas1{ts.DriftDeltas()};
    assert(deltas1);
    assert(deltas == deltas1);
  }

  // ---------------------------------------------------------------------------
  // Measuring 2
  // ---------------------------------------------------------------------------

  std::clog << "Measuring compensated 2..." << std::endl;
  measure(ts, "drift_compensation-compensated2.log");
  assert(!ts.IsBusy());
  std::clog << "done" << std::endl;

  ts.ClearDriftDeltas();
  assert(!ts.DriftDeltas());

  std::clog << "Measuring uncompensated 2..." << std::endl;
  measure(ts, "drift_compensation-uncompensated2.log");
  assert(!ts.IsBusy());
  std::clog << "done" << std::endl;

  // ---------------------------------------------------------------------------
  // Clear references
  // ---------------------------------------------------------------------------

  ts.ClearDriftReferences();
  assert(!ts.DriftReferences(false));
  assert(!ts.DriftReferences(true));
}
