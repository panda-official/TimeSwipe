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
    std::cerr << "Usage: 'sudo " << name << " [--config <configname>] [--input <input_type>] [--output <outname>]'" << std::endl;
    std::cerr << "default for <configname> is ./config.json" << std::endl;
    std::cerr << "default for <input_type> is the first one from <configname>" << std::endl;
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
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    std::ifstream iconfigname(configname);
    iconfigname >> config;
    auto& configitem = *config.begin();
    if (input.length()) configitem = *config.find(input);
    if(dumpname.length()) {
        data_log.open(dumpname);
        dump = true;
    }


    TimeSwipe tswipe;

    // Board Preparation

    tswipe.SetBridge(configitem["U_BRIDGE"]);

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

    // Board Start

    bool ret = tswipe.onButton([&](bool pressed, unsigned count) {
        std::cout << "Button: " <<  (pressed ? "pressed":"released") << std::endl;
    });
    if (!ret) {
        std::cerr << "onButton init failed" << std::endl;
        return 1;
    }

    ret = tswipe.onError([&](uint64_t errors) {
        std::cout << "Got errors: " << errors << std::endl;
    });
    if (!ret) {
        std::cerr << "onError init failed" << std::endl;
        return 1;
    }

    ret = tswipe.Start([&](auto&& records, uint64_t errors) {
            for (size_t i = 0; i < records.size(); i++) {
                const auto& rec = records[i];
                if (i==0) std::cout << rec.Sensors[0] << "\t" << rec.Sensors[1] << "\t" << rec.Sensors[2] << "\t" << rec.Sensors[3] << "\n";
                if (dump) data_log << rec.Sensors[0] << "\t" << rec.Sensors[1] << "\t" << rec.Sensors[2] << "\t" << rec.Sensors[3] << "\n";
            }
    });
    if (!ret) {
        std::cerr << "timeswipe start failed" << std::endl;
        return -1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Board Stop
    if (!tswipe.Stop()) {
        std::cerr << "timeswipe stop failed" << std::endl;
        return -1;
    }

    return 0;
}
