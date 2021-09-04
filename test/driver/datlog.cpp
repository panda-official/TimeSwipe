/*
 * Run as:
 *   sudo ./datlog --config datlog.json --input IEPE --output temp.txt
 *
 * This test will gather data for 10 seconds according to the configuration file
 * specified, from the `IEPE` inputs and will save the data in CSV format to the
 * file `temp.txt`.
 */

#include "../../src/common/json.hpp"
#include "../../src/driver/timeswipe.hpp"

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

namespace drv = panda::timeswipe::driver;

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


    auto& tswipe = drv::Timeswipe::instance();

    // Board Preparation
    drv::Timeswipe_state state;
    if (!config_script.empty())
      state = drv::Timeswipe_state{config_script.dump()};
    state.set_signal_mode(modes.at(configitem["MODE"]));

    tswipe.set_state(state);

    // Board Shutdown on signals

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    shutdown_handler = [&](int /*signal*/) {
        tswipe.stop();
        exit(1);
    };


    using drv::Event;
    tswipe.set_event_handler([&](Event&& event) {
      if (auto* button = event.get<Event::Button>())
        std::cout << "Button event: "
                  << (button->is_pressed() ? "pressed":"released")
                  << " counter: " << button->get_count() << std::endl;
      else if(auto* gain = event.get<Event::Gain>())
        std::cout << "Gain event: " << gain->get_value() << std::endl;
      else if(auto* val = event.get<Event::Set_secondary>())
        std::cout << "Set_secondary event: " << val->get_value() << std::endl;
      else if(auto* val = event.get<Event::Bridge>())
        std::cout << "Bridge event: " <<  val->get_value() << std::endl;
      else if(auto* val = event.get<Event::Record>())
        std::cout << "Record event: " <<  val->get_value() << std::endl;
      else if(auto* val = event.get<Event::Offset>())
        std::cout << "Offset event: " <<  val->get_value() << std::endl;
      else if(auto* val = event.get<Event::Mode>())
        std::cout << "Mode event: " <<  val->get_value() << std::endl;
    });

    tswipe.set_error_handler([&](const std::uint64_t errors)
    {
      std::cout << "Got errors: " << errors << std::endl;
    });
    tswipe.set_sample_rate(samplerate);
    tswipe.set_burst_size(samplerate);

    // Board Start
    int counter = 0;
    tswipe.start([&](auto&& records, uint64_t /*errors*/) {
      counter += records.get_size();
      for (size_t i = 0; i < records.get_size(); i++) {
        if (i == 0) {
          for (size_t j = 0; j < records.get_sensor_count(); j++) {
            if (j != 0) std::cout << "\t";
            std::cout << records[j][i];
          }
          std::cout << '\n';
        }
        if (dump) {
          for (size_t j = 0; j < records.get_sensor_count(); j++) {
            if (j != 0) data_log << "\t";
            data_log << records[j][i];
          }
          data_log << '\n';
        }
      }
    });

    auto start = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(runtime));

    // Board Stop
    tswipe.stop();

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<float> diff = end - start;
    std::cout << "time: " << diff.count() << "s records: " << counter << " rec/sec: " << counter / diff.count() << "\n";

    return 0;
}
