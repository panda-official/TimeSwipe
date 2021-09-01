// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/driver/timeswipe.hpp"
#include "../../src/3rdparty/dmitigr/assert.hpp"
#include "../../src/3rdparty/dmitigr/progpar.hpp"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace chrono = std::chrono;
namespace fs = std::filesystem;
namespace progpar = dmitigr::progpar;
namespace drv = panda::timeswipe::driver;

inline progpar::Program_parameters params;

template<typename ... Types>
void message_error(Types&& ... parts)
{
  std::cerr << params.path().string() << ": ";
  (std::cerr << ... << parts) << std::endl;
}

template<typename ... Types>
void print_usage()
{
  message_error("usage:\n",
    "  ", params.path().string(),
    " --count=<uint> --duration=<uint> --sample-rate=<uint> --frequency=<uint>\n\n"
    "Options:\n"
    "  --count - a number of measurements\n"
    "  --duration - a duration of each measurement, seconds\n"
    "  --sample-rate - a sample rate to use\n"
    "  --frequency - a frequency to use\n");
}

int main(const int argc, const char* const argv[])
try {
  using chrono::seconds;

  // Get command-line parameters.
  params = {argc, argv};
  const auto [count_o, duration_o, sample_rate_o, frequency_o] =
    params.options("count", "duration", "sample-rate", "frequency");
  const auto count = std::stoul(count_o.not_empty_value());
  const seconds duration{std::stol(duration_o.not_empty_value())};
  const auto sample_rate = std::stoul(sample_rate_o.not_empty_value());
  const auto frequency = std::stoul(frequency_o.not_empty_value());

  // Check parameters.
  if (frequency > sample_rate)
    throw std::runtime_error{"frequency cannot be greater than sample-rate"};

  // Initialize Timeswipe.
  auto& ts = drv::Timeswipe::instance();
  ts.set_sample_rate(sample_rate);
  ts.set_burst_size(sample_rate / frequency);

  ts.set_event_handler([](drv::Event&& event)
  {
    try {
      if (auto* gain = event.get<drv::Event::Gain>()) {
        std::cout << "Gain event: " << gain->value() << std::endl;
      }
    } catch (const std::exception& e) {
      std::clog << "OnEvent: " << e.what() << '\n';
    } catch (...) {
      std::clog << "OnEvent: unknown error\n";
    }
  });

  // Start Timeswipe. (Measure.)
  {
    using chrono::duration_cast;
    using Dur = chrono::nanoseconds;

    std::condition_variable finish;
    bool finished{};
    std::mutex finished_mutex;
    std::ofstream log_file;
    std::ofstream elog_file;

    ts.start([
        logs_ready = false, &log_file, &elog_file,
        i = 0u, count,
        d = Dur{}, delta = duration_cast<Dur>(seconds{1}) / frequency, duration = duration_cast<Dur>(duration),
        &finish, &finished, &finished_mutex](auto data, const auto eco) mutable
    {
      if (finished) return;

      // (Re-)open logs.
      if (!logs_ready) {
        const auto log_name = "measurement_"+std::to_string(i)+".csv";
        const auto elog_name = "measurement_errors_"+std::to_string(i)+".csv";
        constexpr auto log_mode{std::ios_base::trunc | std::ios_base::out};
        log_file = std::ofstream{log_name, log_mode};
        log_file.precision(5 + 4);
        elog_file = std::ofstream{elog_name, log_mode};
        std::clog << "Writing " << log_name
                  << " (" << duration_cast<seconds>(duration).count() << " seconds)" << "...";
        logs_ready = true;
      }

      // Write data.
      const auto begin = chrono::system_clock::now();
      const auto row_count{data.size()};
      for (std::size_t row{}; row < row_count; ++row) {
        for (std::size_t col{}; col < data.sensor_count(); ++col)
          log_file << data[col][row] << " ";
        log_file << "\n";
      }
      if (eco)
        elog_file << eco << std::endl;
      const auto end = chrono::system_clock::now();

      // Check post-conditions.
      d += delta + end - begin;
      // std::clog << "d = " << d.count() << ", delta = " << delta.count() << std::endl;
      if (d >= duration) {
        std::clog << "done" << std::endl;
        d = {};
        i++;
        if (i >= count) {
          const std::lock_guard lk{finished_mutex};
          finish.notify_one();
          finished = true;
        } else
          logs_ready = false;
      }
    });
    std::unique_lock lk{finished_mutex};
    finish.wait(lk, [&finished]{ return finished; });
    ts.stop();
  }

  // Cleanup.
  for (unsigned i{}; i < count; ++i) {
    const auto elog_name = "measurement_errors_"+std::to_string(i)+".csv";
    if (!fs::file_size(elog_name))
      fs::remove(elog_name);
  }
} catch (const progpar::Exception& e) {
  std::cerr << e.what() << std::endl;
  print_usage();
  return 1;
} catch (const std::exception& e) {
  std::cerr << "Error: " << e.what() << std::endl;
  return 2;
} catch (...) {
  std::cerr << "Unknown error" << std::endl;
  return 3;
}
