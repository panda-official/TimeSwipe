// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/driver.hpp"
#include "../../src/error.hpp"
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

namespace {

void log(const std::vector<float>& data)
{
  for (const auto dat : data)
    std::clog << dat << " ";
  std::clog << std::endl;
}

void measure(ts::Timeswipe& ts, const std::filesystem::path& logfile)
{
  ts.set_settings(std::move(ts::Driver_settings{}.set_sample_rate(48000)
      .set_burst_buffer_size(48000 / 10)));
  constexpr auto log_mode{std::ios_base::trunc | std::ios_base::out};
  std::ofstream log{logfile, log_mode};
  log.precision(5 + 4);
  ts.start([&log](const auto data, const auto)
  {
    const auto row_count{data.get_size()};
    for (auto row{0*row_count}; row < row_count; ++row) {
      for (const auto& channel : data)
        log << channel[row] << " ";
      log << "\n";
    }
  });
  std::this_thread::sleep_for(chrono::seconds{1});
  ts.stop();
}

} // namespace

int main()
{
  auto& ts = ts::Timeswipe::get_instance();
  assert(!ts.IsBusy());

  ts.clear_drift_references();
  assert(!ts.drift_references(false));
  assert(!ts.drift_references(true));

  try {
    assert(!ts.drift_deltas());
    ts.calculate_drift_deltas();
  } catch (const ts::Runtime_exception& e) {
    assert(e.condition() == ts::Errc::no_drift_references);
  }

  // ---------------------------------------------------------------------------
  // Calculate references
  // ---------------------------------------------------------------------------

  const auto refs{ts.calculate_drift_references()};
  assert(refs.size() == SensorsData::SensorsSize());
  std::clog << "Calculated references: ";
  log(refs);

  assert(!ts.IsBusy());
  {
    const auto refs1{ts.get_drift_references(false)};
    const auto refs2{ts.get_drift_references(true)};
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

  assert(!ts.drift_deltas());
  auto deltas{ts.calculate_drift_deltas()};
  assert(deltas.size() == refs.size());
  std::clog << "Calculated deltas: ";
  log(deltas);

  assert(!ts.IsBusy());
  {
    const auto deltas1{ts.get_drift_deltas()};
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

  ts.clear_drift_deltas();
  assert(!ts.drift_deltas());

  std::clog << "Measuring uncompensated..." << std::endl;
  measure(ts, "drift_compensation-uncompensated.log");
  assert(!ts.IsBusy());
  std::clog << "done" << std::endl;

  // ---------------------------------------------------------------------------
  // Calculate deltas 2
  // ---------------------------------------------------------------------------

  assert(!ts.drift_deltas());
  deltas = ts.calculate_drift_deltas();
  assert(deltas.size() == refs.size());
  std::clog << "Calculated deltas 2: ";
  log(deltas);

  assert(!ts.IsBusy());
  {
    const auto deltas1{ts.get_drift_deltas()};
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

  ts.clear_drift_deltas();
  assert(!ts.drift_deltas());

  std::clog << "Measuring uncompensated 2..." << std::endl;
  measure(ts, "drift_compensation-uncompensated2.log");
  assert(!ts.IsBusy());
  std::clog << "done" << std::endl;

  // ---------------------------------------------------------------------------
  // Clear references
  // ---------------------------------------------------------------------------

  ts.clear_drift_references();
  assert(!ts.drift_references(false));
  assert(!ts.drift_references(true));
}
