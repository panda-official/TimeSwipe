/*
 * Run as:
 *   sudo ./datlog --config datlog.json --input IEPE --output temp.txt
 *
 * This test will gather data for 10 seconds according to the configuration file
 * specified, from the `IEPE` inputs and will save the data in CSV format to the
 * file `temp.txt`.
 */

#include "../../src/driver.hpp"
#include "../../src/firmware/json.hpp"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <thread>

#include <unistd.h>

namespace ts = panda::timeswipe;

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

void usage(const char* name)
{
    std::cerr << "Usage: 'sudo " << name << " [--config <configname>] [--input <input_type>] [--output <outname>] [-- time <runtime>] [--samplerate <hz>] [--trace-spi]'" << std::endl;
    std::cerr << "default for <configname> is ./datlog.json" << std::endl;
    std::cerr << "possible values: PRIMARY NORM DIGITAL. default for <input_type> is the first one from <configname>" << std::endl;
    std::cerr << "if --output given then <outname> created in TSV format" << std::endl;
}

int main(int argc, char *argv[])
{
    nlohmann::json config;
    std::ofstream data_log;
    bool dump = false;
    std::string configname = "datlog.json";
    std::string dumpname;
    std::string input;
    int runtime = 10;
    int samplerate = 48000;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i],"--config")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            configname = argv[i+1];
            ++i;
        } else if (!strcmp(argv[i],"--input")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            input = argv[i+1];
            ++i;
        } else if (!strcmp(argv[i],"--output")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            dumpname = argv[i+1];
            ++i;
        } else if (!strcmp(argv[i],"--time")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            runtime = std::stoi (argv[i+1] );
            ++i;
        } else if (!strcmp(argv[i],"--samplerate")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            samplerate = std::stoi (argv[i+1] );
            ++i;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    using panda::timeswipe::Signal_mode;
    static std::unordered_map<std::string, Signal_mode> const modes = {
      {"PRIMARY", Signal_mode::iepe},
      {"NORM", Signal_mode::normal},
      {"DIGITAL", Signal_mode::digital},
    };

    std::ifstream iconfigname;
    iconfigname.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        iconfigname.open(configname);
        iconfigname >> config;
    } catch (std::system_error& e) {
        std::cerr << "Open config file \"" << configname << "\" failed: " << e.code().message() << std::endl;
        std::cerr << "Check file exists and has read access permissions" << std::endl;
        return 2;
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "config file \"" << configname << "\" parse failed" << std::endl
                  << "\tmessage: " << e.what() << std::endl
                  << "\texception id: " << e.id << std::endl
                  << "\tbyte position of error: " << e.byte << std::endl;
        return 2;
    }
    auto& configitem = *config.begin();

    auto& config_script=*config.find("CONFIG_SCRIPT");

    if (input.length()) configitem = *config.find(input);
    if(dumpname.length()) {
        data_log.open(dumpname);
        dump = true;
    }


    auto& driver = ts::Driver::instance().initialize();

    // Board Preparation
    ts::Board_settings settings;
    if (!config_script.empty())
      settings = ts::Board_settings{config_script.dump()};
    settings.set_signal_mode(modes.at(configitem["MODE"]));

    driver.set_settings(settings);

    // Board Shutdown on signals

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    shutdown_handler = [&](int /*signal*/) {
        driver.stop_measurement();
        exit(1);
    };

    // Start measurement.
    unsigned total_row_count{};
    driver.set_settings(ts::Driver_settings{}
      .set_sample_rate(samplerate).set_burst_buffer_size(samplerate))
      .start_measurement([&](const auto records, const int error_marker) {
        if (error_marker < 0) {
          std::clog << "Got fatal error " << -error_marker << "\n";
          return;
        } else if (error_marker > 0) {
          std::cout << "Got errors count " << error_marker << "\n";
          return;
        }
        const auto col_count = records.column_count();
        const auto row_count = records.row_count();
          total_row_count += row_count;
        for (size_t i = 0; i < row_count; i++) {
          if (i == 0) {
            for (size_t j = 0; j < col_count; j++) {
              if (j != 0) std::cout << "\t";
              std::cout << records.value(j, i);
            }
            std::cout << '\n';
          }
          if (dump) {
            for (size_t j = 0; j < col_count; j++) {
              if (j != 0) data_log << "\t";
              data_log << records.value(j, i);
            }
            data_log << '\n';
          }
        }
      });

    const auto start_moment = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds{runtime});
    driver.stop_measurement();
    const auto end_moment = std::chrono::system_clock::now();
    const std::chrono::duration<float> duration = end_moment - start_moment;
    std::cout << "time: " << duration.count() << "s records: " << total_row_count
              << " rec/sec: " << total_row_count / duration.count() << std::endl;
}
