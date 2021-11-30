// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/driver.hpp"
#include "../../src/3rdparty/dmitigr/fs/filesystem.hpp"
#include "../../src/3rdparty/dmitigr/prg/parameters.hpp"
#include "../../src/3rdparty/dmitigr/rajson/rajson.hpp"
#include "../../src/3rdparty/dmitigr/str/str.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace chrono = std::chrono;
namespace fs = std::filesystem;
namespace prg = dmitigr::prg;
namespace rajson = dmitigr::rajson;
namespace str = dmitigr::str;
namespace ts = panda::timeswipe;

inline prg::Parameters params;
inline std::atomic_int stop;

template<typename ... Types>
void message_error(Types&& ... parts)
{
  std::cerr << params.path().filename().string() << ": ";
  (std::cerr << ... << std::forward<Types>(parts)) << std::endl;
}

template<typename ... Types>
void print_usage()
{
  message_error("usage: ", params.path().filename().string(),
    " [--out-part-suffix=<string>] --config=<path>\n\n"
    "Options:\n"
    "  --config - a path to configuration file\n"
    "  --out-part-suffix - a string to use as an output file suffix");
}

void handle_signal(const int sig)
{
  stop = sig;
}

int main(const int argc, const char* const argv[])
try {
  using ms = chrono::milliseconds;
  using ns = chrono::nanoseconds;

  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);

  // Get command-line parameters.
  params = {argc, argv};
  const auto out_suffix = params.option("out-part-suffix").value_or("");
  const auto json = rajson::to_document(
    str::to_string(fs::path{params.option("config").not_empty_value()}));
  rajson::Value_view jv{json};
  const ts::Board_settings board_settings{rajson::to_text(jv.mandatory("board").value())};
  const ts::Driver_settings driver_settings{rajson::to_text(jv.mandatory("driver").value())};
  const auto out_count = jv.optional<int>("outPartCount").value_or(0);
  const ms out_duration{jv.optional<std::int64_t>("outPartDuration").value_or(10000)};

  // Initialize the driver.
  auto& driver = ts::Driver::instance().initialize();
  driver.set_settings(board_settings).set_settings(driver_settings);

  // Enable measurement.
  {
    using chrono::duration_cast;

    std::condition_variable finish;
    bool finished{};
    std::mutex finished_mutex;
    const auto finish_measurement = [&finish, &finished, &finished_mutex]
    {
      const std::lock_guard lk{finished_mutex};
      finished = true;
      finish.notify_one();
    };

    std::ofstream out_file;
    std::ofstream log_file;
    driver.start_measurement([
        files_ready = false, &out_file, &log_file,
        outsuf = !out_suffix.empty() ? std::string{"_"}.append(out_suffix) : "",
        i = 0, out_count,
        d = ns::zero(), duration = duration_cast<ns>(out_duration),
        t_curr = chrono::system_clock::time_point{},
        &finished, &finish_measurement](const auto data, const auto err) mutable
    {
      // Short-circuit if finished.
      if (finished) return;

      // Check stop-condition.
      if (stop) {
        std::clog << "received signal " << stop << std::endl;
        finish_measurement();
        return;
      }

      // Check and update finish-conditions.
      const auto t_prev = t_curr;
      t_curr = chrono::system_clock::now();
      if (t_prev.time_since_epoch() != ns::zero()) {
        const auto delta = duration_cast<ns>(t_curr - t_prev);
        d += delta;
        // std::clog << "d = " << d.count() << ", delta = " << delta.count() << std::endl;
        if (d >= duration) {
          std::clog << "done" << std::endl;
          d = {};
          i++;
          if (i >= out_count) {
            finish_measurement();
            return;
          } else
            files_ready = false;
        }
      }

      // (Re-)open logs.
      if (out_count && !files_ready) {
        const auto out_name = "meas_"+std::to_string(i)+outsuf+".csv";
        const auto log_name = "meas_"+std::to_string(i)+".log";
        constexpr auto fmode{std::ios_base::trunc | std::ios_base::out};
        if ( !(out_file = std::ofstream{out_name, fmode}))
          throw std::runtime_error{"cannot open file " + out_name};
        else if ( !(log_file = std::ofstream{log_name, fmode}))
          throw std::runtime_error{"cannot open file " + log_name};
        out_file.precision(5 + 4);
        std::clog << "Writing " << out_name
                  << " (" << duration_cast<ms>(duration).count() << " ms)...";
        files_ready = true;
      }

      // Choise output streams.
      std::ostream& out_stream = out_count ? out_file : std::cout;
      std::ostream& err_stream = out_count ? log_file : std::cerr;

      // Write data.
      const auto row_count = data.row_count();
      const auto col_count = data.column_count();
      for (std::size_t row{}; row < row_count; ++row) {
        for (std::size_t col{}; col < col_count; ++col)
          out_stream << data.value(col, row) << " ";
        out_stream << "\n";
      }
      if (err)
        err_stream << err << std::endl;
    });

    std::unique_lock lk{finished_mutex};
    finish.wait(lk, [&finished]{ return finished; });
    driver.stop_measurement();
  }

  // Cleanup.
  for (int i{}; i < out_count; ++i) {
    const auto log_name = "meas_"+std::to_string(i)+".log";
    if (fs::exists(log_name) && !fs::file_size(log_name))
      fs::remove(log_name);
  }
} catch (const prg::Exception& e) {
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
