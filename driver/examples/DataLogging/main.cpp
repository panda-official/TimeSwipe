#include <nlohmann/json.hpp>

#include <iostream>

#include <csignal>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <functional>
#include <thread>
#include <chrono>
#include "timeswipe.hpp"

// #include <chrono>

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

void usage(const char* name)
{
    std::cerr << "Usage: 'sudo " << name << " [--config <configname>] [--input <input_type>] [--output <outname>] [-- time <runtime>] [--log-resample] [--trace-spi]'" << std::endl;
    std::cerr << "default for <configname> is ./config.json" << std::endl;
    std::cerr << "possible values: PRIMARY NORM DIGITAL. default for <input_type> is the first one from <configname>" << std::endl;
    std::cerr << "if --output given then <outname> created in TSV format" << std::endl;
}

int main(int argc, char *argv[])
{
    nlohmann::json config;
    std::ofstream data_log;
    bool dump = false;
    std::string configname = "config.json";
    std::string dumpname;
    std::string input;
    int runtime = 10;
    bool trace_spi = false;

    for (unsigned i = 1; i < argc; i++) {
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
        } else if (!strcmp(argv[i],"--log-resample")) {
            TimeSwipe::resample_log = true;
        } else if (!strcmp(argv[i],"--trace-spi")) {
            trace_spi = true;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    static std::unordered_map<std::string,TimeSwipe::Mode> const modes = {
        {"PRIMARY",TimeSwipe::Mode::Primary},
        {"NORM",TimeSwipe::Mode::Norm},
        {"DIGITAL",TimeSwipe::Mode::Digital},
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
    if (input.length()) configitem = *config.find(input);
    if(dumpname.length()) {
        data_log.open(dumpname);
        dump = true;
    }


    TimeSwipe tswipe;
    tswipe.TraceSPI(trace_spi);

    // Board Preparation

    tswipe.SetMode(modes.at(configitem["MODE"]));

    const auto& offs = configitem["SENSOR_OFFSET"];
    tswipe.SetSensorOffsets(offs[0], offs[1], offs[2], offs[3]);

    const auto& gains = configitem["SENSOR_GAIN"];
    tswipe.SetSensorGains(gains[0], gains[1], gains[2], gains[3]);

    const auto& trans = configitem["SENSOR_TRANSMISSION"];
    tswipe.SetSensorTransmissions(trans[0], trans[1], trans[2], trans[3]);


    // Board Shutdown on signals

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    shutdown_handler = [&](int signal) {
        tswipe.Stop();
        exit(1);
    };


    bool ret = tswipe.onEvent([&](TimeSwipeEvent&& event) {
        if (event.is<TimeSwipeEvent::Button>()) {
            auto button = event.get<TimeSwipeEvent::Button>();
            std::cout << "Button event: " <<  (button.pressed() ? "pressed":"released") << " counter: " << button.count() << std::endl;
        } else if(event.is<TimeSwipeEvent::Gain>()) {
            auto val = event.get<TimeSwipeEvent::Gain>();
            std::cout << "Gain event: " <<  val.value() << std::endl;
        } else if(event.is<TimeSwipeEvent::SetSecondary>()) {
            auto val = event.get<TimeSwipeEvent::SetSecondary>();
            std::cout << "SetSecondary event: " <<  val.value() << std::endl;
        } else if(event.is<TimeSwipeEvent::Bridge>()) {
            auto val = event.get<TimeSwipeEvent::Bridge>();
            std::cout << "Bridge event: " <<  val.value() << std::endl;
        } else if(event.is<TimeSwipeEvent::Record>()) {
            auto val = event.get<TimeSwipeEvent::Record>();
            std::cout << "Record event: " <<  val.value() << std::endl;
        } else if(event.is<TimeSwipeEvent::Offset>()) {
            auto val = event.get<TimeSwipeEvent::Offset>();
            std::cout << "Offset event: " <<  val.value() << std::endl;
        } else if(event.is<TimeSwipeEvent::Mode>()) {
            auto val = event.get<TimeSwipeEvent::Mode>();
            std::cout << "Mode event: " <<  val.value() << std::endl;
        }
    });
    if (!ret) {
        std::cerr << "onEvent init failed" << std::endl;
        return 1;
    }

    ret = tswipe.onError([&](uint64_t errors) {
        std::cout << "Got errors: " << errors << std::endl;
    });
    if (!ret) {
        std::cerr << "onError init failed" << std::endl;
        return 1;
    }

    tswipe.SetSampleRate(24000);
    tswipe.SetBurstSize(24000);

    // Board Start

    int counter = 0;
    ret = tswipe.Start([&](auto&& records, uint64_t errors) {
        counter += records.DataSize();
            for (size_t i = 0; i < records.DataSize(); i++) {
                if (i == 0) {
                    for (size_t j = 0; j < records.SensorsSize(); j++) {
                        if (j != 0) std::cout << "\t";
                        std::cout << records[j][i];
                    }
                    std::cout << '\n';
                }
                if (dump) {
                    for (size_t j = 0; j < records.SensorsSize(); j++) {
                        if (j != 0) data_log << "\t";
                        data_log << records[j][i];
                    }
                    data_log << '\n';
                }
            }
    });
    if (!ret) {
        std::cerr << "timeswipe start failed" << std::endl;
        return -1;
    }

    auto start = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(runtime));

    // Board Stop
    if (!tswipe.Stop()) {
        std::cerr << "timeswipe stop failed" << std::endl;
        return -1;
    }
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<float> diff = end - start;
    std::cout << "time: " << diff.count() << "s records: " << counter << " rec/sec: " << counter / diff.count() << "\n";

    return 0;
}
