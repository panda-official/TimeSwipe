// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/driver.hpp"
#include "../../src/error_detail.hpp"
#include "../../src/3rdparty/dmitigr/filesystem.hpp"

#include <algorithm>
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

void measure(ts::Driver& ts, const std::filesystem::path& logfile)
{
  ts.set_settings(std::move(ts::Driver_settings{}.set_sample_rate(48000)
      .set_burst_buffer_size(48000 / 10)));
  constexpr auto log_mode{std::ios_base::trunc | std::ios_base::out};
  std::ofstream log{logfile, log_mode};
  log.precision(5 + 4);
  ts.start_measurement([&log](const auto data, const auto)
  {
    const auto row_count = data.size();
    for (std::size_t row{}; row < row_count; ++row) {
      for (const auto& channel : data)
        log << channel[row] << " ";
      log << "\n";
    }
  });
  std::this_thread::sleep_for(chrono::seconds{1});
  ts.stop_measurement();
}

} // namespace

int main()
{
  auto& driver = ts::Driver::instance();
  PANDA_TIMESWIPE_ASSERT(!driver.is_measurement_started());

  driver.clear_drift_references();
  PANDA_TIMESWIPE_ASSERT(!driver.drift_references(false));
  PANDA_TIMESWIPE_ASSERT(!driver.drift_references(true));

  try {
    PANDA_TIMESWIPE_ASSERT(!driver.drift_deltas());
    driver.calculate_drift_deltas();
  } catch (const ts::Exception& e) {
    PANDA_TIMESWIPE_ASSERT(e.condition() == ts::Generic_errc::drift_comp_refs_not_found);
  }

  // ---------------------------------------------------------------------------
  // Calculate references
  // ---------------------------------------------------------------------------

  const auto refs{driver.calculate_drift_references()};
  PANDA_TIMESWIPE_ASSERT(refs.size() == static_cast<unsigned>(driver.max_channel_count()));
  std::clog << "Calculated references: ";
  log(refs);

  PANDA_TIMESWIPE_ASSERT(!driver.is_measurement_started());
  {
    const auto refs1{driver.drift_references(false)};
    const auto refs2{driver.drift_references(true)};
    PANDA_TIMESWIPE_ASSERT(refs1);
    PANDA_TIMESWIPE_ASSERT(refs2);
    std::vector<int> refsi(refs.size());
    transform(cbegin(refs), cend(refs), begin(refsi),
      [](const auto v) {return static_cast<int>(std::lroundf(v));});
    std::vector<int> refsi1(refs1->size());
    transform(cbegin(*refs1), cend(*refs1), begin(refsi1),
      [](const auto v) {return static_cast<int>(std::lroundf(v));});
    std::vector<int> refsi2(refs2->size());
    transform(cbegin(*refs2), cend(*refs2), begin(refsi2),
      [](const auto v) {return static_cast<int>(std::lroundf(v));});
    PANDA_TIMESWIPE_ASSERT(refsi1 == refsi2);
    PANDA_TIMESWIPE_ASSERT(refsi == refsi1);
  }

  // ---------------------------------------------------------------------------
  // Calculate deltas
  // ---------------------------------------------------------------------------

  PANDA_TIMESWIPE_ASSERT(!driver.drift_deltas());
  auto deltas{driver.calculate_drift_deltas()};
  PANDA_TIMESWIPE_ASSERT(deltas.size() == refs.size());
  std::clog << "Calculated deltas: ";
  log(deltas);

  PANDA_TIMESWIPE_ASSERT(!driver.is_measurement_started());
  {
    const auto deltas1{driver.drift_deltas()};
    PANDA_TIMESWIPE_ASSERT(deltas1);
    PANDA_TIMESWIPE_ASSERT(deltas == deltas1);
  }

  // ---------------------------------------------------------------------------
  // Measuring
  // ---------------------------------------------------------------------------

  std::clog << "Measuring compensated..." << std::endl;
  measure(driver, "drift_compensation-compensated.log");
  PANDA_TIMESWIPE_ASSERT(!driver.is_measurement_started());
  std::clog << "done" << std::endl;

  driver.clear_drift_deltas();
  PANDA_TIMESWIPE_ASSERT(!driver.drift_deltas());

  std::clog << "Measuring uncompensated..." << std::endl;
  measure(driver, "drift_compensation-uncompensated.log");
  PANDA_TIMESWIPE_ASSERT(!driver.is_measurement_started());
  std::clog << "done" << std::endl;

  // ---------------------------------------------------------------------------
  // Calculate deltas 2
  // ---------------------------------------------------------------------------

  PANDA_TIMESWIPE_ASSERT(!driver.drift_deltas());
  deltas = driver.calculate_drift_deltas();
  PANDA_TIMESWIPE_ASSERT(deltas.size() == refs.size());
  std::clog << "Calculated deltas 2: ";
  log(deltas);

  PANDA_TIMESWIPE_ASSERT(!driver.is_measurement_started());
  {
    const auto deltas1{driver.drift_deltas()};
    PANDA_TIMESWIPE_ASSERT(deltas1);
    PANDA_TIMESWIPE_ASSERT(deltas == deltas1);
  }

  // ---------------------------------------------------------------------------
  // Measuring 2
  // ---------------------------------------------------------------------------

  std::clog << "Measuring compensated 2..." << std::endl;
  measure(driver, "drift_compensation-compensated2.log");
  PANDA_TIMESWIPE_ASSERT(!driver.is_measurement_started());
  std::clog << "done" << std::endl;

  driver.clear_drift_deltas();
  PANDA_TIMESWIPE_ASSERT(!driver.drift_deltas());

  std::clog << "Measuring uncompensated 2..." << std::endl;
  measure(driver, "drift_compensation-uncompensated2.log");
  PANDA_TIMESWIPE_ASSERT(!driver.is_measurement_started());
  std::clog << "done" << std::endl;

  // ---------------------------------------------------------------------------
  // Clear references
  // ---------------------------------------------------------------------------

  driver.clear_drift_references();
  PANDA_TIMESWIPE_ASSERT(!driver.drift_references(false));
  PANDA_TIMESWIPE_ASSERT(!driver.drift_references(true));
}
